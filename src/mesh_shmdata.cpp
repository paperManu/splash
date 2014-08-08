#include "mesh_shmdata.h"

#include "log.h"
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
    if (_reader != nullptr)
        shmdata_any_reader_close(_reader);

    _reader = shmdata_any_reader_init();
    shmdata_any_reader_run_gmainloop(_reader, SHMDATA_TRUE);
    shmdata_any_reader_set_on_data_handler(_reader, Mesh_Shmdata::onData, this);
    shmdata_any_reader_start(_reader, filename.c_str());
    _filename = filename;

    return true;
}

/*************/
void Mesh_Shmdata::onData(shmdata_any_reader_t* reader, void* shmbuf, void* data, int data_size, unsigned long long timestamp,
    const char* type_description, void* user_data)
{
    Mesh_Shmdata* ctx = reinterpret_cast<Mesh_Shmdata*>(user_data);

    lock_guard<mutex> lock(ctx->_writeMutex);

    ctx->_bufferMesh.clear();

    // Read the number of vertices and polys
    int* intPtr = (int*)data;
    float* floatPtr = (float*)data;

    int verticeNbr = *(intPtr++);
    int polyNbr = *(intPtr++);

    MeshContainer::VertexHandle vertices[verticeNbr];

    floatPtr += 2;
    // First, create the vertices with no UV, normals or faces
    for (int v = 0; v < verticeNbr; ++v)
    {
        vertices[v] = ctx->_bufferMesh.add_vertex(MeshContainer::Point(floatPtr[0], floatPtr[1], floatPtr[2]));
        floatPtr += 8;
    }

    intPtr += 8 * verticeNbr;
    // Then create the faces
    vector<MeshContainer::VertexHandle> faceHandle;
    for (int p = 0; p < polyNbr; ++p)
    {
        int size = *(intPtr++);
        // Quads and larger polys are converted to tris through a very simple method
        // This can lead to bad shapes especially for polys larger than quads
        for (int tri = 0; tri < size - 2; ++tri)
        {
            faceHandle.clear();
            faceHandle.push_back(vertices[*intPtr]);
            faceHandle.push_back(vertices[*(intPtr + 1 + tri)]);
            faceHandle.push_back(vertices[*(intPtr + 2 + tri)]);
            ctx->_bufferMesh.add_face(faceHandle);
        }
        intPtr += size;
    }

    // Add the UV coords and the normals
    floatPtr = (float*)data;
    floatPtr += 5; // Go to the first UV data
    ctx->_bufferMesh.request_vertex_texcoords2D();
    ctx->_bufferMesh.request_vertex_normals();
    for (int v = 0; v < verticeNbr; ++v)
    {
        ctx->_bufferMesh.set_texcoord2D(vertices[v], MeshContainer::TexCoord2D(floatPtr[0], floatPtr[1]));
        ctx->_bufferMesh.set_normal(vertices[v], MeshContainer::Normal(floatPtr[2], floatPtr[3], floatPtr[4]));
        floatPtr += 8;
    }

    ctx->_meshUpdated = true;
    ctx->updateTimestamp();

    shmdata_any_reader_free(shmbuf);
}

/*************/
void Mesh_Shmdata::registerAttributes()
{
    _attribFunctions["file"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        return read(args[0].asString());
    });
}

} // end of namespace
