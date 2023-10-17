#include "./graphics/gpu_buffer.h"

#include <sstream>
#include <unordered_map>

#include "./graphics/gles_gpu_buffer.h"
#include "./graphics/opengl_gpu_buffer.h"

namespace Splash
{

/*************/
void GpuBuffer::init(GLint elementSize, GLenum type, GLenum usage, size_t size, GLvoid* data)
{
    const bool typeInTable = typeToSize.find(type) != typeToSize.end();

    if (!typeInTable)
    {
        // Print the type in hex for quick searching through GLAD's headers.
        std::stringstream ss;
        ss << std::hex << type;

        Log::get() << "Cannot find type (" << ss.str() << ") in the types supported by GpuBuffer" << Log::endl;
        return;
    }

    _baseSize = typeToSize.at(type);
    _size = size;
    _elementSize = elementSize;
    _type = type;
    _usage = usage;

    const auto totalBytes = size * elementSize * _baseSize;
    allocateBufferData(_glId, GL_ARRAY_BUFFER, totalBytes, data, usage);

    // If data is null, no data is copied. The buffer is just allocated and left uninitialized.
    // Otherwise, we allocate and copy in one move.
    if (data == nullptr)
        zeroBuffer();
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

    zeroBuffer();
}

/*************/
std::vector<char> GpuBuffer::getBufferAsVector(size_t vertexNbr)
{
    if (!_glId || !_type || !_usage || !_elementSize)
        return {};

    // Initialize / resize the copy buffer
    if (!_copyBufferId)
    {
        _copyBufferId = generateAndBindBuffer();
        if (!_copyBufferId)
            return {};
    }

    size_t vectorSize = 0;
    if (vertexNbr)
        vectorSize = _baseSize * _elementSize * vertexNbr;
    else
        vectorSize = _baseSize * _elementSize * _size;

    // Resize the copy buffer if needed
    resizeBuffer(_copyBufferId, vectorSize);

    // Copy the actual buffer to the copy buffer
    copyBetweenBuffers(_glId, _copyBufferId, vectorSize);

    // Read the copy buffer
    return readBufferFromGpu(_copyBufferId, vectorSize);
}

/*************/
void GpuBuffer::setBufferFromVector(const std::vector<char>& buffer)
{
    if (!_glId || !_type || !_usage || !_elementSize)
        return;

    if (buffer.size() > _baseSize * _elementSize * _size)
        resize(buffer.size());

    setBufferData(_glId, GL_ARRAY_BUFFER, buffer.size(), buffer.data());
}

/*************/
void GpuBuffer::resize(size_t size)
{
    if (!_type || !_usage || !_elementSize)
        return;

    const auto totalSize = size * _elementSize * _baseSize;
    resizeBuffer(_glId, totalSize);

    _size = size;
}

/*************/
std::shared_ptr<GpuBuffer> GpuBuffer::copyBuffer() const
{
    std::shared_ptr<GpuBuffer> newBuffer;
    if (const auto ptr = dynamic_cast<const GLESGpuBuffer*>(this))
    {
        return std::make_shared<GLESGpuBuffer>(this->_elementSize, this->_type, this->_usage, this->_size, nullptr);
    }
    else if (const auto ptr = dynamic_cast<const OpenGLGpuBuffer*>(this))
    {
        return std::make_shared<OpenGLGpuBuffer>(this->_elementSize, this->_type, this->_usage, this->_size, nullptr);
    }
    else
    {
        assert(false && "Forgot to add support for this new GPU buffer type!");
        return nullptr;
    }
}

/*************/
void GpuBuffer::resizeBuffer(GLuint& bufferId, GLsizeiptr size)
{
    int bufferSize = 0;
    getBufferParameteriv(bufferId, GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
    if (static_cast<GLsizeiptr>(bufferSize) < size)
    {
        glDeleteBuffers(1, &bufferId);
        bufferId = generateAndBindBuffer();

        allocateBufferData(bufferId, GL_ARRAY_BUFFER, size, nullptr, GL_STREAM_COPY);
    }
}

/*************/
GLuint GpuBuffer::generateAndBindBuffer()
{
    GLuint newBuffer = 0;
    glGenBuffers(1, &newBuffer);

    assert(newBuffer != 0 && "Failed to create a buffer.");

    glBindBuffer(GL_ARRAY_BUFFER, newBuffer);

    return newBuffer;
}

} // namespace Splash
