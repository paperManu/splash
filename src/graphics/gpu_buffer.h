/*
 * Copyright (C) 2015 Emmanuel Durand
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
 * Class handling OpenGL buffers
 */

#ifndef SPLASH_GPU_BUFFER_H
#define SPLASH_GPU_BUFFER_H

#include <chrono>
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <vector>

#include "./core/attribute.h"
#include "./core/constants.h"
#include "./mesh/mesh.h"

namespace Splash
{

class GpuBuffer
{
  public:
    /**
     * Constructor
     */
    GpuBuffer() = default;

    /**
     * Destructor
     */
    virtual ~GpuBuffer();

    GpuBuffer(GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;
    GpuBuffer(GpuBuffer&&) = delete;
    GpuBuffer& operator=(GpuBuffer&&) = delete;

    /**
     * Bool pattern
     */
    explicit operator bool() const
    {
        if (_size == 0 || _baseSize == 0 || _elementSize == 0 || _glId == 0)
            return false;
        else
            return true;
    }

    /**
     * Fill the buffer with 0
     */
    void clear();

    /**
     * Copy this buffer to a new one
     * \return Return a deep copy of this buffer
     */
    std::shared_ptr<GpuBuffer> copyBuffer() const;

    /**
     * Get the GL id
     * \return Return the GL id
     */
    inline GLuint getId() const { return _glId; }

    /**
     * Get the entry count in the buffer
     * \return Return the entry count
     */
    inline size_t getSize() const { return _size; }

    /**
     * Get the size in bytes of the buffer
     * \return Return the size in bytes for the buffer
     */
    inline size_t getMemorySize() const { return _size * _baseSize * _elementSize; }

    /**
     * Get the content of the buffer
     * One can specify the vertex number to get, with care!
     */
    std::vector<char> getBufferAsVector(size_t vertexNbr = 0);

    /**
     * Get the component size
     * \return Return the component size
     */
    inline size_t getComponentSize() const { return _baseSize; }

    /**
     * Get component count per entry
     * \return Return the component count per entry
     */
    inline size_t getElementSize() const { return _elementSize; }

    /**
     * Resize the GL buffer
     * \param size Entry count
     */
    void resize(size_t size);

    /**
     * Set the content from a vector of char
     * \param buffer Source buffer
     */
    void setBufferFromVector(const std::vector<char>& buffer);

  protected:
    GLuint _glId{0};
    size_t _size{0};
    size_t _baseSize{0};   // component size, dependent of the type
    GLint _elementSize{0}; // Number of components per vector
    GLenum _type{0};
    GLenum _usage{0};
    GLuint _copyBufferId{0};

    const static inline std::unordered_map<GLenum, size_t> typeToSize = {{GL_FLOAT, sizeof(float)},
        {GL_INT, sizeof(int)},
        {GL_UNSIGNED_INT, sizeof(unsigned int)},
        {GL_SHORT, sizeof(short)},
        {GL_UNSIGNED_BYTE, sizeof(unsigned char)},
        {GL_BYTE, sizeof(char)}};

    void init(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data);

    /**
     * Resize a buffer
     * \param bufferId Buffer handle
     * \param size New size
     */
    void resizeBuffer(GLuint& bufferId, GLsizeiptr size);

    /**
     * Generate a new buffer and bind it
     * \return Return the buffer handle
     */
    GLuint generateAndBindBuffer();

    /**
     * Set the buffer to zero.
     * The buffer is considered to have already been allocated.
     */
    virtual void zeroBuffer() = 0;

    /**
     * Alocate and initializes the buffer given its handle
     * \param bufferId Handle of the buffer
     * \param target Usage target
     * \param size Data size
     * \param data Pointer to the data
     * \param  usage Usage pattern
     */
    virtual void allocateBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) = 0;

    /**
     * Copy data between buffers
     * \param fromId Handle of the source buffer
     * \param toId Handle of the target buffer
     * \param size Size (in bytes) of the data to copy
     */
    virtual void copyBetweenBuffers(GLuint fromId, GLuint toId, GLsizeiptr size) = 0;

    /**
     * Read a buffer and return it
     * \param bufferId Handle of the buffer
     * \param bytesToRead Total bytes to read from the bufer
     * \return Return a std::vector<char> containing the data
     */
    virtual std::vector<char> readBufferFromGpu(GLuint bufferId, GLsizeiptr bytesToRead) = 0;

    /**
     * Wrapper around glGetBufferParameteriv
     * \param bufferId Buffer handle
     * \param target Target usage
     * \param value Value
     * \param data Pre-allocated pointer to data, to hold the parameter
     */
    virtual void getBufferParameteriv(GLuint bufferId, GLenum target, GLenum value, GLint* data) = 0;

    /**
     * Wrapper around glBufferSubData
     * \param bufferId Buffer handle
     * \param target Target usage
     * \param size Data size
     * \param data Pointer to the data to set the bufer to
     */
    virtual void setBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data) = 0;
};

} // namespace Splash

#endif // SPLASH_GPU_BUFFER_H
