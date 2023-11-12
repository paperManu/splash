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
 * @gpu_buffer.h
 * Implementation of GpuBuffer for OpenGL
 */

#ifndef SPLASH_OpenGL_GPU_BUFFER_H
#define SPLASH_OpenGL_GPU_BUFFER_H

#include "./graphics/api/gpu_buffer.h"

namespace Splash::gfx::opengl
{

class GpuBuffer final : public gfx::GpuBuffer
{
  public:
    /**
     * Constructor
     * \param elementSize Base element size
     * \param type Element type
     * \param usage Usage hint for the buffer
     * \param size Size of the buffer
     * \param data Data to upload to the buffer, if any
     */
    GpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data);

    /**
     * Destructor
     */
    virtual ~GpuBuffer() = default;

    /**
     * Copy constructor and operators
     */
    GpuBuffer(GpuBuffer& o)
    {
        _size = o._size;
        _baseSize = o._baseSize;
        _elementSize = o._elementSize;
        _type = o._type;
        _usage = o._usage;

        glGenBuffers(1, &_glId);
        glBindBuffer(GL_ARRAY_BUFFER, _glId);
        glBufferData(GL_ARRAY_BUFFER, _size * _elementSize * _baseSize, nullptr, _usage);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    GpuBuffer& operator=(const GpuBuffer&) = delete;
    GpuBuffer(GpuBuffer&&) = delete;
    GpuBuffer& operator=(GpuBuffer&&) = delete;

  protected:
    /**
     * Set the buffer to zero.
     * The buffer is considered to have already been allocated.
     */
    virtual void zeroBuffer() override final;

    /**
     * Alocate and initializes the buffer given its handle
     * \param bufferId Handle of the buffer
     * \param target Usage target
     * \param size Data size
     * \param data Pointer to the data
     * \param  usage Usage pattern
     */
    virtual void allocateBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) override final;

    /**
     * Copy data between buffers
     * \param fromId Handle of the source buffer
     * \param toId Handle of the target buffer
     * \param size Size (in bytes) of the data to copy
     */
    virtual void copyBetweenBuffers(GLuint fromId, GLuint toId, GLsizeiptr size) override final;

    /**
     * Read a buffer and return it
     * \param bufferId Handle of the buffer
     * \param bytesToRead Total bytes to read from the bufer
     * \return Return a std::vector<char> containing the data
     */
    virtual std::vector<char> readBufferFromGpu(GLuint bufferId, GLsizeiptr bytesToRead) override final;

    /**
     * Wrapper around glGetBufferParameteriv
     * \param bufferId Buffer handle
     * \param target Target usage
     * \param value Value
     * \param data Pre-allocated pointer to data, to hold the parameter
     */
    virtual void getBufferParameteriv(GLenum bufferId, GLenum target, GLenum value, GLint* data) override final;

    /**
     * Wrapper around glBufferSubData
     * \param bufferId Buffer handle
     * \param target Target usage
     * \param size Data size
     * \param data Pointer to the data to set the bufer to
     */
    virtual void setBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data) override final;
};

} // namespace Splash

#endif
