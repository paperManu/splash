/*
 * Copyright (C) 2013 Splash authors
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

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/buffer_object.h"

namespace Splash
{

class Mesh : public BufferObject
{
  public:
    struct MeshContainer
    {
        std::string name;
        std::vector<glm::vec4> vertices;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec4> normals;
        std::vector<glm::vec4> annexe;
    };

  public:
    /**
     * Constructor
     * \param root Root object
     * \param meshContainer Mesh container to initialize the Mesh from
     */
    explicit Mesh(RootObject* root, MeshContainer meshContainer = MeshContainer());

    /**
     * Destructor
     */
    ~Mesh() override = default;

    /**
     * Constructors/operators
     */
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    /**
     * Compare meshes based on their timestamps
     * \param otherMesh Mesh to compare to
     * \return Return true if the meshes share the same timestamp
     */
    bool operator==(Mesh& otherMesh) const;

    /**
     * Get a vector of all points in the mesh, in normalized coordinates
     * \return Return a vector representing all points of the mesh
     */
    virtual std::vector<glm::vec4> getVertCoords() const;

    /**
     * Get a flattened vector of all points of the mesh, in normalized coordinates
     * \return Return a flattened vector representing all points of the mesh
     */
    virtual std::vector<float> getVertCoordsFlat() const;

    /**
     * Get a vector of the UV coordinates for all points, same order as getVertCoords()
     * \return Return a vector representing the UV coordinates
     */
    virtual std::vector<glm::vec2> getUVCoords() const;

    /**
     * Get a flattened vector of the UV coordinates for all points, same order as getVertCoordsFlat()
     * \return Return a flattened vector representing the UV coordinates
     */
    virtual std::vector<float> getUVCoordsFlat() const;

    /**
     * Get a vector of the normal at each vertex, same order as getVertCoords(), in normalized coordinates
     * \return Return a vector representing the normals
     */
    virtual std::vector<glm::vec4> getNormals() const;

    /**
     * Get a flattened vector of the normal at each vertex, same order as getVertCoordsFlat(), in normalized coordinates
     * \return Return a flattened vector representing the normals
     */
    virtual std::vector<float> getNormalsFlat() const;

    /**
     * Get a vector of the annexe at each vertex, same order as getVertCoords()
     * \return Return a vector representing the annexes
     */
    virtual std::vector<glm::vec4> getAnnexe() const;

    /**
     * Get a flattened vector of the annexe at each vertex, same order as getVertCoords()
     * \return Return a flattened vector representing the annexes
     */
    virtual std::vector<float> getAnnexeFlat() const;

    /**
     * Read / update the mesh
     * \param filename File to load from
     * \return Return true if all went well
     */
    virtual bool read(const std::string& filename);

    /**
     * Get a serialized representation of the mesh
     * \return Return a serialized object
     */
    SerializedObject serialize() const override;

    /**
     * Set the mesh from a serialized representation
     * \param obj Serialized object
     * \return Return true if all went well
     */
    bool deserialize(SerializedObject&& obj) override;

    /**
     * Update the content of the mesh
     */
    virtual void update() override;

  protected:
    std::string _filepath{};
    MeshContainer _mesh;
    MeshContainer _bufferMesh;
    bool _meshUpdated{false};
    bool _benchmark{false};
    int _planeSubdivisions{0};

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();

  private:
    void init();

    /**
     * Create a plane mesh, subdivided according to the parameter
     * \param subdiv Number of subdivision for the plane
     */
    void createDefaultMesh(int subdiv = 0);
};

} // namespace Splash

#endif // SPLASH_MESH_H
