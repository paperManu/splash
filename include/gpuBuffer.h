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
 * @gpuBuffer.h
 * Class handling OpenGL buffers
 */

#ifndef SPLASH_GPU_BUFFER_H
#define SPLASH_GPU_BUFFER_H

#include <chrono>
#include <map>
#include <vector>
#include <glm/glm.hpp>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "mesh.h"

namespace Splash {

class GpuBuffer
{
    public:
        /**
         * Constructor
         * size is given as the number of elements
         */
        GpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data = nullptr);

        /**
         * Destructor
         */
        ~GpuBuffer();

        /**
         * Copy and move constructors
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
            glBufferData(GL_ARRAY_BUFFER, _size * _baseSize, nullptr, _usage);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        GpuBuffer& operator=(const GpuBuffer&) = delete;
        GpuBuffer(GpuBuffer&&) = delete;
        GpuBuffer& operator=(GpuBuffer&&) = default;

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
         */
        inline GLuint getId() const {return _glId;}

        /**
         * Get the size of the buffer
         */
        inline size_t getSize() const {return _size;}

        /**
         * Get the size in bytes of the buffer
         */
        inline size_t getMemorySize() const {return _size * _baseSize * _elementSize;}

        /**
         * Get the content of the buffer
         * One can specify the vertex number to get, with care!
         */
        std::vector<char> getBufferAsVector(size_t vertexNbr = 0);

        /**
         * Get the component size
         */
        inline size_t getComponentSize() const {return _baseSize;}

        /**
         * Get the element size
         */
        inline size_t getElementSize() const {return _elementSize;}

        /**
         * Resize the GL buffer
         */
        void resize(size_t size);

        /**
         * Set the content from a vector of char
         */
        void setBufferFromVector(const std::vector<char>& buffer);

    private:
        GLuint _glId {0};
        size_t _size {0};
        size_t _baseSize {0}; // component size, dependent of the type
        GLint _elementSize {0}; // Number of components per vector
        GLenum _type {0};
        GLenum _usage {0};

        GLuint _copyBufferId {0};
};

} // end of namespace

#endif // SPLASH_GPU_BUFFER_H
