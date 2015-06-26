#include "geometry.h"

#include "log.h"
#include "mesh.h"

using namespace std;
using namespace glm;

namespace Splash {

/*************/
Geometry::Geometry()
{
    _type = "geometry";
    registerAttributes();

    glGenQueries(1, &_feedbackQuery);

    _mesh = make_shared<Mesh>();
    update();
    _timestamp = _mesh->getTimestamp();
}

/*************/
Geometry::~Geometry()
{
    for (auto v : _vertexArray)
        glDeleteVertexArrays(1, &(v.second));

    glDeleteQueries(1, &_feedbackQuery);

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Geometry::~Geometry - Destructor" << Log::endl;
#endif
}

/*************/
void Geometry::activate()
{
    _mutex.lock();
    glBindVertexArray(_vertexArray[glfwGetCurrentContext()]);
}

/*************/
void Geometry::activateAsSharedBuffer()
{
    _mutex.lock();

    if (_useAlternativeBuffers)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _glAlternativeBuffers[0]->getId());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _glAlternativeBuffers[1]->getId());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _glAlternativeBuffers[2]->getId());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _glAlternativeBuffers[3]->getId());
    }
    else
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _glBuffers[0]->getId());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _glBuffers[1]->getId());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _glBuffers[2]->getId());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _glBuffers[3]->getId());
    }
}

/*************/
void Geometry::activateForFeedback()
{
    _feedbackMaxNbrPrimitives = std::max(_verticesNumber / 3, _feedbackMaxNbrPrimitives);
    if (_glTemporaryBuffers.size() < _glBuffers.size() || _buffersDirty || _feedbackMaxNbrPrimitives * 3 * 4 >= _temporaryBufferSize)
    {
        _glTemporaryBuffers.clear();
        _temporaryBufferSize = _feedbackMaxNbrPrimitives * 3 * 5; // 3 vertices per primitive, 4 components for each buffer, plus a bonus
        for (auto& buffer : _glBuffers)
        {
            // This creates a copy of the buffer
            auto altBuffer = std::make_shared<GpuBuffer>(*buffer);
            altBuffer->resize(_temporaryBufferSize);
            _glTemporaryBuffers.push_back(altBuffer);
        }
        _buffersResized = true;
    }
    else
    {
        _buffersResized = false;
    }

    for (unsigned int i = 0; i < _glTemporaryBuffers.size(); ++i)
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, _glTemporaryBuffers[i]->getId());

    glBeginQuery(GL_PRIMITIVES_GENERATED, _feedbackQuery);
}

/*************/
void Geometry::deactivate() const
{
#if DEBUG
    glBindVertexArray(0);
#endif
    _mutex.unlock();
}

/*************/
void Geometry::deactivateFeedback()
{
#if DEBUG
    for (unsigned int i = 0; i < _glTemporaryBuffers.size(); ++i)
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, 0);
#endif

    glEndQuery(GL_PRIMITIVES_GENERATED);
    int drawnPrimitives;
    glGetQueryObjectiv(_feedbackQuery, GL_QUERY_RESULT, &drawnPrimitives);
    _feedbackMaxNbrPrimitives = std::max(_feedbackMaxNbrPrimitives, drawnPrimitives);
    _temporaryVerticesNumber = drawnPrimitives * 3;
}

/*************/
bool Geometry::linkTo(BaseObjectPtr obj)
{
    // Mandatory before trying to link
    BaseObject::linkTo(obj);

    if (dynamic_pointer_cast<Mesh>(obj).get() != nullptr)
    {
        MeshPtr mesh = dynamic_pointer_cast<Mesh>(obj);
        setMesh(mesh);
        return true;
    }

    return false;
}

/*************/
float Geometry::pickVertex(dvec3 p, dvec3& v)
{
    float distance = numeric_limits<float>::max();
    dvec3 closestVertex;

    vector<float> vertices = _mesh->getVertCoords();
    for (int i = 0; i < vertices.size(); i += 4)
    {
        dvec3 vertex(vertices[i], vertices[i + 1], vertices[i + 2]);
        float dist = length(p - vertex);
        if (dist < distance)
        {
            closestVertex = vertex;
            distance = dist;
        }
    }

    v = closestVertex;

    return distance;
}

