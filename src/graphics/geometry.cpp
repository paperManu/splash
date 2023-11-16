#include "./graphics/geometry.h"

#include <memory>

#include "./core/scene.h"
#include "./core/serialize/serialize_mesh.h"
#include "./graphics/api/geometry_gfx_impl.h"
#include "./mesh/mesh.h"
#include "./utils/log.h"

using namespace glm;

namespace Splash
{

/*************/
Geometry::Geometry(RootObject* root, std::unique_ptr<gfx::GeometryGfxImpl> gfxImpl)
    : BufferObject(root)
    , _gfxImpl(std::move(gfxImpl))
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

    _mesh = std::make_shared<Mesh>(_root);
    update();
    _timestamp = _mesh->getTimestamp();
}

/*************/
Geometry::~Geometry()
{
    if (!_root)
        return;

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Geometry::~Geometry - Destructor" << Log::endl;
#endif
}

/*************/
void Geometry::activate()
{
    _gfxImpl->activate();
}

/*************/
void Geometry::activateAsSharedBuffer()
{
    _gfxImpl->activateAsSharedBuffer();
}

/*************/
void Geometry::activateForFeedback()
{
    // TODO: Move this to the impl?
    const bool shouldResize = _buffersDirty || _gfxImpl->buffersTooSmall();

    if (shouldResize)
        _gfxImpl->resizeTempBuffers();

    _buffersResized = shouldResize;

    _gfxImpl->activateForFeedback();
}

/*************/
void Geometry::deactivate() const
{
    _gfxImpl->deactivate();
}

/*************/
void Geometry::deactivateFeedback()
{
    _gfxImpl->deactivateFeedback();
}

/*************/
uint Geometry::getVerticesNumber() const
{
    return _gfxImpl->getVerticesNumber();
}

/*************/
std::vector<char> Geometry::getGpuBufferAsVector(Geometry::BufferType type, bool forceAlternativeBuffer) const
{
    return _gfxImpl->getGpuBufferAsVector(static_cast<uint8_t>(type), forceAlternativeBuffer);
}

/*************/
SerializedObject Geometry::serialize() const
{
    const auto mesh = _gfxImpl->serialize(_name);

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
    doMatch &= (mesh.vertices.size() == mesh.normals.size());
    doMatch &= (mesh.vertices.size() == mesh.annexe.size());

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
    _gfxImpl->swapBuffers();
}

/*************/
void Geometry::updateBuffers()
{
    _mesh->update();

    std::vector<float> vertices = _mesh->getVertCoordsFlat();
    if (vertices.empty())
        return;

    _gfxImpl->initVertices(vertices.data(), vertices.size() / 4);

    std::vector<float> texcoords = _mesh->getUVCoordsFlat();
    _gfxImpl->allocateOrInitBuffer(BufferType::TexCoords, 2, texcoords);

    std::vector<float> normals = _mesh->getNormalsFlat();
    _gfxImpl->allocateOrInitBuffer(BufferType::Normal, 4, normals);

    // An additional annexe buffer, to be filled by compute shaders. Contains a vec4 for each vertex
    std::vector<float> annexe = _mesh->getAnnexeFlat();
    _gfxImpl->allocateOrInitBuffer(BufferType::Annexe, 4, annexe);

    _gfxImpl->clearFromAllContexts();

    _timestamp = _mesh->getTimestamp();

    _buffersDirty = true;
}

/*************/

void Geometry::updateTemporaryBuffers()
{
    std::lock_guard<Spinlock> updateLock(_updateMutex);

    _gfxImpl->updateTemporaryBuffers(_deserializedMesh.get());

    swapBuffers();
    _buffersDirty = true;
    _deserializedMesh.reset();
}

/*************/
void Geometry::update()
{
    assert(_mesh);

    // Update the vertex buffers if mesh was updated
    if (_timestamp != _mesh->getTimestamp())
        updateBuffers();

    // If a serialized geometry is present, we use it as the alternative buffer
    if (!_onMasterScene && _deserializedMesh != nullptr)
        updateTemporaryBuffers();

    _gfxImpl->update(_buffersDirty);
    _buffersDirty = false;
}

/*************/
void Geometry::useAlternativeBuffers(bool isActive)
{
    _gfxImpl->useAlternativeBuffers(isActive);
    _buffersDirty = true;
}

/*************/
void Geometry::registerAttributes()
{
    BufferObject::registerAttributes();
}

} // namespace Splash
