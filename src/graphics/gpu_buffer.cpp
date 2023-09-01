#include "./graphics/gpu_buffer.h"
#include "glad/glad.h"

namespace Splash
{

/*************/
GpuBuffer::GpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data)
{
    glGenBuffers(1, &_glId);

    switch (type)
    {
    case GL_FLOAT:
        _baseSize = sizeof(float);
        break;
    case GL_INT:
        _baseSize = sizeof(int);
        break;
    case GL_UNSIGNED_INT:
        _baseSize = sizeof(unsigned int);
        break;
    case GL_SHORT:
        _baseSize = sizeof(short);
        break;
    case GL_UNSIGNED_BYTE:
        _baseSize = sizeof(unsigned char);
        break;
    case GL_BYTE:
        _baseSize = sizeof(char);
        break;
    default:
        return;
    }

    _size = size;
    _elementSize = elementSize;
    _type = type;
    _usage = usage;

    glBindBuffer(GL_ARRAY_BUFFER, _glId);
    if (data == nullptr)
    {
        auto zeroBuffer = std::vector<char>(size * _elementSize * _baseSize, 0);
        glBufferData(GL_ARRAY_BUFFER, size * _elementSize * _baseSize, zeroBuffer.data(), usage);
    }
    else
    {
        glBufferData(GL_ARRAY_BUFFER, size * _elementSize * _baseSize, data, usage);
    }
}

/*************/
GpuBuffer::~GpuBuffer()
{
    if (_glId)
        glDeleteBuffers(1, &_glId);
    if (_copyBufferId)
        glDeleteBuffers(1, &_copyBufferId);
}

/*************/
void GpuBuffer::clear()
{
    if (!_glId)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, _glId);

    // Previously used `glClearBufferData` with the data set to `nullptr`, this causes the buffer to be filled with zeros. Unfortunately, this function is not available in OpenGL
    // ES, so we make do with a manual upload.
    const auto zerosSize = _size * _elementSize * _baseSize;
    const auto zeros = std::vector<char>(zerosSize, 0);
    glBufferData(GL_ARRAY_BUFFER, zerosSize, zeros.data(), _usage);
}

/*************/
std::vector<char> GpuBuffer::getBufferAsVector(size_t vertexNbr)
{
    if (!_glId || !_type || !_usage || !_elementSize)
        return {};

    size_t vectorSize = 0;
    if (vertexNbr)
        vectorSize = _baseSize * _elementSize * vertexNbr;
    else
        vectorSize = _baseSize * _elementSize * _size;

    // Initialize / resize the copy buffer
    if (!_copyBufferId)
    {
        glGenBuffers(1, &_copyBufferId);
        if (!_copyBufferId)
            return {};
    }

    int copyBufferSize = 0;
    glBindBuffer(GL_ARRAY_BUFFER, _copyBufferId);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &copyBufferSize);
    if (static_cast<uint32_t>(copyBufferSize) < vectorSize)
    {
        glDeleteBuffers(1, &_copyBufferId);
        glGenBuffers(1, &_copyBufferId);

        glBindBuffer(GL_ARRAY_BUFFER, _copyBufferId);
        glBufferData(GL_ARRAY_BUFFER, vectorSize, nullptr, GL_STREAM_COPY);
    }

    // Copy the actual buffer to the copy buffer
    glBindBuffer(GL_COPY_READ_BUFFER, _glId);
    glBindBuffer(GL_COPY_WRITE_BUFFER, _copyBufferId);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, vectorSize);

    // Read the copy buffer
    glBindBuffer(GL_ARRAY_BUFFER, _copyBufferId);
    auto* bufferPtr = static_cast<char*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, vectorSize, GL_MAP_READ_BIT));
    auto buffer = std::vector<char>();
    buffer.assign(bufferPtr, bufferPtr + vectorSize);

    return buffer;
}

/*************/
void GpuBuffer::setBufferFromVector(const std::vector<char>& buffer)
{
    if (!_glId || !_type || !_usage || !_elementSize)
        return;

    if (buffer.size() > _baseSize * _elementSize * _size)
        resize(buffer.size());

    glBindBuffer(GL_ARRAY_BUFFER, _glId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, buffer.size(), buffer.data());
}

/*************/
void GpuBuffer::resize(size_t size)
{
    if (!_type || !_usage || !_elementSize)
        return;

    glDeleteBuffers(1, &_glId);
    glGenBuffers(1, &_glId);
    if (!_glId)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, _glId);
    glBufferData(GL_ARRAY_BUFFER, size * _elementSize * _baseSize, nullptr, _usage);

    _size = size;
}

} // namespace Splash
