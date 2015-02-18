/*
 * Copyright (C) 2014 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @shaderSources.h
 * Contains the sources for all shaders used in Splash
 */

#ifndef SPLASH_SHADERSOURCES_H
#define SPLASH_SHADERSOURCES_H

namespace Splash
{

struct ShaderSources
{
    /**
     * Default vertex shader
     */
    const std::string VERTEX_SHADER_DEFAULT {R"(
        #version 330 core

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        layout(location = 2) in vec3 _normal;
        uniform mat4 _modelViewProjectionMatrix;
        uniform mat4 _normalMatrix;
        uniform vec3 _scale = vec3(1.0, 1.0, 1.0);

        out VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec3 normal;
        } vertexOut;

        void main(void)
        {
            vertexOut.position = _modelViewProjectionMatrix * vec4(_vertex.x * _scale.x, _vertex.y * _scale.y, _vertex.z * _scale.z, 1.0);
            gl_Position = vertexOut.position;
            vertexOut.normal = normalize((_normalMatrix * vec4(_normal, 0.0)).xyz);
            vertexOut.texCoord = _texcoord;
        }
    )"};

    /**
     * Textured fragment shader
     */
    const std::string FRAGMENT_SHADER_TEXTURE {R"(
        #version 330 core

        #define PI 3.14159265359

        uniform sampler2D _tex0;
        uniform sampler2D _tex1;
        uniform int _sideness = 0;
        uniform int _textureNbr = 0;
        uniform int _texBlendingMap = 0;
        uniform vec3 _cameraAttributes = vec3(0.05, 0.0, 1.0); // blendWidth, blackLevel and brightness
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g
        uniform int _isColorLUT = 0;
        uniform vec3 _colorLUT[256];
        uniform mat3 _colorMixMatrix = mat3(1.0, 0.0, 0.0,
                                            0.0, 1.0, 0.0,
                                            0.0, 0.0, 1.0);

        in VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec3 normal;
        } vertexIn;

        out vec4 fragColor;
        // Texture transformation
        uniform int _tex0_flip = 0;
        uniform int _tex0_flop = 0;
        //uniform int _tex1_flip = 0;
        //uniform int _tex1_flop = 0;
        // HapQ specific parameters
        uniform int _tex0_YCoCg = 0;
        //uniform int _tex1_YCoCg = 0;

