#include "mesh.h"

using namespace std;

namespace Splash {

/*************/
Mesh::Mesh()
{
}

/*************/
Mesh::~Mesh()
{
}

/*************/
vector<float> Mesh::getVertCoords() const
{
    vector<float> coords;
    coords.resize(_mesh.n_faces() * 3 * 4);

    for_each (_mesh.faces_begin(), _mesh.faces_end(), [&] (const MeshContainer::FaceHandle face)
    {
        for_each (_mesh.cfv_iter(face), _mesh.cfv_end(face), [&] (const MeshContainer::VertexHandle vertex)
        {
            auto point = _mesh.point(vertex);
            for (int i = 0; i < 3; ++i)
                coords.push_back(point[i]);
            coords.push_back(1.f); // We add this to get normalized coordinates
        });
    });

    return coords;
}

/*************/
vector<float> Mesh::getUVCoords() const
{
    vector<float> coords;
    coords.resize(_mesh.n_faces() * 3 * 2);

    for_each (_mesh.faces_begin(), _mesh.faces_end(), [&] (const MeshContainer::FaceHandle face)
    {
        for_each (_mesh.cfv_iter(face), _mesh.cfv_end(face), [&] (const MeshContainer::VertexHandle vertex)
        {
            auto point = _mesh.texcoord2D(vertex);
            for (int i = 0; i < 2; ++i)
                coords.push_back(point[i]);
        });
    });

    return coords;
}

/*************/
vector<float> Mesh::getNormals() const
{
    vector<float> normals;
    normals.resize(_mesh.n_faces() * 3 * 4);

    for_each (_mesh.faces_begin(), _mesh.faces_end(), [&] (const MeshContainer::FaceHandle face)
    {
        for_each (_mesh.cfv_iter(face), _mesh.cfv_end(face), [&] (const MeshContainer::VertexHandle vertex)
        {
            auto point = _mesh.normal(vertex);
            for (int i = 0; i < 3; ++i)
                normals.push_back(point[i]);
            normals.push_back(1.f); // We add this to get normalized coordinates
        });
    });

    return normals;
}

/*************/
Mesh::SerializedObject Mesh::serialize() const
{
    SerializedObject obj;

    // For this, we will use the getVertex, getUV, etc. methods to create a serialized representation of the mesh
    vector<vector<float>> data;
    data.push_back(move(getVertCoords()));
    data.push_back(move(getUVCoords()));
    data.push_back(move(getNormals()));

    vector<float> vertices = move(getVertCoords());
    vector<float> uv = move(getUVCoords());
    vector<float> normals = move(getNormals());

    int nbrVertices = data[0].size();
    float totalSize = sizeof(nbrVertices); // We add to all this the total number of vertices
    for (auto d : data)
        totalSize += d.size() * sizeof(d[0]);
    obj.resize(totalSize);
    
    auto currentObjPtr = obj.data();
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(&nbrVertices);
    copy(ptr, ptr + sizeof(nbrVertices), currentObjPtr);
    currentObjPtr += sizeof(nbrVertices);

    for (auto d : data)
    {
        ptr = reinterpret_cast<const unsigned char*>(d.data());
        copy(ptr, ptr + d.size() * sizeof(float), currentObjPtr);
        currentObjPtr += d.size() * sizeof(float);
    }

    return obj;
}

/*************/
bool Mesh::deserialize(SerializedObject& obj)
{
    if (obj.size() == 0)
        return false;

    // First, we get the number of vertices
    float nbrVertices;
    unsigned char* ptr = reinterpret_cast<unsigned char*>(&nbrVertices);

    auto currentObjPtr = obj.data();
    copy(currentObjPtr, currentObjPtr + sizeof(nbrVertices), ptr); // This will fail if float have different size between sender and receiver
    currentObjPtr += sizeof(nbrVertices);

    vector<vector<float>> data;
    data.push_back(vector<float>(nbrVertices * 4));
    data.push_back(vector<float>(nbrVertices * 2));
    data.push_back(vector<float>(nbrVertices * 4));

    // Let's read the values
    try
    {
        for (auto d : data)
        {
            ptr = reinterpret_cast<unsigned char*>(d.data());
            copy(currentObjPtr, currentObjPtr + d.size() * sizeof(float), ptr);
            currentObjPtr += d.size() * sizeof(float);
        }

        // Next step: use these values to reset the _mesh
        _mesh.clear();
        for (int face = 0; face < nbrVertices / 3; ++face)
        {
            MeshContainer::VertexHandle vertices[3];
            vertices[0] = _mesh.add_vertex(MeshContainer::Point(data[0][face * 3 * 4 + 0], data[0][face * 3 * 4 + 1], data[0][face * 3 * 4 + 2]));
            vertices[1] = _mesh.add_vertex(MeshContainer::Point(data[0][face * 3 * 4 + 4], data[0][face * 3 * 4 + 5], data[0][face * 3 * 4 + 6]));
            vertices[2] = _mesh.add_vertex(MeshContainer::Point(data[0][face * 3 * 4 + 8], data[0][face * 3 * 4 + 9], data[0][face * 3 * 4 + 10]));

            vector<MeshContainer::VertexHandle> faceHandle;
            faceHandle.clear();
            faceHandle.push_back(vertices[0]);
            faceHandle.push_back(vertices[1]);
            faceHandle.push_back(vertices[2]);
            _mesh.add_face(faceHandle);
        }
    }
    catch (...)
    {
        gLog(Log::ERROR, __FUNCTION__, " - Unable to deserialize the given object");
        return false;
    }

    return true;
}

} // end of namespace
