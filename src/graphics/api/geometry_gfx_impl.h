#ifndef SPLASH_GFX_GEOMETRY_GFX_IMPL_H
#define SPLASH_GFX_GEOMETRY_GFX_IMPL_H

#include <chrono>
#include <map>
#include <memory>

#include "./core/constants.h"

// TODO: Move GPUBuffer to be internal to GeometryGfxImpl?
#include "./graphics/geometry.h"
#include "./graphics/gpu_buffer.h"
#include "./graphics/renderer.h"

namespace Splash::gfx
{
class GeometryGfxImpl
{
  public:
    GeometryGfxImpl() { glGenQueries(1, &_feedbackQuery); }

    ~GeometryGfxImpl()
    {
        for (auto v : _vertexArray)
            glDeleteVertexArrays(1, &(v.second));

        glDeleteQueries(1, &_feedbackQuery);
    }

    void activate() { glBindVertexArray(_vertexArray[glfwGetCurrentContext()]); }

    void deactivate()
    {
#if DEBUG
        glBindVertexArray(0);
#endif
    }

    void activateAsSharedBuffer()
    {
        const auto& buffers = _useAlternativeBuffers ? _glAlternativeBuffers : _glBuffers;

        for (uint i = 0; i < 4; i++)
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, buffers[i]->getId());
    }

    void resizeTempBuffers()
    {
        _temporaryBufferSize = _feedbackMaxNbrPrimitives * 6; // 3 vertices per primitive, times two to keep some margin for future updates
        for (size_t i = 0; i < _glBuffers.size(); ++i)
        {
            // This creates a copy of the buffer
            auto tempBuffer = _glBuffers[i]->copyBuffer();
            tempBuffer->resize(_temporaryBufferSize);
            _glTemporaryBuffers[i] = tempBuffer;
        }
    }

    void activateForFeedback()
    {
        for (unsigned int i = 0; i < _glTemporaryBuffers.size(); ++i)
        {
            _glTemporaryBuffers[i]->clear();
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, _glTemporaryBuffers[i]->getId());
        }

