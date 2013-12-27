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
 * @texture.h
 * The Geometry base class
 */

#ifndef GEOMETRY_H
#define GEOMETRY_H

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#include <config.h>

#include <GLFW/glfw3.h>

#include "log.h"
#include "mesh.h"

namespace Splash {

class Geometry
{
    public:
        /**
         * Constructor
         */
        Geometry();

        /**
         * Destructor
         */
        ~Geometry();

        /**
         * Get the texture coords
         */
        GLuint getTextureCoords() const {return _texCoords;}

        /**
         * Get the vertex coords
         */
        GLuint getVertexCoords() const {return _vertexCoords;}

    private:
        MeshPtr _mesh;

        GLuint _vertexCoords = {0};
        GLuint _texCoords = {0};
};

typedef std::shared_ptr<Geometry> GeometryPtr;

} // end of namespace

#endif // GEOMETRY_H
