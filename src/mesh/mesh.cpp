#include "./mesh/mesh.h"

#include "./core/root_object.h"
#include "./mesh/meshloader.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
Mesh::Mesh(RootObject* root)
    : BufferObject(root)
{
    init();
    _renderingPriority = Priority::MEDIA;
}

/*************/
Mesh::~Mesh()
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Mesh::~Mesh - Destructor" << Log::endl;
#endif
}

/*************/
void Mesh::init()
{
    _type = "mesh";
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
    std::lock_guard<Spinlock> lock(_readMutex);
    return _mesh.vertices;
}

/*************/
std::vector<float> Mesh::getVertCoordsFlat() const
{
    std::lock_guard<Spinlock> lock(_readMutex);
    const auto vertPtr = reinterpret_cast<const float*>(_mesh.vertices.data());
    const auto vertCount = _mesh.vertices.size() * 4;
    std::vector<float> coords(vertPtr, vertPtr + vertCount);
    return coords;
}

/*************/
std::vector<glm::vec2> Mesh::getUVCoords() const
{
    std::lock_guard<Spinlock> lock(_readMutex);
    return _mesh.uvs;
}

/*************/
std::vector<float> Mesh::getUVCoordsFlat() const
{
    std::lock_guard<Spinlock> lock(_readMutex);
    const auto uvPtr = reinterpret_cast<const float*>(_mesh.uvs.data());
    const auto uvCount = _mesh.uvs.size() * 2;
    std::vector<float> coords(uvPtr, uvPtr + uvCount);
    return coords;
}

/*************/
std::vector<glm::vec4> Mesh::getNormals() const
{
    std::lock_guard<Spinlock> lock(_readMutex);
    return _mesh.normals;
}

/*************/
std::vector<float> Mesh::getNormalsFlat() const
{
    std::lock_guard<Spinlock> lock(_readMutex);
    const auto normalsPtr = reinterpret_cast<const float*>(_mesh.normals.data());
    const auto normalsCount = _mesh.normals.size() * 4;
    std::vector<float> normals(normalsPtr,  normalsPtr + normalsCount);
    return normals;
}

/*************/
std::vector<glm::vec4> Mesh::getAnnexe() const
{
    std::lock_guard<Spinlock> lock(_readMutex);
    return _mesh.annexe;
}

/*************/
std::vector<float> Mesh::getAnnexeFlat() const
{
    std::lock_guard<Spinlock> lock(_readMutex);
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

        std::lock_guard<std::shared_mutex> lock(_writeMutex);
        _mesh = mesh;
        updateTimestamp();
    }

    return true;
}

/*************/
std::shared_ptr<SerializedObject> Mesh::serialize() const
{
    const auto obj = std::make_shared<SerializedObject>();

    if (Timer::get().isDebug())
        Timer::get() << "serialize " + _name;

    // For this, we will use the getVertex, getUV, etc. methods to create a serialized representation of the mesh
    std::vector<std::vector<float>> data;
    data.push_back(getVertCoordsFlat());
    data.push_back(getUVCoordsFlat());
    data.push_back(getNormalsFlat());
    data.push_back(getAnnexeFlat());

    std::lock_guard<Spinlock> lock(_readMutex);
    const int nbrVertices = data[0].size() / 4;
    int totalSize = sizeof(nbrVertices); // We add to all this the total number of vertices
    for (auto& d : data)
        totalSize += d.size() * sizeof(d[0]);
    obj->resize(totalSize);

    auto currentObjPtr = obj->data();
    const char* ptr = reinterpret_cast<const char*>(&nbrVertices);
    std::copy(ptr, ptr + sizeof(nbrVertices), currentObjPtr);
    currentObjPtr += sizeof(nbrVertices);

    for (auto& d : data)
    {
        ptr = reinterpret_cast<const char*>(d.data());
        std::copy(ptr, ptr + d.size() * sizeof(float), currentObjPtr);
        currentObjPtr += d.size() * sizeof(float);
    }

    if (Timer::get().isDebug())
        Timer::get() >> ("serialize " + _name);

    return obj;
}

