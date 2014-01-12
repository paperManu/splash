#include "mesh.h"

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
    vector<float> coords;
    coords.resize(_mesh.n_faces() * 3 * 4);

    int idx = 0;
    for_each (_mesh.faces_begin(), _mesh.faces_end(), [&] (const MeshContainer::FaceHandle& face)
    {
        for_each (_mesh.cfv_begin(face), _mesh.cfv_end(face), [&] (const MeshContainer::VertexHandle& vertex)
        {
            auto point = _mesh.point(vertex);
            for (int i = 0; i < 3; ++i)
                coords[idx + i] = point[i];
            coords[idx + 3] = 1.f; // We add this to get normalized coordinates
            idx += 4;
        });
    });

    return coords;
}

/*************/
vector<float> Mesh::getUVCoords() const
{
    vector<float> coords;
    coords.resize(_mesh.n_faces() * 3 * 2);

    int idx = 0;
    for_each (_mesh.faces_begin(), _mesh.faces_end(), [&] (const MeshContainer::FaceHandle& face)
    {
        for_each (_mesh.cfv_begin(face), _mesh.cfv_end(face), [&] (const MeshContainer::VertexHandle& vertex)
        {
            auto tex = _mesh.texcoord2D(vertex);
            for (int i = 0; i < 2; ++i)
                coords[idx + i] = tex[i];
            idx += 2;
        });
    });

    return coords;
}

/*************/
vector<float> Mesh::getNormals() const
{
    vector<float> normals;
    normals.resize(_mesh.n_faces() * 3 * 4);

    int idx = 0;
    for_each (_mesh.faces_begin(), _mesh.faces_end(), [&] (const MeshContainer::FaceHandle& face)
    {
        for_each (_mesh.cfv_begin(face), _mesh.cfv_end(face), [&] (const MeshContainer::VertexHandle& vertex)
        {
            auto normal = _mesh.normal(vertex);
            for (int i = 0; i < 3; ++i)
                normals[idx + i] = normal[i];
            normals[idx + 3] = 1.f; // We add this to get normalized coordinates
            idx += 4;
        });
    });

    return normals;
}

/*************/
bool Mesh::read(const string& filename)
{
    OpenMesh::IO::Options readOptions;
    readOptions += OpenMesh::IO::Options::VertexTexCoord;

    MeshContainer mesh;
    mesh.request_vertex_texcoords2D();
    if (!OpenMesh::IO::read_mesh(mesh, filename, readOptions))
    {
        SLog::log << Log::WARNING << "Mesh::" << __FUNCTION__ << " - Unable to read the specified mesh file" << Log::endl;
        return false;
    }
    
    // Update the normals
    mesh.request_vertex_normals();
    mesh.request_face_normals();
    mesh.update_normals();
    mesh.release_face_normals();

    _mesh = mesh;
    updateTimestamp();
    return true;
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

    int nbrVertices = data[0].size() / 4;
    int totalSize = sizeof(nbrVertices); // We add to all this the total number of vertices
    for (auto d : data)
        totalSize += d.size() * sizeof(d[0]);
    obj.resize(totalSize);

    //cout << data[0].size() << " " << data[1].size() << " " << data[2].size() << endl;

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
bool Mesh::deserialize(const SerializedObject& obj)
{
    if (obj.size() == 0)
        return false;

    // First, we get the number of vertices
    int nbrVertices;
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
        for (auto& d : data)
        {
            ptr = reinterpret_cast<unsigned char*>(d.data());
            copy(currentObjPtr, currentObjPtr + d.size() * sizeof(float), ptr);
            currentObjPtr += d.size() * sizeof(float);
        }

        // Next step: use these values to reset the vertices of _mesh
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

        // We are also loading uv coords and normals
        _mesh.request_vertex_normals();
        _mesh.request_vertex_texcoords2D();
        int faceIdx = 0;
        for_each (_mesh.faces_begin(), _mesh.faces_end(), [&] (const MeshContainer::FaceHandle& face)
        {
            int vertexIdx = 0;
            for_each (_mesh.cfv_begin(face), _mesh.cfv_end(face), [&] (const MeshContainer::VertexHandle& vertex)
            {
                int floatIdx = faceIdx * 3 + vertexIdx;
                _mesh.set_texcoord2D(vertex, MeshContainer::TexCoord2D(data[1][floatIdx * 2 + 0], data[1][floatIdx * 2 + 1]));
                _mesh.set_normal(vertex, MeshContainer::Normal(data[2][floatIdx * 4 + 0], data[2][floatIdx * 4 + 1], data[2][floatIdx * 4 + 2]));
                vertexIdx++;
            });
            faceIdx++;
        });

        updateTimestamp();
    }
    catch (...)
    {
        createDefaultMesh();
        SLog::log(Log::ERROR, __FUNCTION__, " - Unable to deserialize the given object");
        return false;
    }

    return true;
}

/*************/
void Mesh::createDefaultMesh()
{
    _mesh.clear();

    // Create the vertices
    MeshContainer::VertexHandle vertices[4];
    vertices[0] = _mesh.add_vertex(MeshContainer::Point(-1, -1, 0));
    vertices[1] = _mesh.add_vertex(MeshContainer::Point(1, -1, 0));
    vertices[2] = _mesh.add_vertex(MeshContainer::Point(1, 1, 0));
    vertices[3] = _mesh.add_vertex(MeshContainer::Point(-1, 1, 0));

    // Then the faces
    vector<MeshContainer::VertexHandle> faceHandle;
    faceHandle.clear();
    faceHandle.push_back(vertices[0]);
    faceHandle.push_back(vertices[1]);
    faceHandle.push_back(vertices[2]);
    _mesh.add_face(faceHandle);

    faceHandle.clear();
    faceHandle.push_back(vertices[2]);
    faceHandle.push_back(vertices[3]);
    faceHandle.push_back(vertices[0]);
    _mesh.add_face(faceHandle);

    // Update the normals
    _mesh.request_vertex_normals();
    _mesh.request_face_normals();
    _mesh.update_normals();
    _mesh.release_face_normals();

    // Add the UV coords
    _mesh.request_vertex_texcoords2D();
    _mesh.set_texcoord2D(vertices[0], MeshContainer::TexCoord2D(0, 0));
    _mesh.set_texcoord2D(vertices[1], MeshContainer::TexCoord2D(1, 0));
    _mesh.set_texcoord2D(vertices[2], MeshContainer::TexCoord2D(1, 1));
    _mesh.set_texcoord2D(vertices[3], MeshContainer::TexCoord2D(0, 1));

    updateTimestamp();
}

/*************/
void Mesh::updateTimestamp()
{
    _timestamp = chrono::high_resolution_clock::now();
}

/*************/
void Mesh::registerAttributes()
{
}

} // end of namespace
