#include "./mesh/mesh_shmdata.h"

#include "./core/root_object.h"
#include "./utils/osutils.h"
#include "./utils/log.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

/*************/
Mesh_Shmdata::Mesh_Shmdata(RootObject* root)
    : Mesh(root)
{
    init();
}

/*************/
Mesh_Shmdata::~Mesh_Shmdata()
{
}

/*************/
bool Mesh_Shmdata::read(const string& filename)
{
    _reader = make_unique<shmdata::Follower>(filename, [&](void* data, size_t size) { onData(data, size); }, [&](const string& caps) { onCaps(caps); }, [&]() {}, &_logger);

    return true;
}

/*************/
void Mesh_Shmdata::init()
{
    _type = "mesh_shmdata";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;
}

/*************/
void Mesh_Shmdata::onCaps(const string& dataType)
{
    Log::get() << Log::MESSAGE << "Mesh_Shmdata::" << __FUNCTION__ << " - Trying to connect with the following caps: " << dataType << Log::endl;
    if (dataType == "application/x-polymesh")
    {
        _capsIsValid = true;
        Log::get() << Log::MESSAGE << "Mesh_Shmdata::" << __FUNCTION__ << " - Connection successful" << Log::endl;
    }
    else
    {
        _capsIsValid = false;
        Log::get() << Log::MESSAGE << "Mesh_Shmdata::" << __FUNCTION__ << " - Wrong data type" << Log::endl;
    }
}

/*************/
void Mesh_Shmdata::onData(void* data, int /*data_size*/)
{
    if (!_capsIsValid)
        return;

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

    lock_guard<shared_timed_mutex> lock(_writeMutex);
    if (Timer::get().isDebug())
        Timer::get() << "mesh_shmdata " + _name;

    _bufferMesh = std::move(newMesh);
    _meshUpdated = true;
    updateTimestamp();

    if (Timer::get().isDebug())
        Timer::get() >> ("mesh_shmdata " + _name);
}

/*************/
void Mesh_Shmdata::registerAttributes()
{
    Mesh::registerAttributes();
}

} // end of namespace
