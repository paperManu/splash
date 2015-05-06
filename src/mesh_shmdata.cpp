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
}

/*************/
bool Mesh_Shmdata::read(const string& filename)
{
    auto filepath = string(filename);
    if (Utils::getPathFromFilePath(filepath) == "" || filepath.find(".") == 0)
        filepath = _configFilePath + filepath;

    _reader.reset(new shmdata::Follower(filepath,
                                        [&](void* data, size_t size) {
                                            onData(data, size, this);
                                        },
                                        [&](const string& caps) {
                                            _caps = caps;
                                        },
                                        [&](){},
                                        &_logger));

    _filename = filename;

    return true;
}

/*************/
void Mesh_Shmdata::onData(void* data, int data_size, void* user_data)
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
    if (Timer::get().isDebug())
        Timer::get() << "mesh_shmdata " + ctx->_name;

    ctx->_bufferMesh = std::move(newMesh);
    ctx->_meshUpdated = true;
    ctx->updateTimestamp();

    if (Timer::get().isDebug())
        Timer::get() >> "mesh_shmdata " + ctx->_name;
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
