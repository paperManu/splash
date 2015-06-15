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

    glBufferData(GL_ARRAY_BUFFER, size * _baseSize, data, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*************/
GpuBuffer::~GpuBuffer()
{
    glDeleteBuffers(1, &_glId);
}

/*************/
void GpuBuffer::resize(size_t size)
{
    if (!_type || !_usage || !_elementSize)
        return;

    glDeleteBuffers(1, &_glId);
    glGenBuffers(1, &_glId);
    glBindBuffer(GL_ARRAY_BUFFER, _glId);
    glBufferData(GL_ARRAY_BUFFER, size * _baseSize, nullptr, _usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // end of namespace
