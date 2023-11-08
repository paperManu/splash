#include "./graphics/api/opengl/gpu_buffer.h"

namespace Splash::gfx::opengl
{

/*************/
GpuBuffer::GpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data)
{
    _glId = generateAndBindBuffer();
    init(elementSize, type, usage, size, data);
}

/*************/
void GpuBuffer::zeroBuffer()
{
    glClearNamedBufferData(_glId, GL_R8, GL_RED, _type, NULL);
}

/*************/
void GpuBuffer::allocateBufferData(GLuint bufferId, GLenum /*target*/, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    glNamedBufferData(bufferId, size, data, usage);
}

/*************/
void GpuBuffer::copyBetweenBuffers(GLuint fromId, GLuint toId, GLsizeiptr size)
{
    glCopyNamedBufferSubData(fromId, toId, 0, 0, size);
}

/*************/
std::vector<char> GpuBuffer::readBufferFromGpu(GLuint bufferId, GLsizeiptr bytesToRead)
{
    auto buffer = std::vector<char>(bytesToRead);
    glGetNamedBufferSubData(bufferId, 0, buffer.size(), buffer.data());

    return buffer;
}

/*************/
void GpuBuffer::getBufferParameteriv(GLenum bufferId, GLenum /*target*/, GLenum value, GLint* data)
{
    glGetNamedBufferParameteriv(bufferId, value, data);
}

/*************/
void GpuBuffer::setBufferData(GLuint bufferId, GLenum /*target*/, GLsizeiptr size, const GLvoid* data)
{
    glNamedBufferSubData(bufferId, 0, size, data);
}

}; // namespace Splash::gfx::opengl
