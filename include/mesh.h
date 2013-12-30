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

#ifndef MESH_H
#define MESH_H

#include "config.h"

#include <memory>
#include <vector>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

#include "log.h"

namespace Splash {

class Mesh {
    public:
        typedef std::vector<unsigned char> SerializedObject;
        typedef OpenMesh::TriMesh_ArrayKernelT<OpenMesh::DefaultTraits> MeshContainer;

        /**
         * Constructor
         */
        Mesh();

        /**
         * Destructor
         */
        ~Mesh();

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
         * Get a serialized representation of the mesh
         */
        SerializedObject serialize() const;

        /**
         * Set the mesh from a serialized representation
         */
        bool deserialize(SerializedObject& obj);

    private:
        MeshContainer _mesh;
};

typedef std::shared_ptr<Mesh> MeshPtr;

} // end of namespace

#endif // MESH_H
