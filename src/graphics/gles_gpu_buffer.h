#ifndef SPLASH_GLES_GPU_BUFFER_H
#define SPLASH_GLES_GPU_BUFFER_H

#include "graphics/gpu_buffer.h"

namespace Splash 
{
    class GLESGpuBuffer final: public GpuBuffer 
    {
	public:
	    GLESGpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data);

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
