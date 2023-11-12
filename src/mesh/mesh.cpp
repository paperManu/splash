#include "./mesh/mesh.h"

#include "./core/root_object.h"
#include "./core/serialize/serialize_mesh.h"
#include "./core/serializer.h"
#include "./mesh/meshloader.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
Mesh::Mesh(RootObject* root, MeshContainer meshContainer)
    : BufferObject(root)
    , _mesh(meshContainer)
{
    init();
}

/*************/
void Mesh::init()
{
    _type = "mesh";
    _renderingPriority = Priority::MEDIA;

    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    createDefaultMesh();
}

/*************/
bool Mesh::operator==(Mesh& otherMesh) const
{
    if (_timestamp != otherMesh.getTimestamp())
        return false;
    return true;
}

/*************/
std::vector<glm::vec4> Mesh::getVertCoords() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    return _mesh.vertices;
}

/*************/
std::vector<float> Mesh::getVertCoordsFlat() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    const auto vertPtr = reinterpret_cast<const float*>(_mesh.vertices.data());
    const auto vertCount = _mesh.vertices.size() * 4;
    std::vector<float> coords(vertPtr, vertPtr + vertCount);
    return coords;
}

/*************/
std::vector<glm::vec2> Mesh::getUVCoords() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    return _mesh.uvs;
}

/*************/
std::vector<float> Mesh::getUVCoordsFlat() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    const auto uvPtr = reinterpret_cast<const float*>(_mesh.uvs.data());
    const auto uvCount = _mesh.uvs.size() * 2;
    std::vector<float> coords(uvPtr, uvPtr + uvCount);
    return coords;
}

/*************/
std::vector<glm::vec4> Mesh::getNormals() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    return _mesh.normals;
}

/*************/
std::vector<float> Mesh::getNormalsFlat() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    const auto normalsPtr = reinterpret_cast<const float*>(_mesh.normals.data());
    const auto normalsCount = _mesh.normals.size() * 4;
    std::vector<float> normals(normalsPtr, normalsPtr + normalsCount);
    return normals;
}

/*************/
std::vector<glm::vec4> Mesh::getAnnexe() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    return _mesh.annexe;
}

/*************/
std::vector<float> Mesh::getAnnexeFlat() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    const auto annexePtr = reinterpret_cast<const float*>(_mesh.annexe.data());
    const auto annexeCount = _mesh.annexe.size() * 2;
    std::vector<float> annexe(annexePtr, annexePtr + annexeCount);
    return annexe;
}

/*************/
bool Mesh::read(const std::string& filename)
{
    if (!_isConnectedToRemote)
    {
        Loader::Obj objLoader;
        if (!objLoader.load(filename))
        {
            Log::get() << Log::WARNING << "Mesh::" << __FUNCTION__ << " - Unable to read the specified mesh file: " << filename << Log::endl;
            return false;
        }

        MeshContainer mesh;
        mesh.vertices = objLoader.getVertices();
        mesh.uvs = objLoader.getUVs();
        mesh.normals = objLoader.getNormals();

        std::lock_guard<std::shared_mutex> readLock(_readMutex);
        _mesh = mesh;
        updateTimestamp();
    }

    return true;
}

/*************/
SerializedObject Mesh::serialize() const
{
    if (Timer::get().isDebug())
        Timer::get() << "serialize " + _name;

    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    auto mesh = _mesh;
    mesh.name = _name;
    std::vector<uint8_t> data;
    Serial::serialize(mesh, data);
    SerializedObject obj(ResizableArray(std::move(data)));

    if (Timer::get().isDebug())
        Timer::get() >> ("serialize " + _name);

    return obj;
}

/*************/
bool Mesh::deserialize(SerializedObject&& obj)
{
    if (obj.size() == 0)
        return false;

    if (Timer::get().isDebug())
        Timer::get() << "deserialize " + _name;

    // After this, obj does not hold any more data
    const auto serializedMesh = obj.grabData();
    auto serializedMeshIt = serializedMesh.cbegin();
    auto mesh = Serial::detail::deserializer<MeshContainer>(serializedMeshIt);

    _bufferMesh = mesh;
    _meshUpdated = true;

    updateTimestamp();

    if (Timer::get().isDebug())
        Timer::get() >> ("deserialize " + _name);

    return true;
}

