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
#include <vector>

#include "config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./mesh/mesh.h"

namespace Splash
{

class GpuBuffer
{
  public:
    /**
     * \brief Constructor
     * \param elementSize Component count for each entry
     * \param type Component type, as per OpenGL specs
     * \param usage Buffer usage, as per OpenGL specs
     * \param size Number of entries
     * \param data Pointer to data to initialized the buffer with
     */
    GpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data = nullptr);

    /**
     * \brief Destructor
     */
    ~GpuBuffer();

    /**
     * \brief Copy constructor
     */
    GpuBuffer(const GpuBuffer& o)
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
    GpuBuffer& operator=(GpuBuffer&&) = default;

    /**
     * \brief Bool pattern
     */
    explicit operator bool() const
    {
        if (_size == 0 || _baseSize == 0 || _elementSize == 0 || _glId == 0)
            return false;
        else
            return true;
    }

    /**
     * \brief Fill the buffer with 0
     */
    void clear();

    /**
     * \brief Get the GL id
     * \return Return the GL id
     */
    inline GLuint getId() const { return _glId; }

    /**
     * \brief Get the entry count in the buffer
     * \return Return the entry count
     */
    inline size_t getSize() const { return _size; }

    /**
     * \brief Get the size in bytes of the buffer
     * \return Return the size in bytes for the buffer
     */
    inline size_t getMemorySize() const { return _size * _baseSize * _elementSize; }

    /**
     * Get the content of the buffer
     * One can specify the vertex number to get, with care!
     */
    std::vector<char> getBufferAsVector(size_t vertexNbr = 0);

    /**
     * \brief Get the component size
     * \return Return the component size
     */
    inline size_t getComponentSize() const { return _baseSize; }

    /**
     * \brief Get component count per entry
     * \return Return the component count per entry
     */
    inline size_t getElementSize() const { return _elementSize; }

    /**
     * \brief Resize the GL buffer
     * \param size Entry count
     */
    void resize(size_t size);

    /**
     * \brief Set the content from a vector of char
     * \param buffer Source buffer
     */
    void setBufferFromVector(const std::vector<char>& buffer);

  private:
    GLuint _glId{0};
    size_t _size{0};
    size_t _baseSize{0};   // component size, dependent of the type
    GLint _elementSize{0}; // Number of components per vector
    GLenum _type{0};
    GLenum _usage{0};

    GLuint _copyBufferId{0};
};

} // end of namespace

#endif // SPLASH_GPU_BUFFER_H
