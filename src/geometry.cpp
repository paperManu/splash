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

    _mesh = make_shared<Mesh>();
    update();
    _timestamp = _mesh->getTimestamp();
}

/*************/
Geometry::~Geometry()
{
    for (auto v : _vertexArray)
        glDeleteVertexArrays(1, &(v.second));

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

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _glBuffers[0]->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _glBuffers[1]->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _glBuffers[2]->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _glBuffers[3]->getId());
}

/*************/
void Geometry::deactivate() const
{
#ifdef DEBUG
    glBindVertexArray(0);
#endif
    _mutex.unlock();
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
void Geometry::setAlternativeBuffer(shared_ptr<GpuBuffer> buffer, int index)
{
    if (index >= 4)
        return;

    _glAlternativeBuffers[index] = buffer;
    _buffersDirty = true;
}

/*************/
void Geometry::resetAlternativebuffer(int index)
{
    if (index >= 4)
    {
        return;
    }
    else if (index >= 0)
    {
        _glAlternativeBuffers[index].reset();
    }
    else
    {
        for (int i = 0; i < _glAlternativeBuffers.size(); ++i)
            _glAlternativeBuffers[i].reset();
    }

    _buffersDirty = true;
}

/*************/
void Geometry::update()
{
    if (!_mesh)
        return; // No mesh, no update

    if (_glBuffers.size() != 4)
        _glBuffers.resize(4);
    if (_glAlternativeBuffers.size() != 4)
        _glAlternativeBuffers.resize(4);

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

        for (int idx = 0; idx < 4; ++idx)
        {
            if (_glAlternativeBuffers[idx])
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
void Geometry::registerAttributes()
{
}

} // end of namespace
