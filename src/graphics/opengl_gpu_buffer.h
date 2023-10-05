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

#ifndef SPLASH_OpenGL_GPU_BUFFER_H
#define SPLASH_OpenGL_GPU_BUFFER_H

#include "./graphics/gpu_buffer.h"

namespace Splash 
{

class OpenGLGpuBuffer final: public GpuBuffer 
{
    public:
	OpenGLGpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data);

    protected:
	virtual void zeroBuffer() override final;
	virtual void allocateBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) override final;
	virtual void copyBetweenBuffers(GLuint fromId, GLuint toId, GLsizeiptr size) override final;
	virtual std::vector<char> readBufferFromGpu(GLuint bufferId, GLsizeiptr bytesToRead) override final;
	virtual void getBufferParameteriv(GLenum bufferId, GLenum target, GLenum value, GLint * data) override final;
	virtual void setBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data) override final;
};

}

#endif
