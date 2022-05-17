#include "./graphics/geometry.h"

#include "./core/scene.h"
#include "./core/serialize/serialize_mesh.h"
#include "./mesh/mesh.h"
#include "./utils/log.h"

using namespace glm;

namespace chrono = std::chrono;

namespace Splash
{

/*************/
Geometry::Geometry(RootObject* root)
    : BufferObject(root)
{
    init();

    auto scene = dynamic_cast<Scene*>(_root);
    if (scene)
        _onMasterScene = scene->isMaster();
}

/*************/
void Geometry::init()
{
    _type = "geometry";
    registerAttributes();

    // If the root object weak_ptr is expired, this means that
    // this object has been created outside of a World or Scene.
    // This is used for getting documentation "offline"
    if (!_root)
        return;

    glCreateQueries(GL_PRIMITIVES_GENERATED, 1, &_feedbackQuery);

    _mesh = std::make_shared<Mesh>(_root);
    update();
    _timestamp = _mesh->getTimestamp();
}

/*************/
Geometry::~Geometry()
{
    if (!_root)
        return;

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
    if (_glTemporaryBuffers.size() < _glBuffers.size() || _buffersDirty || _feedbackMaxNbrPrimitives * 6 > _temporaryBufferSize)
    {
        _temporaryBufferSize = _feedbackMaxNbrPrimitives * 6; // 3 vertices per primitive, times two to keep some margin for future updates
        for (size_t i = 0; i < _glBuffers.size(); ++i)
        {
            // This creates a copy of the buffer
            auto altBuffer = std::make_shared<GpuBuffer>(*_glBuffers[i]);
            altBuffer->resize(_temporaryBufferSize);
            _glTemporaryBuffers[i] = altBuffer;
        }
        _buffersResized = true;
    }
    else
    {
        _buffersResized = false;
    }

    for (unsigned int i = 0; i < _glTemporaryBuffers.size(); ++i)
    {
        _glTemporaryBuffers[i]->clear();
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, _glTemporaryBuffers[i]->getId());
    }

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
    int drawnPrimitives = 0;
    while (true)
    {
        glGetQueryObjectiv(_feedbackQuery, GL_QUERY_RESULT_AVAILABLE, &drawnPrimitives);
        if (drawnPrimitives != 0)
            break;
        std::this_thread::sleep_for(chrono::microseconds(500));
    }

    glGetQueryObjectiv(_feedbackQuery, GL_QUERY_RESULT, &drawnPrimitives);
    _feedbackMaxNbrPrimitives = std::max(_feedbackMaxNbrPrimitives, drawnPrimitives);
    _temporaryVerticesNumber = drawnPrimitives * 3;
}

/*************/
std::vector<char> Geometry::getGpuBufferAsVector(Geometry::BufferType type)
{
    auto typeId = static_cast<int>(type);
    if (_useAlternativeBuffers && _glAlternativeBuffers[typeId] != nullptr)
        return _glAlternativeBuffers[typeId]->getBufferAsVector();
    else
        return _glBuffers[typeId]->getBufferAsVector();
}

/*************/
SerializedObject Geometry::serialize() const
{
    if (std::any_of(_glAlternativeBuffers.cbegin(), _glAlternativeBuffers.cend(), [](const auto& buffer) { return buffer == nullptr; }))
        return {};

    const auto vertices = _glAlternativeBuffers[0]->getBufferAsVector(_alternativeVerticesNumber);
    const auto uvs = _glAlternativeBuffers[1]->getBufferAsVector(_alternativeVerticesNumber);
    const auto normals = _glAlternativeBuffers[2]->getBufferAsVector(_alternativeVerticesNumber);
    const auto annexe = _glAlternativeBuffers[3]->getBufferAsVector(_alternativeVerticesNumber);

    const auto verticesData = reinterpret_cast<const glm::vec4*>(vertices.data());
    const auto uvsData = reinterpret_cast<const glm::vec2*>(uvs.data());
    const auto normalsData = reinterpret_cast<const glm::vec4*>(normals.data());
    const auto annexeData = reinterpret_cast<const glm::vec4*>(annexe.data());

    const Mesh::MeshContainer mesh = {
        .name = _name,
        .vertices = std::vector<glm::vec4>(verticesData, verticesData + _alternativeVerticesNumber),
        .uvs = std::vector<glm::vec2>(uvsData, uvsData + _alternativeVerticesNumber),
        .normals = std::vector<glm::vec4>(normalsData, normalsData + _alternativeVerticesNumber),
        .annexe = std::vector<glm::vec4>(annexeData, annexeData + _alternativeVerticesNumber),
    };

    std::vector<uint8_t> data;
    Serial::serialize(mesh, data);
    SerializedObject serializedObject(ResizableArray(std::move(data)));

    return serializedObject;
}

/*************/
bool Geometry::deserialize(SerializedObject&& obj)
{
    // After this, obj does not hold any more data
    const auto serializedMesh = obj.grabData();
    auto serializedMeshIt = serializedMesh.cbegin();
    auto mesh = Serial::detail::deserializer<Mesh::MeshContainer>(serializedMeshIt);

    bool doMatch = (mesh.vertices.size() == mesh.uvs.size());
    doMatch = (mesh.vertices.size() == mesh.normals.size());
    doMatch = (mesh.vertices.size() == mesh.annexe.size());

    if (!doMatch)
    {
        Log::get() << Log::WARNING << "Geometry::" << __FUNCTION__ << " - Received buffers size do not match. Dropping." << Log::endl;
        return false;
    }

    _deserializedMesh = std::make_unique<Mesh::MeshContainer>(std::move(mesh));
    return true;
}

