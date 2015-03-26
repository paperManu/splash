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

#ifndef SPLASH_GEOMETRY_H
#define SPLASH_GEOMETRY_H

#include "config.h"
#include "coretypes.h"
#include "basetypes.h"

#include <chrono>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace Splash {

class Geometry : public BaseObject
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
         * No copy constructor, but a move one
         */
        Geometry(const Geometry&) = delete;
        Geometry& operator=(const Geometry&) = delete;

        Geometry(Geometry&& g)
        {
            *this = std::move(g);
        }

        Geometry& operator=(Geometry&& g)
        {
            if (this != &g)
            {
                _mesh = std::move(g._mesh);
                _timestamp = g._timestamp;

                _vertexArray = std::move(g._vertexArray);
                _vertexCoords = g._vertexCoords;
                _texCoords = g._texCoords;
                _normals = g._normals;

                _normals = g._normals;
            }
            return *this;
        }

        /**
         * Activate the geometry for rendering
         */
        void activate();

        /**
         * Deactivate the geometry for rendering
         */
        void deactivate() const;

        /**
         * Get the number of vertices for this geometry
         */
        int getVerticesNumber() const {return _verticesNumber;}

        /**
         * Get the normals
         */
        GLuint getNormals() const {return _normals;}

        /**
         * Get the texture coords
         */
        GLuint getTextureCoords() const {return _texCoords;}

        /**
         * Get the vertex coords
         */
        GLuint getVertexCoords() const {return _vertexCoords;}

        /**
         * Try to link the given BaseObject to this
         */
        bool linkTo(BaseObjectPtr obj);

        /**
         * Get the coordinates of the closest vertex to the given point
         */
        float pickVertex(glm::dvec3 p, glm::dvec3& v);

        /**
         * Set the mesh for this object
         */
        void setMesh(MeshPtr mesh) {_mesh = mesh;}

        /**
         * Updates the object
         */
        void update();

    private:
        mutable std::mutex _mutex;

        MeshPtr _mesh;
        std::chrono::high_resolution_clock::time_point _timestamp;

        std::unordered_map<GLFWwindow*, GLuint> _vertexArray;
        GLuint _vertexCoords {0};
        GLuint _texCoords {0};
        GLuint _normals {0};

        int _verticesNumber {0};

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Geometry> GeometryPtr;

} // end of namespace

#endif // SPLASH_GEOMETRY_H
