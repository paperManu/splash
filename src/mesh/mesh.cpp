#include "./mesh/mesh.h"

#include "./core/root_object.h"
#include "./mesh/meshloader.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

using namespace std;

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
vector<float> Mesh::getVertCoords() const
{
    lock_guard<Spinlock> lock(_readMutex);
    vector<float> coords;
    for (auto& v : _mesh.vertices)
    {
        coords.push_back(v[0]);
        coords.push_back(v[1]);
        coords.push_back(v[2]);
        coords.push_back(v[3]);
    }
    return coords;
}

/*************/
vector<float> Mesh::getUVCoords() const
{
    lock_guard<Spinlock> lock(_readMutex);
    vector<float> coords;
    for (auto& u : _mesh.uvs)
    {
        coords.push_back(u[0]);
        coords.push_back(u[1]);
    }
    return coords;
}

/*************/
vector<float> Mesh::getNormals() const
{
    lock_guard<Spinlock> lock(_readMutex);
    vector<float> normals;
    for (auto& n : _mesh.normals)
    {
        normals.push_back(n[0]);
        normals.push_back(n[1]);
        normals.push_back(n[2]);
        normals.push_back(0.f);
    }
    return normals;
}

/*************/
vector<float> Mesh::getAnnexe() const
{
    lock_guard<Spinlock> lock(_readMutex);
    vector<float> annexe;
    for (auto& a : _mesh.annexe)
    {
        annexe.push_back(a[0]);
        annexe.push_back(a[1]);
        annexe.push_back(a[2]);
        annexe.push_back(a[3]);
    }
    return annexe;
}

/*************/
bool Mesh::read(const string& filename)
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

        lock_guard<shared_timed_mutex> lock(_writeMutex);
        _mesh = mesh;
        updateTimestamp();
    }

    return true;
}

/*************/
shared_ptr<SerializedObject> Mesh::serialize() const
{
    auto obj = make_shared<SerializedObject>();

    if (Timer::get().isDebug())
        Timer::get() << "serialize " + _name;

    // For this, we will use the getVertex, getUV, etc. methods to create a serialized representation of the mesh
    vector<vector<float>> data;
    data.push_back(getVertCoords());
    data.push_back(getUVCoords());
    data.push_back(getNormals());
    data.push_back(getAnnexe());

    lock_guard<Spinlock> lock(_readMutex);
    int nbrVertices = data[0].size() / 4;
    int totalSize = sizeof(nbrVertices); // We add to all this the total number of vertices
    for (auto& d : data)
        totalSize += d.size() * sizeof(d[0]);
    obj->resize(totalSize);

    auto currentObjPtr = obj->data();
    const char* ptr = reinterpret_cast<const char*>(&nbrVertices);
    copy(ptr, ptr + sizeof(nbrVertices), currentObjPtr);
    currentObjPtr += sizeof(nbrVertices);

    for (auto& d : data)
    {
        ptr = reinterpret_cast<const char*>(d.data());
        copy(ptr, ptr + d.size() * sizeof(float), currentObjPtr);
        currentObjPtr += d.size() * sizeof(float);
    }

    if (Timer::get().isDebug())
        Timer::get() >> ("serialize " + _name);

    return obj;
}

