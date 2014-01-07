/*
 * Copyright (C) 2013 Emmanuel Durand
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
 * @shader.h
 * The Shader class
 */

#ifndef SHADER_H
#define SHADER_H

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#include <config.h>

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <GLFW/glfw3.h>

#include "coretypes.h"
#include "geometry.h"
#include "texture.h"

namespace Splash {

class Shader : public BaseObject
{
    public:
        enum ShaderType
        {
            vertex = 0,
            geometry,
            fragment
        };

        /**
         * Constructor
         */
        Shader();

        /**
         * Destructor
         */
        ~Shader();

        /**
         * No copy constructor, but a move one
         */
        Shader(const Shader&) = delete;
        Shader(Shader&& s)
        {
            _shaders = s._shaders;
            _program = s._program;
            _isLinked = s._isLinked;
            _geometry = s._geometry;
        }

        /**
         * Activate this shader
         */
        void activate(const GeometryPtr geometry);

        /**
         * Deactivate this shader
         */
        void deactivate();

        /**
         * Set a shader source
         */
        void setSource(const std::string& src, const ShaderType type);

        /**
         * Set a shader source from file
         */
        void setSourceFromFile(const std::string filename, const ShaderType type);

        /**
         * Add a new texture to use
         */
        void setTexture(const TexturePtr texture, const GLuint textureUnit, const std::string& name);

        /**
         * Set the view projection matrix
         */
        void setViewProjectionMatrix(const glm::mat4& mvp);

    private:
        std::map<ShaderType, GLuint> _shaders;
        GLuint _program;
        bool _isLinked = {false};
        GeometryPtr _geometry;
        GLint _locationMVP {0};

        void compileProgram();
        bool linkProgram();

    public:
        const std::string DEFAULT_VERTEX_SHADER {R"(
            #version 330 core

            in vec4 _vertex;
            in vec2 _texcoord;
            uniform mat4 _viewProjectionMatrix;
            smooth out vec2 finalTexCoord;

            void main(void)
            {
                gl_Position.xyz = (_viewProjectionMatrix * _vertex).xyz;
                finalTexCoord = _texcoord;
            }
        )"};

        const std::string DEFAULT_FRAGMENT_SHADER {R"(
            #version 330 core

            uniform sampler2D _tex0;
            in vec2 finalTexCoord;
            out vec4 fragColor;

            void main(void)
            {
                fragColor = texture(_tex0, finalTexCoord);
                fragColor += vec4(0.5f, 0.f, 0.f, 1.f);
            }
        )"};

};

typedef std::shared_ptr<Shader> ShaderPtr;

} // end of namespace

#endif // SHADER_H
