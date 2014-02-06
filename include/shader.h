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

#include "config.h"
#include "coretypes.h"

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <GLFW/glfw3.h>

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

        enum Sideness
        {
            doubleSided = 0,
            singleSided,
            inverted
        };

        enum Fill
        {
            texture = 0,
            color,
            uv
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
            _locationMVP = s._locationMVP;
            _locationNormalMatrix = s._locationNormalMatrix;
            _locationSide = s._locationSide;
            _locationTextureNbr = s._locationTextureNbr;

            _sideness = s._sideness;
            _textureNbr = s._textureNbr;
        }

        /**
         * Activate this shader
         */
        void activate();

        /**
         * Deactivate this shader
         */
        void deactivate();

        /**
         * Set the sideness of the object
         */
        void setSideness(const Sideness side);
        Sideness getSideness() const {return _sideness;}

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
        void setModelViewProjectionMatrix(const glm::mat4& mvp);

    private:
        std::map<ShaderType, GLuint> _shaders;
        GLuint _program {0};
        bool _isLinked = {false};
        GLint _locationMVP {0};
        GLint _locationNormalMatrix {0};
        GLint _locationSide {0};
        GLint _locationTextureNbr {0};
        GLint _locationColor {0};
        GLint _locationScale {0};

        // Rendering parameters
        Fill _fill {texture};
        Sideness _sideness {doubleSided};
        int _textureNbr {0};
        glm::vec4 _color {0.0, 1.0, 0.0, 1.0};
        glm::vec3 _scale {1.0, 1.0, 1.0};

        /**
         * Compile the shader program
         */
        void compileProgram();

        /**
         * Link the shader program
         */
        bool linkProgram();

        /**
         * Get a string expression of the shader type, used for logging
         */
        std::string stringFromShaderType(ShaderType type);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Shader> ShaderPtr;

} // end of namespace

#endif // SHADER_H
