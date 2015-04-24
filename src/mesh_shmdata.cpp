#include "mesh_shmdata.h"

#include "log.h"
#include "osUtils.h"
#include "timer.h"

using namespace std;

namespace Splash {

/*************/
Mesh_Shmdata::Mesh_Shmdata()
{
    _type = "mesh_shmdata";

    registerAttributes();
}

/*************/
Mesh_Shmdata::~Mesh_Shmdata()
{
    if (_reader != nullptr)
        shmdata_any_reader_close(_reader);
}

/*************/
bool Mesh_Shmdata::read(const string& filename)
{
    auto filepath = string(filename);
    if (Utils::getPathFromFilePath(filepath) == "" || filepath.find(".") == 0)
        filepath = _configFilePath + filepath;

    if (_reader != nullptr)
        shmdata_any_reader_close(_reader);

    _reader = shmdata_any_reader_init();
    shmdata_any_reader_run_gmainloop(_reader, SHMDATA_TRUE);
    shmdata_any_reader_set_on_data_handler(_reader, Mesh_Shmdata::onData, this);
    shmdata_any_reader_start(_reader, filepath.c_str());
    _filename = filename;

    return true;
}

/*************/
void Mesh_Shmdata::onData(shmdata_any_reader_t* reader, void* shmbuf, void* data, int data_size, unsigned long long timestamp,
    const char* type_description, void* user_data)
{
    // Read the number of vertices and polys
    int* intPtr = (int*)data;
    float* floatPtr = (float*)data;

    int verticeNbr = *(intPtr++);
    int polyNbr = *(intPtr++);

    vector<glm::vec4> vertices(verticeNbr);
    vector<glm::vec2> uvs(verticeNbr);
    vector<glm::vec3> normals(verticeNbr);

    floatPtr += 2;
    // First, create the vertices with no UV, normals or faces
    for (int v = 0; v < verticeNbr; ++v)
    {
        vertices[v] = glm::vec4(floatPtr[0], floatPtr[1], floatPtr[2], 1.f);
        uvs[v] = glm::vec2(floatPtr[3], floatPtr[4]);
        normals[v] = glm::vec3(floatPtr[5], floatPtr[6], floatPtr[7]);
        floatPtr += 8;
    }

    intPtr += 8 * verticeNbr;
    // Then create the faces
    MeshContainer newMesh;
    for (int p = 0; p < polyNbr; ++p)
    {
        int size = *(intPtr++);
        // Quads and larger polys are converted to tris through a very simple method
        // This can lead to bad shapes especially for polys larger than quads
        if (size >= 3)
        {
            for (int vert = 0; vert < 3; ++vert)
            {
                newMesh.vertices.push_back(vertices[*(intPtr + vert)]);
                newMesh.uvs.push_back(uvs[*(intPtr + vert)]);
                newMesh.normals.push_back(normals[*(intPtr + vert)]);
            }
        }
        if (size == 4)
        {
            for (int vert = 2; vert < 5; ++vert)
            {
                newMesh.vertices.push_back(vertices[*(intPtr + (vert % 4))]);
                newMesh.uvs.push_back(uvs[*(intPtr + (vert % 4))]);
                newMesh.normals.push_back(normals[*(intPtr + (vert % 4))]);
            }
        }

        intPtr += size;
    }

    Mesh_Shmdata* ctx = reinterpret_cast<Mesh_Shmdata*>(user_data);
    lock_guard<mutex> lock(ctx->_writeMutex);
    if (STimer::timer.isDebug())
        STimer::timer << "mesh_shmdata " + ctx->_name;

    ctx->_bufferMesh = std::move(newMesh);
    ctx->_meshUpdated = true;
    ctx->updateTimestamp();

    shmdata_any_reader_free(shmbuf);

    if (STimer::timer.isDebug())
        STimer::timer >> "mesh_shmdata " + ctx->_name;
}

/*************/
void Mesh_Shmdata::registerAttributes()
{
    _attribFunctions["file"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        return read(args[0].asString());
    });
}

} // end of namespace
