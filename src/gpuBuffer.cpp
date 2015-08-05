#include "gpuBuffer.h"

using namespace std;

namespace Splash
{

/*************/
GpuBuffer::GpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data)
{
    glGenBuffers(1, &_glId);
    glBindBuffer(GL_ARRAY_BUFFER, _glId);
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

    if (data == nullptr)
    {
        auto zeroBuffer = vector<char>(size * _elementSize * _baseSize, 0);
        glBufferData(GL_ARRAY_BUFFER, size * _elementSize * _baseSize, zeroBuffer.data(), usage);
    }
    else
    {
        glBufferData(GL_ARRAY_BUFFER, size * _elementSize * _baseSize, data, usage);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    glClearBufferData(GL_ARRAY_BUFFER, GL_R8, GL_RED, _type, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*************/
vector<char> GpuBuffer::getBufferAsVector(size_t vertexNbr)
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
    if (copyBufferSize < vectorSize)
    {
        glDeleteBuffers(1, &_copyBufferId);
        glGenBuffers(1, &_copyBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, _copyBufferId);
        glBufferData(GL_ARRAY_BUFFER, vectorSize, nullptr, GL_STREAM_COPY);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Copy the actual buffer to the copy buffer
    glBindBuffer(GL_COPY_READ_BUFFER, _glId);
    glBindBuffer(GL_COPY_WRITE_BUFFER, _copyBufferId);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, vectorSize);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    glBindBuffer(GL_COPY_READ_BUFFER, 0);

    // Read the copy buffer
    auto buffer = vector<char>(vectorSize);
    glBindBuffer(GL_ARRAY_BUFFER, _copyBufferId);
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, buffer.size(), buffer.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return buffer;
}

/*************/
void GpuBuffer::setBufferFromVector(const vector<char>& buffer)
{
    if (!_glId || !_type || !_usage || !_elementSize)
        return;

    if (buffer.size() > _baseSize * _elementSize * _size)
        resize(buffer.size());

    glBindBuffer(GL_ARRAY_BUFFER, _glId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, buffer.size(), buffer.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    _size = size;
}

} // end of namespace