        void main(void)
        {
            float blendWidth = _cameraAttributes.x;
            float blackLevel = _cameraAttributes.y;
            float brightness = _cameraAttributes.z;

            vec4 position = vertexIn.position;
            vec2 texCoord = vertexIn.texCoord;
            vec3 normal = vertexIn.normal;

            vec2 screenPos = vec2(position.x / position.w, position.y / position.w);
            vec2 screenSize = vec2(tan(_fovAndColorBalance.x / 2.0), tan(_fovAndColorBalance.y / 2.0));
            float cosNormalAngle = dot(normal, normalize(vec3(-screenSize.x * screenPos.x, -screenSize.y * screenPos.y, 1.0)));

            if ((cosNormalAngle <= 0.0 && _sideness == 1) || (cosNormalAngle >= 0.0 && _sideness == 2))
                discard;

            vec4 color;
            if (_tex0_flip == 1 && _tex0_flop == 0)
                color = texture(_tex0, vec2(texCoord.x, 1.0 - texCoord.y));
            else if (_tex0_flip == 0 && _tex0_flop == 1)
                color = texture(_tex0, vec2(1.0 - texCoord.x, texCoord.y));
            else if (_tex0_flip == 1 && _tex0_flop == 1)
                color = texture(_tex0, vec2(1.0 - texCoord.x, 1.0 - texCoord.y));
            else
                color = texture(_tex0, texCoord);

            // If the color is expressed as YCoCg (for HapQ compression), extract RGB color from it
            if (_tex0_YCoCg == 1)
            {
                float scale = (color.z * (255.0 / 8.0)) + 1.0;
                float Co = (color.x - (0.5 * 256.0 / 255.0)) / scale;
                float Cg = (color.y - (0.5 * 256.0 / 255.0)) / scale;
                float Y = color.w;
                color.rgba = vec4(Y + Co - Cg, Y + Cg, Y - Co - Cg, 1.0);
                color.rgb = pow(color.rgb, vec3(2.2));
            }

            float maxBalanceRatio = max(_fovAndColorBalance.z, _fovAndColorBalance.w);
            color.r *= _fovAndColorBalance.z / maxBalanceRatio;
            color.g *= 1.0 / maxBalanceRatio;
            color.b *= _fovAndColorBalance.w / maxBalanceRatio;

            // Black level
            float blackCorrection = max(min(blackLevel, 1.0), 0.0);
            color.rgb = color.rgb * (1.0 - blackLevel) + blackLevel;
            
            // If no blending map has been computed
            if (_texBlendingMap == 0)
            {
                if (_textureNbr > 1)
                {
                    vec4 color2 = texture(_tex1, texCoord);
                    color.rgb = color.rgb * (1.0 - color2.a) + color2.rgb * color2.a;
                }
            }
            // If there is a blending map
            else
            {
                int blendFactor = int(texture(_tex1, texCoord).r * 65536.0);
                // Extract the number of cameras
                int camNbr = blendFactor / 4096;
                blendFactor = blendFactor - camNbr * 4096;
                float blendFactorFloat = 0.0;

                // If the max channel value is higher than 2*blacklevel, we smooth the blending edges
                bool smoothBlend = false;
                if (color.r > blackLevel * 2.0 || color.g > blackLevel * 2.0 || color.b > blackLevel * 2.0)
                    smoothBlend = true;

                if (blendFactor == 0)
                    blendFactorFloat = 0.05; // The non-visible part is kinda hidden
                else if (blendWidth > 0.0 && smoothBlend == true)
                {
                    vec2 normalizedPos = vec2(screenPos.x / 2.0 + 0.5, screenPos.y / 2.0 + 0.5);
                    float distX = min(normalizedPos.x, 1.0 - normalizedPos.x);
                    float distY = min(normalizedPos.y, 1.0 - normalizedPos.y);
                    float dist = min(1.0, min(distX, distY) / blendWidth);
                    dist = smoothstep(0.0, 1.0, dist);
                    blendFactorFloat = 256.0 * dist / float(blendFactor);
                }
                else
                {
                    blendFactorFloat = 1.0 / float(camNbr);
                }
                color.rgb = color.rgb * min(1.0, blendFactorFloat);
            }

            // Brightness correction
            color.rgb = color.rgb * brightness;

            // Finally, correct for the incidence
            // cosNormalAngle can't be 0.0, it would have been discarded
            // TODO: this has to also use shift values to be meaningful
            //color.rgb /= abs(cosNormalAngle);

            // Color correction through a LUT
            if (_isColorLUT != 0)
            {
                ivec3 icolor = ivec3(round(color.rgb * 255.f));
                color.rgb = vec3(_colorLUT[icolor.r].r, _colorLUT[icolor.g].g, _colorLUT[icolor.b].b);
                //color.rgb = clamp(_colorMixMatrix * color.rgb, vec3(0.0), vec3(1.0));
            }
            
            fragColor.rgb = color.rgb;
            fragColor.a = 1.0;
        }
    )"};

    /**
     * Single color fragment shader
     */
    const std::string FRAGMENT_SHADER_COLOR {R"(
        #version 330 core

        #define PI 3.14159265359

        uniform int _sideness = 0;
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g
        uniform vec4 _color = vec4(0.0, 1.0, 0.0, 1.0);

        in VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec3 normal;
        } vertexIn;

        out vec4 fragColor;

        void main(void)
        {
            vec4 position = vertexIn.position;
            vec2 texCoord = vertexIn.texCoord;
            vec3 normal = vertexIn.normal;

            vec2 screenPos = vec2(position.x / position.w, position.y / position.w);
            vec2 screenSize = vec2(tan(_fovAndColorBalance.x / 2.0), tan(_fovAndColorBalance.y / 2.0));
            float cosNormalAngle = dot(normal, normalize(vec3(-screenSize.x * screenPos.x, -screenSize.y * screenPos.y, 1.0)));

            if ((cosNormalAngle <= 0.0 && _sideness == 1) || (cosNormalAngle >= 0.0 && _sideness == 2))
                discard;

            fragColor = _color;
        }
    )"};

    /**
     * UV drawing fragment shader
     * UV coordinates are encoded on 2 channels each, to get 16bits precision
     */
    const std::string FRAGMENT_SHADER_UV {R"(
        #version 330 core

        #define PI 3.14159265359

        uniform int _sideness = 0;
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g

        in VertexData
        {
            vec4 position;
            vec2 texCoord;
            vec3 normal;
        } vertexIn;

        out vec4 fragColor;

        void main(void)
        {
            vec4 position = vertexIn.position;
            vec2 texCoord = vertexIn.texCoord;
            vec3 normal = vertexIn.normal;

            vec2 screenPos = vec2(position.x / position.w, position.y / position.w);
            vec2 screenSize = vec2(tan(_fovAndColorBalance.x / 2.0), tan(_fovAndColorBalance.y / 2.0));
            float cosNormalAngle = dot(normal, normalize(vec3(-screenSize.x * screenPos.x, -screenSize.y * screenPos.y, 1.0)));

            if ((cosNormalAngle <= 0.0 && _sideness == 1) || (cosNormalAngle >= 0.0 && _sideness == 2))
                discard;

            float U = texCoord.x * 65536.0;
            float V = texCoord.y * 65536.0;

            fragColor.rg = vec2(floor(U / 256.0) / 256.0, (U - floor(U / 256.0) * 256.0) / 256.0);
            fragColor.ba = vec2(floor(V / 256.0) / 256.0, (V - floor(V / 256.0) * 256.0) / 256.0);
        }
    )"};

    /**
     * Wireframe rendering
     */
    const std::string VERTEX_SHADER_WIREFRAME {R"(
        #version 330 core

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        layout(location = 2) in vec3 _normal;
        uniform mat4 _modelViewProjectionMatrix;

        out VertexData
        {
            vec4 vertex;
            vec3 normal;
            vec2 texcoord;
        } vertexOut;

        void main()
        {
            vertexOut.vertex = _vertex;
            vertexOut.normal = _normal;
            vertexOut.texcoord = _texcoord;
        }
    )"};

    const std::string GEOMETRY_SHADER_WIREFRAME {R"(
        #version 330 core

        layout(triangles) in;
        layout(triangle_strip, max_vertices = 3) out;
        uniform mat4 _modelViewProjectionMatrix;
        uniform mat4 _normalMatrix;
        uniform vec3 _scale = vec3(1.0, 1.0, 1.0);

        in VertexData
        {
            vec4 vertex;
            vec3 normal;
            vec2 texcoord;
        } vertexIn[];

        out VertexData
        {
            vec2 texcoord;
            vec3 normal;
            vec3 bcoord;
            vec4 position;
        } vertexOut;

        void main()
        {
            vec4 v = _modelViewProjectionMatrix * vec4(vertexIn[0].vertex.xyz * _scale.xyz, 1.0);
            gl_Position = v;
            vertexOut.texcoord = vertexIn[0].texcoord;
            vertexOut.normal = (_normalMatrix * vec4(vertexIn[0].normal, 0.0)).xyz;
            vertexOut.bcoord = vec3(1.0, 0.0, 0.0);
            vertexOut.position = v;
            EmitVertex();

            v = _modelViewProjectionMatrix * vec4(vertexIn[1].vertex.xyz * _scale.xyz, 1.0);
            gl_Position = v;
            vertexOut.texcoord = vertexIn[1].texcoord;
            vertexOut.normal = (_normalMatrix * vec4(vertexIn[1].normal, 0.0)).xyz;
            vertexOut.bcoord = vec3(0.0, 1.0, 0.0);
            vertexOut.position = v;
            EmitVertex();

            v = _modelViewProjectionMatrix * vec4(vertexIn[2].vertex.xyz * _scale.xyz, 1.0);
            gl_Position = v;
            vertexOut.texcoord = vertexIn[2].texcoord;
            vertexOut.normal = (_normalMatrix * vec4(vertexIn[2].normal, 0.0)).xyz;
            vertexOut.bcoord = vec3(0.0, 0.0, 1.0);
            vertexOut.position = v;
            EmitVertex();

            EndPrimitive();
        }
    )"};

    const std::string FRAGMENT_SHADER_WIREFRAME {R"(
        #version 330 core

        #define PI 3.14159265359

        in VertexData
        {
            vec2 texcoord;
            vec3 normal;
            vec3 bcoord;
            vec4 position;
        } vertexIn;

        uniform int _sideness = 0;
        uniform vec4 _fovAndColorBalance = vec4(0.0, 0.0, 1.0, 1.0); // fovX and fovY, r/g and b/g
        out vec4 fragColor;

        void main(void)
        {
            vec4 position = vertexIn.position;
            vec3 normal = vertexIn.normal;

            vec2 screenPos = vec2(position.x / position.w, position.y / position.w);
            vec2 screenSize = vec2(tan(_fovAndColorBalance.x / 2.0), tan(_fovAndColorBalance.y / 2.0));
            float cosNormalAngle = dot(normal, normalize(vec3(-screenSize.x * screenPos.x, -screenSize.y * screenPos.y, 1.0)));

            if ((cosNormalAngle <= 0.0 && _sideness == 1) || (cosNormalAngle >= 0.0 && _sideness == 2))
                discard;

            vec3 b = vertexIn.bcoord;
            float minDist = min(min(b[0], b[1]), b[2]);
            vec4 matColor = vec4(0.3, 0.3, 0.3, 1.0);
            if (minDist < 0.025)
                fragColor.rgba = mix(vec4(1.0), matColor, (minDist - 0.0125) / 0.0125);
            else
                fragColor.rgba = matColor;
        }
    )"};

    /**
     * Rendering of the output windows
     */
    const std::string VERTEX_SHADER_WINDOW {R"(
        #version 330 core

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        //layout(location = 2) in vec3 _normal;
        //uniform mat4 _modelViewProjectionMatrix;
        //uniform vec3 _scale = vec3(1.0, 1.0, 1.0);
        smooth out vec2 texCoord;

        void main(void)
        {
            //gl_Position = _modelViewProjectionMatrix * vec4(_vertex.x * _scale.x, _vertex.y * _scale.y, _vertex.z * _scale.z, 1.0);
            gl_Position = vec4(_vertex.x, _vertex.y, _vertex.z, 1.0);
            texCoord = _texcoord;
        }
    )"};

    const std::string FRAGMENT_SHADER_WINDOW {R"(
        #version 330 core

        #define PI 3.14159265359

        uniform sampler2D _tex0;
        uniform sampler2D _tex1;
        uniform sampler2D _tex2;
        uniform sampler2D _tex3;
        uniform int _textureNbr = 0;
        uniform ivec4 _layout = ivec4(0, 1, 2, 3);
        uniform vec2 _gamma = vec2(1.0, 2.2);
        in vec2 texCoord;
        out vec4 fragColor;

        void main(void)
        {
            float frames = float(_textureNbr);
            for (int i = 0; i < _textureNbr; ++i)
            {
                int value = _layout[i];
                for (int j = i + 1; j < _textureNbr; ++j)
                {
                    if (_layout[j] == value)
                    {
                        frames--;
                        break;
                    }
                }
            }

            fragColor.rgba = vec4(0.0);
            if (_textureNbr > 0 && texCoord.x > float(_layout[0]) / frames && texCoord.x < (float(_layout[0]) + 1.0) / frames)
            {
                fragColor = texture(_tex0, vec2((texCoord.x - float(_layout[0]) / frames) * frames, texCoord.y));
            }
            if (_textureNbr > 1 && texCoord.x > float(_layout[1]) / frames && texCoord.x < (float(_layout[1]) + 1.0) / frames)
            {
                vec4 color = texture(_tex1, vec2((texCoord.x - float(_layout[1]) / frames) * frames, texCoord.y));
                fragColor.rgb = mix(fragColor.rgb, color.rgb, color.a);
            }
            if (_textureNbr > 2 && texCoord.x > float(_layout[2]) / frames && texCoord.x < (float(_layout[2]) + 1.0) / frames)
            {
                vec4 color = texture(_tex2, vec2((texCoord.x - float(_layout[2]) / frames) * frames, texCoord.y));
                fragColor.rgb = mix(fragColor.rgb, color.rgb, color.a);
            }
            if (_textureNbr > 3 && texCoord.x > float(_layout[3]) / frames && texCoord.x < (float(_layout[3]) + 1.0) / frames)
            {
                vec4 color = texture(_tex3, vec2((texCoord.x - float(_layout[3]) / frames) * frames, texCoord.y));
                fragColor.rgb = mix(fragColor.rgb, color.rgb, color.a);
            }
            fragColor.a = 1.0;

            if (_gamma.x != 1.0)
                fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / _gamma.y));
        }
    )"};

} ShaderSources;

} // end of namespace

#endif // SPLASH_SHADERSOURCES_H
