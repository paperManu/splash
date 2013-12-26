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

#include <config.h>

#include <memory>
#include <string>
#include <vector>

#include "geometry.h"
#include "texture.h"

class Shader {
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
         * Activates this shader
         */
        void activate(const GeometryPtr geometry);

        /**
         * Set a shader source
         */
        void setSource(const std::string src, const ShaderType type);

        /**
         * Add a new texture to use
         */
        void setTexture(const TexturePtr texture, const GLuint textureUnit, const std::string name);

    private:
        GLuint _vertex;
        GLuint _geometry;
        GLuint _fragment;
        GLuint _program;
        bool _isLinked;
};

typedef std::shared_ptr<Shader> ShaderPtr;

#endif // SHADER_H
