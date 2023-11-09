#ifndef SPLASH_GFX_GEOMETRY_GFX_IMPL_H
#define SPLASH_GFX_GEOMETRY_GFX_IMPL_H

#include <chrono>
#include <map>
#include <memory>

#include "./core/constants.h"
#include "./graphics/geometry.h"
#include "./graphics/gpu_buffer.h"
#include "./graphics/renderer.h"

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
     * \param renderer Renderer, used to create the GpuBuffers
     * \param data Coordinates of the vertices, in uniform coordinates (4 floats per vertex)
     * \param numVerts Number of vertices to initialize
     */
    virtual void initVertices(Renderer* renderer, float* data, uint numVerts) = 0;

    /**
     * Allocate or init the chosen buffer
     * \param renderer Renderer, used to create GpuBuffers
     * \param bufferType Buffer type, one of Geometry::BufferType
     * \param componentsPerElement Component (float, int, ...) counts per element (vec2, ivec3, ...)
     */
    virtual void allocateOrInitBuffer(Renderer* renderer, Geometry::BufferType bufferType, uint componentsPerElement, std::vector<float>& dataVec) = 0;

    /**
     * Delete all vertex arrays
     */
    virtual void clearFromAllContexts() = 0;

    /**
     * Update the temporary buffers
     * \param renderer Renderer, used to create GpuBuffers
     * \param deserializedMesh Deserialized mesh, to be used to update the buffers
     */
    virtual void updateTemporaryBuffers(Renderer* renderer, Mesh::MeshContainer* deserializedMesh) = 0;

    /**
     * Update the object
     * \param buffersDirty In/out param, must be true if the buffers are considered dirty
     */
    virtual void update(bool& buffersDirty) = 0;

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