/*************/
bool Mesh::deserialize(const std::shared_ptr<SerializedObject>& obj)
{
    if (obj.get() == nullptr || obj->size() == 0)
        return false;

    if (Timer::get().isDebug())
        Timer::get() << "deserialize " + _name;

    // First, we get the number of vertices
    int nbrVertices;
    char* ptr = reinterpret_cast<char*>(&nbrVertices);

    auto currentObjPtr = obj->data();
    std::copy(currentObjPtr, currentObjPtr + sizeof(nbrVertices), ptr); // This will fail if float have different size between sender and receiver
    currentObjPtr += sizeof(nbrVertices);

    if (nbrVertices < 0 || nbrVertices > static_cast<int>(obj->size()))
    {
        Log::get() << Log::WARNING << "Mesh::" << __FUNCTION__ << " - Bad buffer received, discarding" << Log::endl;
        return false;
    }

    std::vector<std::vector<float>> data;
    data.push_back(std::vector<float>(nbrVertices * 4));
    data.push_back(std::vector<float>(nbrVertices * 2));
    data.push_back(std::vector<float>(nbrVertices * 4));

    bool hasAnnexe = false;
    if (obj->size() > static_cast<uint32_t>(nbrVertices) * 4 * 14) // Check whether there is an annexe buffer in all this
    {
        hasAnnexe = true;
        data.push_back(std::vector<float>(nbrVertices * 4));
    }

    // Let's read the values
    try
    {
        for (auto& d : data)
        {
            ptr = reinterpret_cast<char*>(d.data());
            std::copy(currentObjPtr, currentObjPtr + d.size() * sizeof(float), ptr);
            currentObjPtr += d.size() * sizeof(float);
        }

        // Next step: use these values to reset the vertices of _mesh
        MeshContainer mesh;

        mesh.vertices.resize(nbrVertices);
        const auto vertPtr = reinterpret_cast<float*>(mesh.vertices.data());
        std::copy(data[0].data(), data[0].data() + nbrVertices * 4, vertPtr);

        mesh.uvs.resize(nbrVertices);
        const auto uvsPtr = reinterpret_cast<float*>(mesh.uvs.data());
        std::copy(data[1].data(), data[1].data() + nbrVertices * 2, uvsPtr);

        mesh.normals.resize(nbrVertices);
        const auto normalsPtr = reinterpret_cast<float*>(mesh.normals.data());
        std::copy(data[2].data(), data[2].data() + nbrVertices * 4, normalsPtr);

        if (hasAnnexe)
        {
            mesh.annexe.resize(nbrVertices);
            const auto annexePtr = reinterpret_cast<float*>(mesh.annexe.data());
            std::copy(data[3].data(), data[3].data() + nbrVertices * 4, annexePtr);
        }

        _bufferMesh = mesh;
        _meshUpdated = true;

        updateTimestamp();
    }
    catch (...)
    {
        createDefaultMesh();
        Log::get()(Log::ERROR, __FUNCTION__, " - Unable to deserialize the given object");
        return false;
    }

    if (Timer::get().isDebug())
        Timer::get() >> ("deserialize " + _name);

    return true;
}

/*************/
void Mesh::update()
{
    if (_meshUpdated)
    {
        std::lock_guard<Spinlock> lock(_readMutex);
        std::shared_lock<std::shared_mutex> lockWrite(_writeMutex);
        _mesh = _bufferMesh;
        _meshUpdated = false;
    }
    else if (_benchmark)
        updateTimestamp();
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

    std::lock_guard<std::shared_mutex> lock(_writeMutex);
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

    addAttribute("benchmark",
        [&](const Values& args) {
            _benchmark = args[0].as<bool>();
            return true;
        },
        {'b'});
    setAttributeDescription("benchmark", "Set to true to resend the image even when not updated");
}

} // namespace Splash
