/*
 * Copyright (C) 2023 Splash authors
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
 * @geometry_gfx_impl.h
 * Base class for geometry API implementations
 */

#ifndef SPLASH_GFX_GEOMETRY_GFX_IMPL_H
#define SPLASH_GFX_GEOMETRY_GFX_IMPL_H

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "./core/constants.h"
#include "./mesh/mesh.h"

namespace Splash::gfx
{

class GeometryGfxImpl
{
  public:
    /**
     * Constructor
     */
    GeometryGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~GeometryGfxImpl() = default;

    /**
     * Other constructors
     */
    GeometryGfxImpl(const GeometryGfxImpl&) = delete;
    GeometryGfxImpl& operator=(const GeometryGfxImpl&) = delete;
    GeometryGfxImpl(GeometryGfxImpl&&) = delete;
    GeometryGfxImpl& operator=(const GeometryGfxImpl&&) = delete;

    /**
     * Activate for rendering
     */
    virtual void activate() = 0;

    /**
     * Deactivate for rendering
     */
    virtual void deactivate() = 0;

    /**
     * Activate the geometry for gpgpu
     */
    virtual void activateAsSharedBuffer() = 0;

    /**
     * Resize temporary buffers
     */
    virtual void resizeTempBuffers() = 0;

    /**
     * Activate for transform feedback
     */
    virtual void activateForFeedback() = 0;

    /**
     * Deactivate  for transform feedback
     */
    virtual void deactivateFeedback() = 0;

    /**
     * Get the number of vertices for this geometry
     * \return Return the vertice count
     */
    virtual uint32_t getVerticesNumber() const = 0;

    /**
     * Get whether the buffers are too small to hold the whole geometry
     * \return Return true if the buffers are too small
     */
    virtual bool buffersTooSmall() = 0;

    /**
     * Get a copy of the given GPU buffer
     * \param type GPU buffer type
     * \return Return a vector containing a copy of the buffer
     */
    virtual std::vector<char> getGpuBufferAsVector(int typeId, bool forceAlternativeBuffers = false) = 0;

    /**
     * Get the geometry as serialized
     * \return Return the serialized geometry
     */
    virtual Mesh::MeshContainer serialize(const std::string& name) = 0;

    /**
     * Swap between temporary and alternative buffers
     */
    virtual void swapBuffers() = 0;

    /**
     * Initialize the vertices to the given number and data
     * \param data Coordinates of the vertices, in uniform coordinates (4 floats per vertex)
     * \param numVerts Number of vertices to initialize
     */
    virtual void initVertices(float* data, uint numVerts) = 0;

    /**
     * Allocate or init the chosen buffer
     * \param bufferIndex Index of the buffer to allocate
     * \param componentsPerElement Component (float, int, ...) counts per element (vec2, ivec3, ...)
     * \param dataVec Vector holding the data to initialize the buffer with
     */
    virtual void allocateOrInitBuffer(uint32_t bufferIndex, uint componentsPerElement, std::vector<float>& dataVec) = 0;

    /**
     * Delete all vertex arrays
     */
    virtual void clearFromAllContexts() = 0;

    /**
     * Update the temporary buffers
     * \param deserializedMesh Deserialized mesh, to be used to update the buffers
     */
    virtual void updateTemporaryBuffers(Mesh::MeshContainer* deserializedMesh) = 0;

    /**
     * Update the object
     * \param buffersDirty True if the buffers are considered dirty, forces update
     */
    virtual void update(bool buffersDirty) = 0;

    /**
     * Activate alternative buffers for draw
     * \param isActive If true, use alternative buffers
     */
    virtual void useAlternativeBuffers(bool isActive) = 0;

  private:
    /**
     * Get whether any alternative buffer is missing
     * \return Return true if one of the alternative buffers is missing
     */
    virtual bool anyAlternativeBufferMissing() const = 0;
};

} // namespace Splash::gfx

#endif