/*************/
bool Geometry::linkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (std::dynamic_pointer_cast<Mesh>(obj))
    {
        std::shared_ptr<Mesh> mesh = std::dynamic_pointer_cast<Mesh>(obj);
        setMesh(mesh);
        return true;
    }

    return false;
}

/*************/
float Geometry::pickVertex(dvec3 p, dvec3& v)
{
    float distance = std::numeric_limits<float>::max();
    dvec3 closestVertex;

    assert(_mesh);
    std::vector<float> vertices = _mesh->getVertCoordsFlat();
    for (uint32_t i = 0; i < vertices.size(); i += 4)
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
    assert(_mesh);

    // Update the vertex buffers if mesh was updated
    if (_timestamp != _mesh->getTimestamp())
    {
        _mesh->update();

        std::vector<float> vertices = _mesh->getVertCoordsFlat();
        if (vertices.empty())
            return;

        _verticesNumber = vertices.size() / 4;
        _glBuffers[0] = std::make_shared<GpuBuffer>(4, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, vertices.data());

        std::vector<float> texcoords = _mesh->getUVCoordsFlat();
        if (!texcoords.empty())
            _glBuffers[1] = std::make_shared<GpuBuffer>(2, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, texcoords.data());
        else
            _glBuffers[1] = std::make_shared<GpuBuffer>(2, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, nullptr);

        std::vector<float> normals = _mesh->getNormalsFlat();
        if (!normals.empty())
            _glBuffers[2] = std::make_shared<GpuBuffer>(4, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, normals.data());
        else
            _glBuffers[2] = std::make_shared<GpuBuffer>(4, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, nullptr);

        // An additional annexe buffer, to be filled by compute shaders. Contains a vec4 for each vertex
        std::vector<float> annexe = _mesh->getAnnexeFlat();
        if (!annexe.empty())
            _glBuffers[3] = std::make_shared<GpuBuffer>(4, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, annexe.data());
        else
            _glBuffers[3] = std::make_shared<GpuBuffer>(4, GL_FLOAT, GL_STATIC_DRAW, _verticesNumber, nullptr);

        for (auto& v : _vertexArray)
            glDeleteVertexArrays(1, &(v.second));
        _vertexArray.clear();

        _timestamp = _mesh->getTimestamp();

        _buffersDirty = true;
    }

    if (_glBuffers.empty())
        return;

    // If a serialized geometry is present, we use it as the alternative buffer
    if (!_onMasterScene && _deserializedMesh != nullptr)
    {
        std::lock_guard<Spinlock> updateLock(_updateMutex);

        _temporaryVerticesNumber = _deserializedMesh->vertices.size();
        _temporaryBufferSize = _temporaryVerticesNumber;

        if (!_glTemporaryBuffers[0])
        {
            _glTemporaryBuffers[0] =
                std::make_shared<GpuBuffer>(4, GL_FLOAT, GL_STATIC_DRAW, _temporaryVerticesNumber, reinterpret_cast<GLvoid*>(_deserializedMesh->vertices.data()));
        }
        else
        {
            const auto data = reinterpret_cast<char*>(_deserializedMesh->vertices.data());
            _glTemporaryBuffers[0]->setBufferFromVector({data, data + _temporaryVerticesNumber * sizeof(float) * 4});
        }

        if (!_glTemporaryBuffers[1])
        {
            _glTemporaryBuffers[1] = std::make_shared<GpuBuffer>(2, GL_FLOAT, GL_STATIC_DRAW, _temporaryVerticesNumber, reinterpret_cast<GLvoid*>(_deserializedMesh->uvs.data()));
        }
        else
        {
            const auto data = reinterpret_cast<char*>(_deserializedMesh->uvs.data());
            _glTemporaryBuffers[1]->setBufferFromVector({data, data + _temporaryVerticesNumber * sizeof(float) * 2});
        }

        if (!_glTemporaryBuffers[2])
        {
            _glTemporaryBuffers[2] =
                std::make_shared<GpuBuffer>(4, GL_FLOAT, GL_STATIC_DRAW, _temporaryVerticesNumber, reinterpret_cast<GLvoid*>(_deserializedMesh->normals.data()));
        }
        else
        {
            const auto data = reinterpret_cast<char*>(_deserializedMesh->normals.data());
            _glTemporaryBuffers[2]->setBufferFromVector({data, data + _temporaryVerticesNumber * sizeof(float) * 4});
        }

        if (!_glTemporaryBuffers[3])
        {
            _glTemporaryBuffers[3] = std::make_shared<GpuBuffer>(4, GL_FLOAT, GL_STATIC_DRAW, _temporaryVerticesNumber, _deserializedMesh->annexe.data());
        }
        else
        {
            const auto data = reinterpret_cast<char*>(_deserializedMesh->annexe.data());
            _glTemporaryBuffers[3]->setBufferFromVector({data, data + _temporaryVerticesNumber * sizeof(float) * 4});
        }

        swapBuffers();
        _buffersDirty = true;
        _deserializedMesh.reset();
    }

    GLFWwindow* context = glfwGetCurrentContext();
    auto vertexArrayIt = _vertexArray.find(context);
    if (vertexArrayIt == _vertexArray.end() || _buffersDirty)
    {
        if (vertexArrayIt == _vertexArray.end())
        {
            vertexArrayIt = (_vertexArray.emplace(std::make_pair(context, 0))).first;
            vertexArrayIt->second = 0;
            glCreateVertexArrays(1, &(vertexArrayIt->second));
        }

        glBindVertexArray(vertexArrayIt->second);

        for (uint32_t idx = 0; idx < _glBuffers.size(); ++idx)
        {
            if (_useAlternativeBuffers && _glAlternativeBuffers[0] != nullptr)
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
    BufferObject::registerAttributes();
}

} // namespace Splash