        glBeginQuery(GL_PRIMITIVES_GENERATED, _feedbackQuery);
    }

    void deactivateFeedback()
    {
#if DEBUG
        for (unsigned int i = 0; i < _glTemporaryBuffers.size(); ++i)
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, 0);
#endif

        glEndQuery(GL_PRIMITIVES_GENERATED);
        GLuint drawnPrimitives = 0;
        while (true)
        {
            glGetQueryObjectuiv(_feedbackQuery, GL_QUERY_RESULT_AVAILABLE, &drawnPrimitives);
            if (drawnPrimitives != 0)
                break;
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        glGetQueryObjectuiv(_feedbackQuery, GL_QUERY_RESULT, &drawnPrimitives);
        _feedbackMaxNbrPrimitives = std::max(_feedbackMaxNbrPrimitives, drawnPrimitives);
        _temporaryVerticesNumber = drawnPrimitives * 3;
    }

    uint getVerticesNumber() const
    {
        return _useAlternativeBuffers ? _alternativeVerticesNumber : _verticesNumber;
    }

    bool buffersTooSmall()
    {
        _feedbackMaxNbrPrimitives = std::max(_verticesNumber / 3, _feedbackMaxNbrPrimitives);
        return _feedbackMaxNbrPrimitives * 6 > _temporaryBufferSize;
    }

    /**
     * Get a copy of the given GPU buffer
     * \param type GPU buffer type
     * \return Return a vector containing a copy of the buffer
     */
    std::vector<char> getGpuBufferAsVector(int typeId, bool forceAlternativeBuffers = false)
    {
        if ((forceAlternativeBuffers || _useAlternativeBuffers) && _glAlternativeBuffers[typeId] != nullptr)
            return _glAlternativeBuffers[typeId]->getBufferAsVector();
        else
            return _glBuffers[typeId]->getBufferAsVector();
    }

    bool anyAlternativeBufferMissing() const
    {
        return std::any_of(_glAlternativeBuffers.cbegin(), _glAlternativeBuffers.cend(), [](const auto& buffer) { return buffer == nullptr; });
    }

    Mesh::MeshContainer serialize(const std::string& name)
    {
        if (anyAlternativeBufferMissing())
            return {};

        const auto vertices = _glAlternativeBuffers[0]->getBufferAsVector();
        const auto uvs = _glAlternativeBuffers[1]->getBufferAsVector();
        const auto normals = _glAlternativeBuffers[2]->getBufferAsVector();
        const auto annexe = _glAlternativeBuffers[3]->getBufferAsVector();

        const auto verticesData = reinterpret_cast<const glm::vec4*>(vertices.data());
        const auto uvsData = reinterpret_cast<const glm::vec2*>(uvs.data());
        const auto normalsData = reinterpret_cast<const glm::vec4*>(normals.data());
        const auto annexeData = reinterpret_cast<const glm::vec4*>(annexe.data());

        return Mesh::MeshContainer{
            .name = name,
            .vertices = std::vector<glm::vec4>(verticesData, verticesData + _alternativeVerticesNumber),
            .uvs = std::vector<glm::vec2>(uvsData, uvsData + _alternativeVerticesNumber),
            .normals = std::vector<glm::vec4>(normalsData, normalsData + _alternativeVerticesNumber),
            .annexe = std::vector<glm::vec4>(annexeData, annexeData + _alternativeVerticesNumber),
        };
    }

    void swapBuffers()
    {
        _glAlternativeBuffers.swap(_glTemporaryBuffers);

        int tmp = _alternativeVerticesNumber;
        _alternativeVerticesNumber = _temporaryVerticesNumber;
        _temporaryVerticesNumber = tmp;

        tmp = _alternativeBufferSize;
        _alternativeBufferSize = _temporaryBufferSize;
        _temporaryBufferSize = tmp;
    }

    void initVertices(Renderer* renderer, float* data, uint numVerts)
    {
        setVerticesNumber(numVerts);
        _glBuffers[0] = renderer->createGpuBuffer(4, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, data);
    }

    // componentsPerElement: How many primitives (float, int, etc..) in one component (vec2, ivec3, etc)
    // TODO: createGpuBuffer probably needs to take a const pointer?
    void allocateOrInitBuffer(Renderer* renderer, Geometry::BufferType bufferType, uint componentsPerElement, std::vector<float>& dataVec)
    {
        const auto bufferIndex = static_cast<uint8_t>(bufferType);
        if (!dataVec.empty())
            _glBuffers[bufferIndex] = renderer->createGpuBuffer(componentsPerElement, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, dataVec.data());
        else
            _glBuffers[bufferIndex] = renderer->createGpuBuffer(componentsPerElement, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, nullptr);
    }

    void clearFromAllContexts()
    {
        for (auto& v : _vertexArray)
            glDeleteVertexArrays(1, &(v.second));

        _vertexArray.clear();
    }

    void updateTemporaryBuffers(Renderer* renderer, Mesh::MeshContainer* deserializedMesh)
    {
        _temporaryVerticesNumber = deserializedMesh->vertices.size();
        _temporaryBufferSize = _temporaryVerticesNumber;

        allocateOrInitTemporaryBuffer(renderer, Geometry::BufferType::Vertex, 4, _temporaryVerticesNumber, reinterpret_cast<char*>(deserializedMesh->vertices.data()));
        allocateOrInitTemporaryBuffer(renderer, Geometry::BufferType::TexCoords, 2, _temporaryVerticesNumber, reinterpret_cast<char*>(deserializedMesh->uvs.data()));
        allocateOrInitTemporaryBuffer(renderer, Geometry::BufferType::Normal, 4, _temporaryVerticesNumber, reinterpret_cast<char*>(deserializedMesh->normals.data()));
        allocateOrInitTemporaryBuffer(renderer, Geometry::BufferType::Annexe, 4, _temporaryVerticesNumber, reinterpret_cast<char*>(deserializedMesh->annexe.data()));
    }

    void update(bool& buffersDirty)
    {
        GLFWwindow* context = glfwGetCurrentContext();
        auto vertexArrayIt = _vertexArray.find(context);
        if (vertexArrayIt == _vertexArray.end() || buffersDirty)
        {
            if (vertexArrayIt == _vertexArray.end())
            {
                vertexArrayIt = (_vertexArray.emplace(std::make_pair(context, 0))).first;
                vertexArrayIt->second = 0;
                glGenVertexArrays(1, &(vertexArrayIt->second));
            }

            glBindVertexArray(vertexArrayIt->second);

            for (uint32_t idx = 0; idx < _glBuffers.size(); ++idx)
            {

                const auto& buffers = (_useAlternativeBuffers && _glAlternativeBuffers[0] != nullptr) ? _glAlternativeBuffers : _glBuffers;

                glBindBuffer(GL_ARRAY_BUFFER, buffers[idx]->getId());
                glVertexAttribPointer((GLuint)idx, buffers[idx]->getElementSize(), GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray((GLuint)idx);
            }

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            buffersDirty = false;
        }
    }

    void useAlternativeBuffers(bool isActive)
    {
        _useAlternativeBuffers = isActive;
    }

  private:
    void setVerticesNumber(uint verticesNumber)
    {
        _verticesNumber = verticesNumber;
    }

    void allocateOrInitTemporaryBuffer(Renderer* renderer, Geometry::BufferType bufferType, uint componentsPerElement, uint tempVerticesNumber, char* data)
    {
        const auto bufferIndex = static_cast<uint8_t>(bufferType);
        if (!_glTemporaryBuffers[bufferIndex])
            _glTemporaryBuffers[bufferIndex] = renderer->createGpuBuffer(componentsPerElement, GL_FLOAT, GL_STATIC_DRAW, tempVerticesNumber, reinterpret_cast<GLvoid*>(data));
        else
            _glTemporaryBuffers[bufferIndex]->setBufferFromVector({data, data + tempVerticesNumber * sizeof(float) * componentsPerElement});
    }

  private:
    std::map<GLFWwindow*, GLuint> _vertexArray;
    GLuint _feedbackQuery;

    std::array<std::shared_ptr<GpuBuffer>, 4> _glBuffers{};
    std::array<std::shared_ptr<GpuBuffer>, 4> _glAlternativeBuffers{}; // Alternative buffers used for rendering
    std::array<std::shared_ptr<GpuBuffer>, 4> _glTemporaryBuffers{};   // Temporary buffers used for feedback

    GLuint _feedbackMaxNbrPrimitives{0};

    GLuint _verticesNumber{0};
    GLuint _temporaryVerticesNumber{0};
    int _alternativeVerticesNumber{0};

    GLuint _temporaryBufferSize{0};
    GLuint _alternativeBufferSize{0};

    bool _useAlternativeBuffers{false};
};

} // namespace Splash::gfx

#endif
