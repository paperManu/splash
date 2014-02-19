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

#ifndef SHADERSOURCES_H
#define SHADERSOURCES_H

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
        uniform vec3 _scale;
        smooth out vec4 position;
        smooth out vec2 texCoord;
        smooth out vec3 normal;

        void main(void)
        {
            position = _modelViewProjectionMatrix * vec4(_vertex.x * _scale.x, _vertex.y * _scale.y, _vertex.z * _scale.z, 1.f);
            gl_Position = position;
            normal = (_normalMatrix * vec4(_normal, 0.0)).xyz;
            texCoord = _texcoord;
        }
    )"};

    /**
     * Default fragment shader
     */
    const std::string FRAGMENT_SHADER_TEXTURE {R"(
        #version 330 core

        #define PI 3.14159265359

        uniform sampler2D _tex0;
        uniform sampler2D _tex1;
        uniform int _sideness;
        uniform int _textureNbr;
        uniform int _texBlendingMap;
        uniform float _blendWidth;
        in vec4 position;
        in vec2 texCoord;
        in vec3 normal;
        out vec4 fragColor;

        void main(void)
        {
            if ((dot(normal, vec3(0.0, 0.0, 1.0)) >= PI && _sideness == 1) || (dot(normal, vec3(0.0, 0.0, 1.0)) <= -PI && _sideness == 2))
                discard;

            if (_texBlendingMap == 0)
            {
                if (_textureNbr > 0)
                    fragColor = texture(_tex0, texCoord);
                if (_textureNbr > 1)
                {
                    vec4 color = texture(_tex1, texCoord);
                    fragColor.rgb = fragColor.rgb * (1.0 - color.a) + color.rgb * color.a;
                }
            }
            else
            {
                float blendFactor = texture(_tex1, texCoord).r * 65535.f;
                if (blendFactor == 0.f)
                    blendFactor = 1.f;
                else if (_blendWidth > 0.f)
                {
                    vec2 screenPos = vec2((position.x / position.w + 1.f) / 2.f, (position.y / position.w + 1.f) / 2.f);
                    float distX = min(screenPos.x, 1.f - screenPos.x);
                    float distY = min(screenPos.y, 1.f - screenPos.y);
                    float dist = min(1.f, min(distX, distY) / _blendWidth);
                    blendFactor = 256.f * dist / blendFactor;
                }
                else
                {
                    blendFactor = 256.f / blendFactor;
                }
                vec4 color = texture(_tex0, texCoord);
                fragColor.rgb = color.rgb * min(1.f, blendFactor);
                fragColor.a = color.a;
            }
        }
    )"};

    /**
     * Single color fragment shader
     */
    const std::string FRAGMENT_SHADER_COLOR {R"(
        #version 330 core

        #define PI 3.14159265359

        uniform int _sideness;
        uniform vec4 _color;
        in vec3 normal;
        out vec4 fragColor;

        void main(void)
        {
            if ((dot(normal, vec3(0.0, 0.0, 1.0)) >= PI && _sideness == 1) || (dot(normal, vec3(0.0, 0.0, 1.0)) <= -PI && _sideness == 2))
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

        uniform int _sideness;
        in vec2 texCoord;
        in vec3 normal;
        out vec4 fragColor;

        void main(void)
        {
            if ((dot(normal, vec3(0.0, 0.0, 1.0)) >= PI && _sideness == 1) || (dot(normal, vec3(0.0, 0.0, 1.0)) <= -PI && _sideness == 2))
                discard;

            float U = texCoord.x * 65536.f;
            float V = texCoord.y * 65536.f;

            fragColor.rg = vec2(floor(U / 256.f) / 256.f, (U - floor(U / 256.f) * 256.f) / 256.f);
            fragColor.ba = vec2(floor(V / 256.f) / 256.f, (V - floor(V / 256.f) * 256.f) / 256.f);
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
        uniform vec3 _scale;

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
        } vertexOut;

        void main()
        {
            vec4 v = _modelViewProjectionMatrix * vec4(vertexIn[0].vertex.xyz * _scale.xyz, 1.f);
            gl_Position = v;
            vertexOut.texcoord = vertexIn[0].texcoord;
            vertexOut.normal = (_normalMatrix * vec4(vertexIn[0].normal, 0.0)).xyz;
            vertexOut.bcoord = vec3(1.0, 0.0, 0.0);
            EmitVertex();

            v = _modelViewProjectionMatrix * vec4(vertexIn[1].vertex.xyz * _scale.xyz, 1.f);
            gl_Position = v;
            vertexOut.texcoord = vertexIn[1].texcoord;
            vertexOut.normal = (_normalMatrix * vec4(vertexIn[1].normal, 0.0)).xyz;
            vertexOut.bcoord = vec3(0.0, 1.0, 0.0);
            EmitVertex();

            v = _modelViewProjectionMatrix * vec4(vertexIn[2].vertex.xyz * _scale.xyz, 1.f);
            gl_Position = v;
            vertexOut.texcoord = vertexIn[2].texcoord;
            vertexOut.normal = (_normalMatrix * vec4(vertexIn[2].normal, 0.0)).xyz;
            vertexOut.bcoord = vec3(0.0, 0.0, 1.0);
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
        } vertexIn;

        uniform int _sideness;
        out vec4 fragColor;

        void main(void)
        {
            vec3 normal = vertexIn.normal;
            if ((dot(normal, vec3(0.0, 0.0, 1.0)) >= PI && _sideness == 1) || (dot(normal, vec3(0.0, 0.0, 1.0)) <= -PI && _sideness == 2))
                discard;

            vec3 b = vertexIn.bcoord;
            float minDist = min(min(b[0], b[1]), b[2]);
            fragColor.rgba = mix(vec4(1.0), vec4(0.0, 0.0, 0.0, 1.0), (minDist - 0.025) / 0.0125);
        }
    )"};

    /**
     * Rendering of the output windows
     */
    const std::string VERTEX_SHADER_WINDOW {R"(
        #version 330 core

        layout(location = 0) in vec4 _vertex;
        layout(location = 1) in vec2 _texcoord;
        layout(location = 2) in vec3 _normal;
        uniform mat4 _modelViewProjectionMatrix;
        uniform mat4 _normalMatrix;
        uniform vec3 _scale;
        smooth out vec4 position;
        smooth out vec2 texCoord;
        smooth out vec3 normal;

        void main(void)
        {
            position = _modelViewProjectionMatrix * vec4(_vertex.x * _scale.x, _vertex.y * _scale.y, _vertex.z * _scale.z, 1.f);
            gl_Position = position;
            normal = (_normalMatrix * vec4(_normal, 0.0)).xyz;
            texCoord = _texcoord;
        }
    )"};

    const std::string FRAGMENT_SHADER_WINDOW {R"(
        #version 330 core

        #define PI 3.14159265359

        uniform sampler2D _tex0;
        uniform sampler2D _tex1;
        uniform sampler2D _tex2;
        uniform int _sideness;
        uniform int _textureNbr;
        uniform int _overlap;
        in vec4 position;
        in vec2 texCoord;
        in vec3 normal;
        out vec4 fragColor;

        void main(void)
        {
            if (_textureNbr == 1)
            {
                fragColor = texture(_tex0, texCoord);
            }
            else if (_overlap == 1)
            {
                fragColor = texture(_tex0, texCoord);
                vec4 color = texture(_tex1, texCoord);
                fragColor.rgb = fragColor.rgb * (1.0 - color.a) + color.rgb * color.a;
            }
            else
            {
                if (_textureNbr == 2)
                {
                    if (texCoord.x <= 0.5)
                        fragColor = texture(_tex0, vec2(texCoord.x * 2.0, texCoord.y));
                    else
                        fragColor = texture(_tex1, vec2((texCoord.x - 0.5) * 2.0, texCoord.y));
                }
                else if (_textureNbr == 3)
                {
                    if (texCoord.x <= 1.0 / 3.0)
                        fragColor = texture(_tex0, vec2(texCoord.x * 3.0, texCoord.y));
                    else if (texCoord.x <= 2.0 / 3.0)
                        fragColor = texture(_tex1, vec2((texCoord.x - 1.0 / 3.0) * 3.0, texCoord.y));
                    else
                        fragColor = texture(_tex2, vec2((texCoord.x - 2.0 / 3.0) * 3.0, texCoord.y));
                }
            }
        }
    )"};

} ShaderSources;

} // end of namespace

#endif // SHADERSOURCES_H
