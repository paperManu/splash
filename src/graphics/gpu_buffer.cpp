#include "./graphics/gpu_buffer.h"

using namespace std;

namespace Splash
{

/*************/
GpuBuffer::GpuBuffer(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data)
{
    glCreateBuffers(1, &_glId);
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
        glNamedBufferData(_glId, size * _elementSize * _baseSize, zeroBuffer.data(), usage);
    }
    else
    {
        glNamedBufferData(_glId, size * _elementSize * _baseSize, data, usage);
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

    glClearNamedBufferData(_glId, GL_R8, GL_RED, _type, NULL);
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
        glCreateBuffers(1, &_copyBufferId);
        if (!_copyBufferId)
            return {};
    }

    int copyBufferSize = 0;
    glGetNamedBufferParameteriv(_copyBufferId, GL_BUFFER_SIZE, &copyBufferSize);
    if (static_cast<uint32_t>(copyBufferSize) < vectorSize)
    {
        glDeleteBuffers(1, &_copyBufferId);
        glCreateBuffers(1, &_copyBufferId);
        glNamedBufferData(_copyBufferId, vectorSize, nullptr, GL_STREAM_COPY);
    }

    // Copy the actual buffer to the copy buffer
    glCopyNamedBufferSubData(_glId, _copyBufferId, 0, 0, vectorSize);

    // Read the copy buffer
    auto buffer = vector<char>(vectorSize);
    glGetNamedBufferSubData(_copyBufferId, 0, buffer.size(), buffer.data());

    return buffer;
}

/*************/
void GpuBuffer::setBufferFromVector(const vector<char>& buffer)
{
    if (!_glId || !_type || !_usage || !_elementSize)
        return;

    if (buffer.size() > _baseSize * _elementSize * _size)
        resize(buffer.size());

    glNamedBufferSubData(_glId, 0, buffer.size(), buffer.data());
}

/*************/
void GpuBuffer::resize(size_t size)
{
    if (!_type || !_usage || !_elementSize)
        return;

    glDeleteBuffers(1, &_glId);
    glCreateBuffers(1, &_glId);
    if (!_glId)
        return;

    glNamedBufferData(_glId, size * _elementSize * _baseSize, nullptr, _usage);

    _size = size;
}

} // end of namespace