/*************/
bool Mesh::deserialize(const shared_ptr<SerializedObject>& obj)
{
    if (obj.get() == nullptr || obj->size() == 0)
        return false;

    if (Timer::get().isDebug())
        Timer::get() << "deserialize " + _name;

    // First, we get the number of vertices
    int nbrVertices;
    char* ptr = reinterpret_cast<char*>(&nbrVertices);

    auto currentObjPtr = obj->data();
    copy(currentObjPtr, currentObjPtr + sizeof(nbrVertices), ptr); // This will fail if float have different size between sender and receiver
    currentObjPtr += sizeof(nbrVertices);

    if (nbrVertices < 0 || nbrVertices > static_cast<int>(obj->size()))
    {
        Log::get() << Log::WARNING << "Mesh::" << __FUNCTION__ << " - Bad buffer received, discarding" << Log::endl;
        return false;
    }

    vector<vector<float>> data;
    data.push_back(vector<float>(nbrVertices * 4));
    data.push_back(vector<float>(nbrVertices * 2));
    data.push_back(vector<float>(nbrVertices * 4));

    bool hasAnnexe = false;
    if (obj->size() > static_cast<uint32_t>(nbrVertices) * 4 * 14) // Check whether there is an annexe buffer in all this
    {
        hasAnnexe = true;
        data.push_back(vector<float>(nbrVertices * 4));
    }

    // Let's read the values
    try
    {
        for (auto& d : data)
        {
            ptr = reinterpret_cast<char*>(d.data());
            copy(currentObjPtr, currentObjPtr + d.size() * sizeof(float), ptr);
            currentObjPtr += d.size() * sizeof(float);
        }

        // Next step: use these values to reset the vertices of _mesh
        MeshContainer mesh;

        mesh.vertices.resize(nbrVertices);
        for (int i = 0; i < nbrVertices; ++i)
        {
            mesh.vertices[i][0] = data[0][i * 4 + 0];
            mesh.vertices[i][1] = data[0][i * 4 + 1];
            mesh.vertices[i][2] = data[0][i * 4 + 2];
            mesh.vertices[i][3] = data[0][i * 4 + 3];
        }

        mesh.uvs.resize(nbrVertices);
        for (int i = 0; i < nbrVertices; ++i)
        {
            mesh.uvs[i][0] = data[1][i * 2 + 0];
            mesh.uvs[i][1] = data[1][i * 2 + 1];
        }

        mesh.normals.resize(nbrVertices);
        for (int i = 0; i < nbrVertices; ++i)
        {
            mesh.normals[i][0] = data[2][i * 4 + 0];
            mesh.normals[i][1] = data[2][i * 4 + 1];
            mesh.normals[i][2] = data[2][i * 4 + 2];
        }

        if (hasAnnexe)
        {
            mesh.annexe.resize(nbrVertices);
            for (int i = 0; i < nbrVertices; ++i)
            {
                mesh.annexe[i][0] = data[3][i * 4 + 0];
                mesh.annexe[i][1] = data[3][i * 4 + 1];
                mesh.annexe[i][2] = data[3][i * 4 + 2];
                mesh.annexe[i][3] = data[3][i * 4 + 3];
            }
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
        lock_guard<Spinlock> lock(_readMutex);
        shared_lock<shared_timed_mutex> lockWrite(_writeMutex);
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

    vector<glm::vec2> positions;
    vector<glm::vec2> uvs;

    for (int v = 0; v < subdiv + 2; ++v)
    {
        glm::vec2 position;
        glm::vec2 uv;

        uv.y = (float)v / ((float)(subdiv + 1));
        position.y = uv.y * 2.f - 1.f;

        for (int u = 0; u < subdiv + 2; ++u)
        {
            uv.x = (float)u / ((float)(subdiv + 1));
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

            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
        }
    }

    lock_guard<shared_timed_mutex> lock(_writeMutex);
    _mesh = std::move(mesh);

    updateTimestamp();
}

/*************/
void Mesh::registerAttributes()
{
    BufferObject::registerAttributes();

    addAttribute("file",
        [&](const Values& args) {
            _filepath = args[0].as<string>();
            return read(Utils::getFullPathFromFilePath(_filepath, _root->getConfigurationPath()));
        },
        [&]() -> Values { return {_filepath}; },
        {'s'});
    setAttributeDescription("file", "Mesh file to load");

    addAttribute("benchmark",
        [&](const Values& args) {
            if (args[0].as<int>() > 0)
                _benchmark = true;
            else
                _benchmark = false;
            return true;
        },
        {'n'});
    setAttributeDescription("benchmark", "Set to 1 to resend the image even when not updated");
}

} // end of namespace