/*************/
void Geometry::resetAlternativeBuffer(int index)
{
    if (index >= 4)
    {
        return;
    }
    else if (index >= 0)
    {
        _glAlternativeBuffers[index].reset();
        _alternativeVerticesNumber = 0;
    }
    else
    {
        for (int i = 0; i < _glAlternativeBuffers.size(); ++i)
            _glAlternativeBuffers[i].reset();
        _alternativeVerticesNumber = 0;
    }

    _buffersDirty = true;
}

/*************/
void Geometry::swapBuffers()
{
    _glAlternativeBuffers.swap(_glTemporaryBuffers);

    int tmp = _alternativeVerticesNumber;
    _alternativeVerticesNumber = _temporaryVerticesNumber;
    _temporaryVerticesNumber = tmp;

    tmp = _alternativeBufferSize;
    _alternativeBufferSize = _temporaryBufferSize;
    _temporaryBufferSize = tmp;
}

/*************/
void Geometry::update()
{
    if (!_mesh)
        return; // No mesh, no update

    if (_glBuffers.size() != 4)
        _glBuffers.resize(4);

    // Update the vertex buffers if mesh was updated
    if (_timestamp != _mesh->getTimestamp())
    {
        _mesh->update();

        vector<float> vertices = _mesh->getVertCoords();
        if (vertices.size() == 0)
            return;
        _verticesNumber = vertices.size() / 4;
        _glBuffers[0] = std::shared_ptr<GpuBuffer>(new GpuBuffer(4, GL_FLOAT, GL_STATIC_DRAW, vertices.size(), vertices.data()));
        
        vector<float> texcoords = _mesh->getUVCoords();
        if (texcoords.size() == 0)
            return;
        _glBuffers[1] = std::shared_ptr<GpuBuffer>(new GpuBuffer(2, GL_FLOAT, GL_STATIC_DRAW, texcoords.size(), texcoords.data()));

        vector<float> normals = _mesh->getNormals();
        if (normals.size() == 0)
            return;
        _glBuffers[2] = std::shared_ptr<GpuBuffer>(new GpuBuffer(4, GL_FLOAT, GL_STATIC_DRAW, normals.size(), normals.data()));

        // An additional annexe buffer, to be filled by compute shaders. Contains a vec4 for each vertex
        _glBuffers[3] = std::shared_ptr<GpuBuffer>(new GpuBuffer(4, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber * 4, nullptr));

        // Check the buffers
        bool buffersSet = true;
        for (auto& buffer : _glBuffers)
            if (!*buffer)
                buffersSet = false;

        if (!buffersSet)
        {
            _glBuffers.clear();
            _glBuffers.resize(4);
            return;
        }

        for (auto& v : _vertexArray)
            glDeleteVertexArrays(1, &(v.second));
        _vertexArray.clear();

        _timestamp = _mesh->getTimestamp();

        _buffersDirty = true;
    }

    GLFWwindow* context = glfwGetCurrentContext();
    auto vertexArrayIt = _vertexArray.find(context);
    if (vertexArrayIt == _vertexArray.end() || _buffersDirty)
    {
        if (vertexArrayIt == _vertexArray.end())
        {
            vertexArrayIt = (_vertexArray.emplace(make_pair(context, 0))).first;
            vertexArrayIt->second = 0;
            glGenVertexArrays(1, &(vertexArrayIt->second));
        }

        glBindVertexArray(vertexArrayIt->second);

        for (int idx = 0; idx < _glBuffers.size(); ++idx)
        {
            if (_useAlternativeBuffers)
            {
                glBindBuffer(GL_ARRAY_BUFFER, _glAlternativeBuffers[idx]->getId());
                glVertexAttribPointer((GLuint)idx, _glAlternativeBuffers[idx]->getElementSize(), GL_FLOAT, GL_FALSE, 0, 0);
            }
            else
            {
                glBindBuffer(GL_ARRAY_BUFFER, _glBuffers[idx]->getId());
                glVertexAttribPointer((GLuint)idx, _glBuffers[idx]->getElementSize(), GL_FLOAT, GL_FALSE, 0, 0);
            }
            glEnableVertexAttribArray((GLuint)idx);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        _buffersDirty = false;
    }
}

/*************/
void Geometry::useAlternativeBuffers(bool isActive)
{
    _useAlternativeBuffers = isActive;
    _buffersDirty = true;
}

/*************/
void Geometry::registerAttributes()
{
}

} // end of namespace
