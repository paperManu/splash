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

#include "./core/constants.h"
#include "./core/attribute.h"
#include "./mesh/mesh.h"

namespace Splash
{

class GpuBuffer
{
  public:
    /**
     * Destructor
     */
    ~GpuBuffer();

    std::shared_ptr<GpuBuffer> copyBuffer() const;
    GpuBuffer& operator=(const GpuBuffer&) = delete;
    GpuBuffer(GpuBuffer&&) = delete;
    GpuBuffer& operator=(GpuBuffer&&) = default;

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
    /**
     * Constructor
     */
    GpuBuffer() = default;

    void init(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data);
    
    // `bufferId` is passed by reference as it may be changed. This change needs to be propagated.
    void resizeBuffer(GLuint& bufferId, GLsizeiptr size);
    
    // Calls `glGen` and `glBindBuffer`. Convinence function.
    GLuint generateAndBindBuffer();


    // Assumes an already allocated buffer on the GPU.
    virtual void zeroBuffer() = 0;

    virtual void allocateBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) = 0;
    virtual void copyBetweenBuffers(GLuint fromId, GLuint toId, GLsizeiptr size) = 0;
    virtual std::vector<char> readBufferFromGpu(GLuint bufferId, GLsizeiptr bytesToRead) = 0;

    // Different `value`s return different lengths of `data`. You need to pre-allocate some array and pass the pointer.
    virtual void getBufferParameteriv(GLuint bufferId, GLenum target, GLenum value, GLint * data) = 0;

    // Assumes an already allocated buffer on the GPU.
    virtual void setBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data) = 0;

  protected:
    GLuint _glId{0};
    size_t _size{0};
    size_t _baseSize{0};   // component size, dependent of the type
    GLint _elementSize{0}; // Number of components per vector
    GLenum _type{0};
    GLenum _usage{0};
    GLuint _copyBufferId{0};

    const static inline std::unordered_map<GLenum, size_t> typeToSize = {
	{GL_FLOAT, sizeof(float)},
	{GL_INT, sizeof(int)},
	{GL_UNSIGNED_INT, sizeof(unsigned int)},
	{GL_SHORT, sizeof(short)},
	{GL_UNSIGNED_BYTE, sizeof(unsigned char)},
	{GL_BYTE, sizeof(char)}
    };

};

} // namespace Splash

#endif // SPLASH_GPU_BUFFER_H
