/*
 * Copyright (C) 2023 Tarek Yasser
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

#ifndef SPLASH_GFX_OPENGL_GEOMETRY_GFX_IMPL_H
#define SPLASH_GFX_OPENGL_GEOMETRY_GFX_IMPL_H

#include <chrono>
#include <map>
#include <memory>

#include "./core/constants.h"
#include "./graphics/api/geometry_gfx_impl.h"

namespace Splash::gfx::opengl
{

class GpuBuffer;

class GeometryGfxImpl : public Splash::gfx::GeometryGfxImpl
{
  public:
    /**
     * Constructor
     */
    GeometryGfxImpl();

    /**
     * Destructor
     */
    virtual ~GeometryGfxImpl() override;

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
    virtual void activate() override final;

    /**
     * Deactivate for rendering
     */
    virtual void deactivate() override final;

    /**
     * Activate the geometry for gpgpu
     */
    virtual void activateAsSharedBuffer() override final;

    /**
     * Resize temporary buffers
     */
    virtual void resizeTempBuffers() override final;

    /**
     * Activate for transform feedback
     */
    virtual void activateForFeedback() override final;

    /**
     * Deactivate  for transform feedback
     */
    virtual void deactivateFeedback() override final;

    /**
     * Get the number of vertices for this geometry
     * \return Return the vertice count
     */
    virtual uint32_t getVerticesNumber() const override final;

    /**
     * Get whether the buffers are too small to hold the whole geometry
     * \return Return true if the buffers are too small
     */
    virtual bool buffersTooSmall() override final;

    /**
     * Get a copy of the given GPU buffer
     * \param type GPU buffer type
     * \return Return a vector containing a copy of the buffer
     */
    virtual std::vector<char> getGpuBufferAsVector(int typeId, bool forceAlternativeBuffers) override final;

    /**
     * Get the geometry as serialized
     * \return Return the serialized geometry
     */
    virtual Mesh::MeshContainer serialize(const std::string& name) override final;

    /**
     * Swap between temporary and alternative buffers
     */
    virtual void swapBuffers() override final;

    /**
     * Initialize the vertices to the given number and data
     * \param data Coordinates of the vertices, in uniform coordinates (4 floats per vertex)
     * \param numVerts Number of vertices to initialize
     */
    virtual void initVertices(float* data, uint numVerts) override final;

    /**
     * Allocate or init the chosen buffer
     * \param bufferType Buffer type, one of Geometry::BufferType
     * \param componentsPerElement Component (float, int, ...) counts per element (vec2, ivec3, ...)
     */
    virtual void allocateOrInitBuffer(Geometry::BufferType bufferType, uint componentsPerElement, std::vector<float>& dataVec) override final;

    /**
     * Delete all vertex arrays
     */
    virtual void clearFromAllContexts() override final;

    /**
     * Update the temporary buffers
     * \param deserializedMesh Deserialized mesh, to be used to update the buffers
     */
    virtual void updateTemporaryBuffers(Mesh::MeshContainer* deserializedMesh) override final;

    /**
     * Update the object
     * \param buffersDirty True if the buffers are considered dirty, forces update
     */
    virtual void update(bool buffersDirty) override final;

    /**
     * Activate alternative buffers for draw
     * \param isActive If true, use alternative buffers
     */
    virtual void useAlternativeBuffers(bool isActive) override final;

  private:
    std::map<GLFWwindow*, GLuint> _vertexArray;
    GLuint _feedbackQuery;

    std::array<std::shared_ptr<gfx::opengl::GpuBuffer>, 4> _glBuffers{};
    std::array<std::shared_ptr<gfx::opengl::GpuBuffer>, 4> _glAlternativeBuffers{}; // Alternative buffers used for rendering
    std::array<std::shared_ptr<gfx::opengl::GpuBuffer>, 4> _glTemporaryBuffers{};   // Temporary buffers used for feedback

    GLuint _feedbackMaxNbrPrimitives{0};

    GLuint _verticesNumber{0};
    GLuint _temporaryVerticesNumber{0};
    int _alternativeVerticesNumber{0};

    GLuint _temporaryBufferSize{0};
    GLuint _alternativeBufferSize{0};

    bool _useAlternativeBuffers{false};

    /**
     * Set the number of vertices
     * \param verticesNumber Number of vertices
     */
    void setVerticesNumber(uint32_t verticesNumber);

    /**
     * Allocate or init the chosen temporary buffer
     * \param bufferType Buffer type, one of Geometry::BufferType
     * \param componentsPerElement Component (float, int, ...) counts per element (vec2, ivec3, ...)
     */
    void allocateOrInitTemporaryBuffer(Geometry::BufferType bufferType, uint componentsPerElement, uint tempVerticesNumber, char* data);

    /**
     * Get whether any alternative buffer is missing
     * \return Return true if one of the alternative buffers is missing
     */
    virtual bool anyAlternativeBufferMissing() const override final;
};

} // namespace Splash::gfx::opengl

#endif
