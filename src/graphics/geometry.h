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
 * @texture.h
 * The Geometry base class
 */

#ifndef SPLASH_GEOMETRY_H
#define SPLASH_GEOMETRY_H

#include <array>
#include <chrono>
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <utility>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/buffer_object.h"
#include "./graphics/gpu_buffer.h"
#include "./graphics/renderer.h"
#include "./mesh/mesh.h"

namespace Splash
{

namespace gfx
{
class GeometryGfxImpl;
}

class Geometry : public BufferObject
{
  public:
    enum class BufferType : uint8_t
    {
        Vertex = 0,
        TexCoords,
        Normal,
        Annexe
    };

  public:
    /**
     * Constructor
     * \param root Root object
     */
    Geometry(RootObject* root);

    /**
     * Destructor
     */
    ~Geometry() override;

    /**
     * No copy constructor, but a move one
     */
    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;

    /**
     * Activate the geometry for rendering
     */
    void activate();

    /**
     * Activate the geometry for gpgpu
     */
    void activateAsSharedBuffer();

    /**
     * Activate the geometry for feedback into the alternative buffers
     */
    void activateForFeedback();

    /**
     * Deactivate the geometry for rendering
     */
    void deactivate() const;

    /**
     * Deactivate for feedback
     */
    void deactivateFeedback();

    std::vector<char> getGpuBufferAsVector(Geometry::BufferType type, bool forceAlternativeBuffer = false) const;

    /**
     * Get the number of vertices for this geometry
     * \return Return the vertice count
     */
    uint getVerticesNumber() const;

    /**
     * Get the geometry as serialized
     * \return Return the serialized geometry
     */
    SerializedObject serialize() const override;

    /**
     * Deserialize the geometry
     * \param obj Serialized object
     * \return Return true if all went well
     */
    bool deserialize(SerializedObject&& obj) override;

    /**
     * Get the timestamp
     * \return Return the timestamp
     */
    virtual int64_t getTimestamp() const final { return _mesh ? _mesh->getTimestamp() : 0; }

    /**
     * Get whether the alternative buffers have been resized during the last feedback call
     * \return Return true if the buffers have been resized
     */
    bool hasBeenResized() { return _buffersResized; }

    /**
     * Get the coordinates of the closest vertex to the given point
     * \param p Point around which to look
     * \param v If detected, vertex coordinates
     * \return Return the distance from p to v
     */
    float pickVertex(glm::dvec3 p, glm::dvec3& v);

    /**
     * Set the mesh for this object
     * \param mesh Mesh
     */
    void setMesh(const std::shared_ptr<Mesh>& mesh)
    {
        if (mesh)
            _mesh = mesh;
    }

    /**
     * Swap between temporary and alternative buffers
     */
    void swapBuffers();

    /**
     * Updates the object
     */
    void update() override;

    /**
     * Activate alternative buffers for draw
     * \param isActive If true, use alternative buffers
     */
    void useAlternativeBuffers(bool isActive);

  protected:
    /**
     * Try to link the given GraphObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkIt(const std::shared_ptr<GraphObject>& obj) override;

  private:
    bool _onMasterScene{false};

    std::unique_ptr<gfx::GeometryGfxImpl> _gfxImpl;

    std::shared_ptr<Mesh> _mesh;
    std::unique_ptr<Mesh::MeshContainer> _deserializedMesh{nullptr};

    bool _buffersDirty{false};
    bool _buffersResized{false}; // Holds whether the alternative buffers have been resized in the previous feedback

    /**
     * Initialization
     */
    void init();

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();

    void updateBuffers();
    void updateTemporaryBuffers();
};

} // namespace Splash

#endif // SPLASH_GEOMETRY_H
