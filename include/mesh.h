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
 * @mesh.h
 * The Mesh class
 */

#ifndef SPLASH_MESH_H
#define SPLASH_MESH_H

#include <chrono>
#include <memory>
#include <mutex>
#include <vector>
#include <glm/glm.hpp>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "coretypes.h"

namespace Splash {

class Mesh : public BufferObject
{
    public:
        /**
         * Constructor
         */
        Mesh();

        /**
         * Destructor
         */
        virtual ~Mesh();

        /**
         * No copy, but some move constructors
         */
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        /**
         * Compare meshes based on their timestamps
         */
        bool operator==(Mesh& otherMesh) const;

        /**
         * Get a 1D vector of all points in the mesh, in normalized coordinates
         */
        std::vector<float> getVertCoords() const;

        /**
         * Get a 1D vector of the UV coordinates for all points, same order as getVertCoords()
         */
        std::vector<float> getUVCoords() const;

        /**
         * Get a 1D vector of the normal at each vertex, same order as getVertCoords(), normalized coords
         */
        std::vector<float> getNormals() const;

        /**
         * Get the timestamp for the current mesh
         */
        std::chrono::high_resolution_clock::time_point getTimestamp() const {return _timestamp;}

        /**
         * Read / update the mesh
         */
        virtual bool read(const std::string& filename);

        /**
         * Get a serialized representation of the mesh
         */
        std::unique_ptr<SerializedObject> serialize() const;

        /**
         * Set the mesh from a serialized representation
         */
        bool deserialize(std::unique_ptr<SerializedObject> obj);

        /**
         * Update the content of the mesh
         */
        virtual void update();

    protected:
        struct MeshContainer
        {
            std::vector<glm::vec4> vertices;
            std::vector<glm::vec2> uvs;
            std::vector<glm::vec3> normals;
        };

        MeshContainer _mesh;
        MeshContainer _bufferMesh;
        bool _meshUpdated {false};
        bool _benchmark {false};

        void createDefaultMesh(); //< As indicated: creates a default mesh (a plane)
        
        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Mesh> MeshPtr;

} // end of namespace

#endif // SPLASH_MESH_H
