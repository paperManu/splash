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
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <vector>

#include "./config.h"

#include "./core/attribute.h"
#include "./core/buffer_object.h"
#include "./core/coretypes.h"

namespace Splash
{

class Mesh : public BufferObject
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Mesh(RootObject* root);

    /**
     * \brief Destructor
     */
    virtual ~Mesh() override;

    /**
     * No copy constructor, but a copy operator
     */
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh& operator=(Mesh&&) = default;

    /**
     * \brief Compare meshes based on their timestamps
     * \param otherMesh Mesh to compare to
     * \return Return true if the meshes share the same timestamp
     */
    bool operator==(Mesh& otherMesh) const;

    /**
     * \brief Get a 1D vector of all points of the mesh, in normalized coordinates
     * \return Return a vector representing all points of the mesh
     */
    virtual std::vector<float> getVertCoords() const;

    /**
     * \brief Get a 1D vector of the UV coordinates for all points, same order as getVertCoords()
     * \return Return a vector representing the UV coordinates
     */
    virtual std::vector<float> getUVCoords() const;

    /**
     * \brief Get a 1D vector of the normal at each vertex, same order as getVertCoords(), normalized coords
     * \return Return a vector representing the normals
     */
    virtual std::vector<float> getNormals() const;

    /**
     * \brief Get a 1D vector of the annexe at each vertex, same order as getVertCoords()
     * \return Return a vector representing the annexes
     */
    virtual std::vector<float> getAnnexe() const;

    /**
     * \brief Read / update the mesh
     * \param filename File to load from
     * \return Return true if all went well
     */
    virtual bool read(const std::string& filename);

    /**
     * \brief Get a serialized representation of the mesh
     * \return Return a serialized object
     */
    std::shared_ptr<SerializedObject> serialize() const override;

    /**
     * \brief Set the mesh from a serialized representation
     * \param obj Serialized object
     * \return Return true if all went well
     */
    bool deserialize(const std::shared_ptr<SerializedObject>& obj) override;

    /**
     * \brief Update the content of the mesh
     */
    virtual void update();

  protected:
    struct MeshContainer
    {
        std::vector<glm::vec4> vertices;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec4> annexe;
    };

    std::string _filepath{};
    MeshContainer _mesh;
    MeshContainer _bufferMesh;
    bool _meshUpdated{false};
    bool _benchmark{false};
    int _planeSubdivisions{0};

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();

  private:
    void init();

    /**
     * \brief Create a plane mesh, subdivided according to the parameter
     * \param subdiv Number of subdivision for the plane
     */
    void createDefaultMesh(int subdiv = 0);
};

} // end of namespace

#endif // SPLASH_MESH_H