/*************/
void Mesh::update()
{
    if (_meshUpdated)
    {
        std::lock_guard<Spinlock> updateLock(_updateMutex);
        std::lock_guard<std::shared_mutex> readLock(_readMutex);
        _mesh = _bufferMesh;
        _meshUpdated = false;
    }
    else if (_benchmark)
    {
        updateTimestamp();
    }
}

/*************/
void Mesh::createDefaultMesh(int subdiv)
{
    if (subdiv < 0)
        subdiv = 0;
    _planeSubdivisions = subdiv;

    MeshContainer mesh;

    std::vector<glm::vec2> positions;
    std::vector<glm::vec2> uvs;

    for (int v = 0; v < subdiv + 2; ++v)
    {
        glm::vec2 position;
        glm::vec2 uv;

        uv.y = static_cast<float>(v) / static_cast<float>(subdiv + 1);
        position.y = uv.y * 2.f - 1.f;

        for (int u = 0; u < subdiv + 2; ++u)
        {
            uv.x = static_cast<float>(u) / static_cast<float>(subdiv + 1);
            position.x = uv.x * 2.f - 1.f;

            positions.push_back(position);
            uvs.push_back(uv);
        }
    }

    for (int v = 0; v < subdiv + 1; ++v)
    {
        for (int u = 0; u < subdiv + 1; ++u)
        {
            mesh.vertices.push_back(glm::vec4(positions[u + v * (subdiv + 2)], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(positions[u + 1 + v * (subdiv + 2)], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(positions[u + (v + 1) * (subdiv + 2)], 0.0, 1.0));

            mesh.vertices.push_back(glm::vec4(positions[u + 1 + v * (subdiv + 2)], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(positions[u + 1 + (v + 1) * (subdiv + 2)], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(positions[u + (v + 1) * (subdiv + 2)], 0.0, 1.0));

            mesh.uvs.push_back(uvs[u + v * (subdiv + 2)]);
            mesh.uvs.push_back(uvs[u + 1 + v * (subdiv + 2)]);
            mesh.uvs.push_back(uvs[u + (v + 1) * (subdiv + 2)]);

            mesh.uvs.push_back(uvs[u + 1 + v * (subdiv + 2)]);
            mesh.uvs.push_back(uvs[u + 1 + (v + 1) * (subdiv + 2)]);
            mesh.uvs.push_back(uvs[u + (v + 1) * (subdiv + 2)]);

            for (size_t i = 0; i < 6; ++i)
                mesh.normals.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
        }
    }

    std::lock_guard<std::shared_mutex> readLock(_readMutex);
    _mesh = std::move(mesh);

    updateTimestamp();
}

/*************/
void Mesh::registerAttributes()
{
    BufferObject::registerAttributes();

    addAttribute(
        "file",
        [&](const Values& args) {
            _filepath = args[0].as<std::string>();
            if (_root)
                return read(Utils::getFullPathFromFilePath(_filepath, _root->getConfigurationPath()));
            else
                return true;
        },
        [&]() -> Values { return {_filepath}; },
        {'s'});
    setAttributeDescription("file", "Mesh file to load");

    addAttribute(
        "reload",
        [&](const Values&) {
            if (_root)
                return read(Utils::getFullPathFromFilePath(_filepath, _root->getConfigurationPath()));
            else
                return true;
        },
        [&]() -> Values { return {false}; },
        {});
    setAttributeDescription("reload", "Reload the file");

    addAttribute("benchmark",
        [&](const Values& args) {
            _benchmark = args[0].as<bool>();
            return true;
        },
        {'b'});
    setAttributeDescription("benchmark", "Set to true to resend the image even when not updated");
}

} // namespace Splash
