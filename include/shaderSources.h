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
        smooth out vec2 texCoord;
        smooth out vec3 normal;

        void main(void)
        {
            gl_Position = _modelViewProjectionMatrix * vec4(_vertex.x * _scale.x, _vertex.y * _scale.y, _vertex.z * _scale.z, 1.f);
            normal = (_normalMatrix * vec4(_normal, 0.0)).xyz;
            texCoord = _texcoord;
        }
    )"};

    /**
     * Default fragment shader
     */
    const std::string FRAGMENT_SHADER_TEXTURE {R"(
        #version 330 core

        uniform sampler2D _tex0;
        uniform sampler2D _tex1;
        uniform int _sideness;
        uniform int _textureNbr;
        in vec2 texCoord;
        in vec3 normal;
        out vec4 fragColor;

        void main(void)
        {
            if ((dot(normal, vec3(0.0, 0.0, 1.0)) >= 0.0 && _sideness == 1) || (dot(normal, vec3(0.0, 0.0, 1.0)) <= 0.0 && _sideness == 2))
                discard;

            if (_textureNbr > 0)
                fragColor = texture(_tex0, texCoord);
            if (_textureNbr > 1)
            {
                vec4 color = texture(_tex1, texCoord);
                fragColor.rgb = fragColor.rgb * (1.0 - color.a) + color.rgb * color.a;
            }
        }
    )"};

    /**
     * Single color fragment shader
     */
    const std::string FRAGMENT_SHADER_COLOR {R"(
        #version 330 core

        uniform int _sideness;
        uniform vec4 _color;
        in vec3 normal;
        out vec4 fragColor;

        void main(void)
        {
            if ((dot(normal, vec3(0.0, 0.0, 1.0)) >= 0.0 && _sideness == 1) || (dot(normal, vec3(0.0, 0.0, 1.0)) <= 0.0 && _sideness == 2))
                discard;

            fragColor = _color;
        }
    )"};

    /**
     * UV drawing fragment shader
     */
    const std::string FRAGMENT_SHADER_UV {R"(
        #version 330 core

        uniform int _sideness;
        in vec2 texCoord;
        in vec3 normal;
        out vec4 fragColor;

        void main(void)
        {
            if ((dot(normal, vec3(0.0, 0.0, 1.0)) >= 0.0 && _sideness == 1) || (dot(normal, vec3(0.0, 0.0, 1.0)) <= 0.0 && _sideness == 2))
                discard;

            fragColor.rg = vec2(texCoord.x, texCoord.y);
            fragColor.ba = vec2(0.f, 1.f);
        }
    )"};
} ShaderSources;

} // end of namespace

#endif // SHADERSOURCES_H
