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

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _vertexCoords->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _texCoords->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _normals->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _annexe->getId());
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
void Geometry::update()
{
    if (!_mesh)
        return; // No mesh, no update

    // Update the vertex buffers if mesh was updated
    if (_timestamp != _mesh->getTimestamp())
    {
        _mesh->update();

        vector<float> vertices = _mesh->getVertCoords();
        if (vertices.size() == 0)
            return;
        _verticesNumber = vertices.size() / 4;
        _vertexCoords = std::unique_ptr<GpuBuffer>(new GpuBuffer(4, GL_FLOAT, GL_STATIC_DRAW, vertices.size(), vertices.data()));
        
        vector<float> texcoords = _mesh->getUVCoords();
        if (texcoords.size() == 0)
            return;
        _texCoords = std::unique_ptr<GpuBuffer>(new GpuBuffer(2, GL_FLOAT, GL_STATIC_DRAW, texcoords.size(), texcoords.data()));

        vector<float> normals = _mesh->getNormals();
        if (normals.size() == 0)
            return;
        _normals = std::unique_ptr<GpuBuffer>(new GpuBuffer(4, GL_FLOAT, GL_STATIC_DRAW, normals.size(), normals.data()));

        // An additional annexe buffer, to be filled by compute shaders. Contains a vec4 for each vertex
        _annexe = std::unique_ptr<GpuBuffer>(new GpuBuffer(4, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber * 4, nullptr));

        if (!*_vertexCoords || !*_texCoords || !*_normals || !*_annexe)
        {
            _vertexCoords.reset();
            _texCoords.reset();
            _normals.reset();
            _annexe.reset();
            return;
        }

        for (auto& v : _vertexArray)
            glDeleteVertexArrays(1, &(v.second));
        _vertexArray.clear();

        _timestamp = _mesh->getTimestamp();
    }

    GLFWwindow* context = glfwGetCurrentContext();
    auto vertexArrayIt = _vertexArray.find(context);
    if (vertexArrayIt == _vertexArray.end())
    {
        vertexArrayIt = (_vertexArray.emplace(make_pair(context, 0))).first;
        vertexArrayIt->second = 0;
        
        glGenVertexArrays(1, &(vertexArrayIt->second));
        glBindVertexArray(vertexArrayIt->second);

        glBindBuffer(GL_ARRAY_BUFFER, _vertexCoords->getId());
        glVertexAttribPointer((GLuint)0, _vertexCoords->getElementSize(), GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray((GLuint)0);

        glBindBuffer(GL_ARRAY_BUFFER, _texCoords->getId());
        glVertexAttribPointer((GLuint)1, _texCoords->getElementSize(), GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray((GLuint)1);

        glBindBuffer(GL_ARRAY_BUFFER, _normals->getId());
        glVertexAttribPointer((GLuint)2, _normals->getElementSize(), GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray((GLuint)2);

        glBindBuffer(GL_ARRAY_BUFFER, _annexe->getId());
        glVertexAttribPointer((GLuint)3, _annexe->getElementSize(), GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray((GLuint)3);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

/*************/
void Geometry::registerAttributes()
{
}

} // end of namespace
