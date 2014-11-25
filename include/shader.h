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

#ifndef SPLASH_SHADER_H
#define SPLASH_SHADER_H

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#include "config.h"
#include "coretypes.h"

#include <atomic>
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
            uv,
            wireframe,
            window
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
        Shader& operator=(const Shader&) = delete;

        Shader& operator=(Shader&& s)
        {
            if (this != &s)
            {
                _shaders = s._shaders;
                _program = s._program;
                _isLinked = s._isLinked;
                _uniforms = s._uniforms;
                _uniformsToUpdate = s._uniformsToUpdate;

                _fill = s._fill;
                _sideness = s._sideness;
                _useBlendingMap = s._useBlendingMap;
                _blendWidth = s._blendWidth;
                _blackLevel = s._blackLevel;
                _brightness = s._brightness;
                _color = s._color;
                _scale = s._scale;
                _textureOverlap = s._textureOverlap;
                _layout = s._layout;
            }
            return *this;
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
         * Set the model view and projection matrices
         */
        void setModelViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp);

        /**
         * Set the currently queued uniforms updates
         */
        void updateUniforms();

    private:
        mutable std::mutex _mutex;
        std::atomic_bool _activated {false};

        std::map<ShaderType, GLuint> _shaders;
        std::map<ShaderType, std::string> _shadersSource;
        GLuint _program {0};
        bool _isLinked = {false};
        std::map<std::string, std::pair<Values, GLint>> _uniforms;
        std::vector<std::string> _uniformsToUpdate;
        std::vector<TexturePtr> _textures; // Currently used textures

        // Rendering parameters
        Fill _fill {texture};
        Sideness _sideness {doubleSided};
        int _useBlendingMap {0};
        float _blendWidth {0.05f};
        float _blackLevel {0.f};
        float _brightness {1.f};
        glm::dvec4 _color {0.0, 1.0, 0.0, 1.0};
        glm::dvec3 _scale {1.0, 1.0, 1.0};
        bool _textureOverlap {true};
        std::vector<int> _layout {0, 0, 0, 0};

        /**
         * Compile the shader program
         */
        void compileProgram();

        /**
         * Link the shader program
         */
        bool linkProgram();

        /**
         * Parses the shader to find uniforms
         */
        void parseUniforms(const std::string& src);

        /**
         * Get a string expression of the shader type, used for logging
         */
        std::string stringFromShaderType(ShaderType type);

        /**
         * Replace a shader with an empty one
         */
        void resetShader(ShaderType type);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Shader> ShaderPtr;

} // end of namespace

#endif // SPLASH_SHADER_H
