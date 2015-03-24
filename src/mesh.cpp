#include "mesh.h"

#include "log.h"
#include "meshLoader.h"
#include "osUtils.h"
#include "timer.h"

using namespace std;

namespace Splash {

/*************/
Mesh::Mesh()
{
    _type = "mesh";

    createDefaultMesh();

    registerAttributes();
}

/*************/
Mesh::~Mesh()
{
#ifdef DEBUG
    SLog::log << Log::DEBUGGING << "Mesh::~Mesh - Destructor" << Log::endl;
#endif
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
    lock_guard<mutex> lock(_readMutex);
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
    lock_guard<mutex> lock(_readMutex);
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
    lock_guard<mutex> lock(_readMutex);
    vector<float> normals;
    for (auto& n : _mesh.normals)
    {
        normals.push_back(n[0]);
        normals.push_back(n[1]);
        normals.push_back(n[2]);
    }
    return normals;
}

/*************/
bool Mesh::read(const string& filename)
{
    auto filepath = string(filename);
    if (Utils::getPathFromFilePath(filepath) == "" || filepath.find(".") == 0)
        filepath = _configFilePath + filepath;

    Loader::Obj objLoader;
    if (!objLoader.load(filepath))
    {
        SLog::log << Log::WARNING << "Mesh::" << __FUNCTION__ << " - Unable to read the specified mesh file" << Log::endl;
        return false;
    }

    MeshContainer mesh;
    mesh.vertices = objLoader.getVertices();
    mesh.uvs = objLoader.getUVs();
    mesh.normals = objLoader.getNormals();

    lock_guard<mutex> lock(_writeMutex);
    _mesh = mesh;
    updateTimestamp();

    return true;
}

/*************/
unique_ptr<SerializedObject> Mesh::serialize() const
{
    unique_ptr<SerializedObject> obj(new SerializedObject());

    STimer::timer << "serialize " + _name;

    // For this, we will use the getVertex, getUV, etc. methods to create a serialized representation of the mesh
    vector<vector<float>> data;
    data.push_back(move(getVertCoords()));
    data.push_back(move(getUVCoords()));
    data.push_back(move(getNormals()));

    lock_guard<mutex> lock(_readMutex);
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
    
    STimer::timer >> "serialize " + _name;

    return obj;
}

/*************/
bool Mesh::deserialize(unique_ptr<SerializedObject> obj)
{
    if (obj.get() == nullptr || obj->size() == 0)
        return false;

    lock_guard<mutex> lock(_writeMutex);

    STimer::timer << "deserialize " + _name;

    // First, we get the number of vertices
    int nbrVertices;
    char* ptr = reinterpret_cast<char*>(&nbrVertices);

    auto currentObjPtr = obj->data();
    copy(currentObjPtr, currentObjPtr + sizeof(nbrVertices), ptr); // This will fail if float have different size between sender and receiver
    currentObjPtr += sizeof(nbrVertices);

    if (nbrVertices < 0 || nbrVertices > obj->size())
    {
        SLog::log << Log::WARNING << "Mesh::" << __FUNCTION__ << " - Bad buffer received, discarding" << Log::endl;
        return false;
    }

    vector<vector<float>> data;
    data.push_back(vector<float>(nbrVertices * 4));
    data.push_back(vector<float>(nbrVertices * 2));
    data.push_back(vector<float>(nbrVertices * 3));

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

        mesh.vertices.resize(data[0].size() / 4);
        for (unsigned int i = 0; i < data[0].size() / 4; ++i)
        {
            mesh.vertices[i][0] = data[0][i*4 + 0];
            mesh.vertices[i][1] = data[0][i*4 + 1];
            mesh.vertices[i][2] = data[0][i*4 + 2];
            mesh.vertices[i][3] = data[0][i*4 + 3];
        }

        mesh.uvs.resize(data[1].size() / 2);
        for (unsigned int i = 0; i < data[1].size() / 2; ++i)
        {
            mesh.uvs[i][0] = data[1][i*2 + 0];
            mesh.uvs[i][1] = data[1][i*2 + 1];
        }

        mesh.normals.resize(data[2].size() / 3);
        for (unsigned int i = 0; i < data[2].size() / 3; ++i)
        {
            mesh.normals[i][0] = data[0][i*3 + 0];
            mesh.normals[i][1] = data[0][i*3 + 1];
            mesh.normals[i][2] = data[0][i*3 + 2];
        }

        _bufferMesh = mesh;
        _meshUpdated = true;

        updateTimestamp();
    }
    catch (...)
    {
        createDefaultMesh();
        SLog::log(Log::ERROR, __FUNCTION__, " - Unable to deserialize the given object");
        return false;
    }

    STimer::timer >> "deserialize " + _name;

    return true;
}

/*************/
void Mesh::update()
{
    lock_guard<mutex> lockRead(_readMutex);
    lock_guard<mutex> lockWrite(_writeMutex);
    if (_meshUpdated)
    {
        _mesh = _bufferMesh;
        _meshUpdated = false;
    }
    else if (_benchmark)
        updateTimestamp();
}

/*************/
void Mesh::createDefaultMesh()
{
    MeshContainer mesh;

    // Create the vertices
    mesh.vertices.push_back(glm::vec4(-1.0, -1.0, 0.0, 1.0));
    mesh.vertices.push_back(glm::vec4(1.0, -1.0, 0.0, 1.0));
    mesh.vertices.push_back(glm::vec4(1.0, 1.0, 0.0, 1.0));
    mesh.vertices.push_back(glm::vec4(1.0, 1.0, 0.0, 1.0));
    mesh.vertices.push_back(glm::vec4(-1.0, 1.0, 0.0, 1.0));
    mesh.vertices.push_back(glm::vec4(-1.0, -1.0, 0.0, 1.0));

    mesh.uvs.push_back(glm::vec2(0.0, 0.0));
    mesh.uvs.push_back(glm::vec2(1.0, 0.0));
    mesh.uvs.push_back(glm::vec2(1.0, 1.0));
    mesh.uvs.push_back(glm::vec2(1.0, 1.0));
    mesh.uvs.push_back(glm::vec2(0.0, 1.0));
    mesh.uvs.push_back(glm::vec2(0.0, 0.0));

    mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
    mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
    mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
    mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
    mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
    mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));

    lock_guard<mutex> lock(_writeMutex);
    _mesh = std::move(mesh);

    updateTimestamp();
}

/*************/
void Mesh::registerAttributes()
{
    _attribFunctions["file"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        return read(args[0].asString());
    });

    _attribFunctions["name"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        _name = args[0].asString();
        return true;
    });
    
    _attribFunctions["benchmark"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        if (args[0].asInt() > 0)
            _benchmark = true;
        else
            _benchmark = false;
        return true;
    });
}

} // end of namespace
