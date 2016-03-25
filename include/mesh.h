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
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
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

namespace Splash {

class Mesh : public BufferObject
{
    public:
        /**
         * Constructor
         */
        Mesh();
        Mesh(bool linkedToWorld); //< This constructor is used if the object is linked to a World counterpart

        /**
         * Destructor
         */
        virtual ~Mesh();

        /**
         * No copy constructor, but a copy operator
         */
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh& operator=(Mesh&&) = default;

        /**
         * Compare meshes based on their timestamps
         */
        bool operator==(Mesh& otherMesh) const;

        /**
         * Get a 1D vector of all points in the mesh, in normalized coordinates
         */
        virtual std::vector<float> getVertCoords() const;

        /**
         * Get a 1D vector of the UV coordinates for all points, same order as getVertCoords()
         */
        virtual std::vector<float> getUVCoords() const;

        /**
         * Get a 1D vector of the normal at each vertex, same order as getVertCoords(), normalized coords
         */
        virtual std::vector<float> getNormals() const;

        /**
         * Get a 1D vector of the annexe at each vertex, same order as getVertCoords()
         */
        virtual std::vector<float> getAnnexe() const;

        /**
         * Read / update the mesh
         */
        virtual bool read(const std::string& filename);

        /**
         * Get a serialized representation of the mesh
         */
        std::shared_ptr<SerializedObject> serialize() const;

        /**
         * Set the mesh from a serialized representation
         */
        bool deserialize(std::shared_ptr<SerializedObject> obj);

        /**
         * Update the content of the mesh
         */
        virtual void update();

    protected:
        bool _linkedToWorldObject {false};

        struct MeshContainer
        {
            std::vector<glm::vec4> vertices;
            std::vector<glm::vec2> uvs;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec4> annexe;
        };

        std::string _filepath {};
        MeshContainer _mesh;
        MeshContainer _bufferMesh;
        bool _meshUpdated {false};
        bool _benchmark {false};
        int _planeSubdivisions {0};

    private:
        void init();

        /**
         * Create a plane mesh, subdivided according to the parameter
         */
        void createDefaultMesh(int subdiv = 8);
        
        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

} // end of namespace

#endif // SPLASH_MESH_H
