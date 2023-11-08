#include "./graphics/gles_gpu_buffer.h"

namespace Splash
{

/*************/
GLESGpuBuffer::GLESGpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data)
{
    _glId = generateAndBindBuffer();
    init(elementSize, type, usage, size, data);
}

/*************/
void GLESGpuBuffer::zeroBuffer()
{
    // Previously used `glClearBufferData` with the data set to `nullptr`, this causes the buffer to be filled with zeros. Unfortunately, this function is not available in OpenGL
    // ES, so we make do with a manual upload.
    const auto zerosSize = _size * _elementSize * _baseSize;
    const auto zeroBuffer = std::vector<char>(zerosSize, 0);

    setBufferData(_glId, GL_ARRAY_BUFFER, zerosSize, zeroBuffer.data());
}

/*************/
void GLESGpuBuffer::allocateBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    glBindBuffer(target, bufferId);
    glBufferData(target, size, data, usage);
}

/*************/
void GLESGpuBuffer::copyBetweenBuffers(GLuint fromId, GLuint toId, GLsizeiptr size)
{
    glBindBuffer(GL_COPY_READ_BUFFER, fromId);
    glBindBuffer(GL_COPY_WRITE_BUFFER, toId);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, size);
}

/*************/
std::vector<char> GLESGpuBuffer::readBufferFromGpu(GLuint bufferId, GLsizeiptr bytesToRead)
{
    glBindBuffer(GL_ARRAY_BUFFER, bufferId);
    const auto* bufferPtr = static_cast<char*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, bytesToRead, GL_MAP_READ_BIT));
    assert(bufferPtr != nullptr && "Mapped buffer returned a null pointer!");
    auto buffer = std::vector<char>(bufferPtr, bufferPtr + bytesToRead);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return buffer;
}

/*************/
void GLESGpuBuffer::getBufferParameteriv(GLenum bufferId, GLenum target, GLenum value, GLint* data)
{
    glBindBuffer(target, bufferId);
    glGetBufferParameteriv(target, value, data);
    glBindBuffer(target, 0);
}

/*************/
void GLESGpuBuffer::setBufferData(GLuint bufferId, GLenum target, GLsizeiptr size, const GLvoid* data)
{
    glBindBuffer(target, bufferId);
    glBufferSubData(target, 0, size, data);
    glBindBuffer(target, 0);
}

} // namespace Splash
