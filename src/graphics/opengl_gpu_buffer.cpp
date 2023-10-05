#include "./graphics/opengl_gpu_buffer.h"

namespace Splash 
{

/*************/
OpenGLGpuBuffer::OpenGLGpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data)
{
    _glId = generateAndBindBuffer();
    init(elementSize, type, usage, size, data);
}

/*************/
void OpenGLGpuBuffer::zeroBuffer() 
{
    glClearNamedBufferData(_glId, GL_R8, GL_RED, _type, NULL);
}

/*************/
void OpenGLGpuBuffer::allocateBufferData(GLuint bufferId, GLenum /*target*/, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    glNamedBufferData(bufferId, size, data, usage);
}

/*************/
void OpenGLGpuBuffer::copyBetweenBuffers(GLuint fromId, GLuint toId, GLsizeiptr size)
{
    glCopyNamedBufferSubData(fromId, toId, 0, 0, size);
}

/*************/
std::vector<char> OpenGLGpuBuffer::readBufferFromGpu(GLuint bufferId, GLsizeiptr bytesToRead) 
{
    auto buffer = std::vector<char>(bytesToRead);
    glGetNamedBufferSubData(bufferId, 0, buffer.size(), buffer.data());

    return buffer;
}

/*************/
void OpenGLGpuBuffer::getBufferParameteriv(GLenum bufferId, GLenum /*target*/, GLenum value, GLint* data)
{
    glGetNamedBufferParameteriv(bufferId, value, data);
}

/*************/
void OpenGLGpuBuffer::setBufferData(GLuint bufferId, GLenum /*target*/, GLsizeiptr size, const GLvoid* data) 
{
    glNamedBufferSubData(bufferId, 0, size, data);
}

};
