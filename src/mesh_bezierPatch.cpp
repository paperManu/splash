#include "mesh_bezierPatch.h"

using namespace std;

namespace Splash
{

/*************/
Mesh_BezierPatch::Mesh_BezierPatch()
{
    init();
}

/*************/
Mesh_BezierPatch::Mesh_BezierPatch(bool linkedToWorld)
    : Mesh(linkedToWorld)
{
    init();
}

/*************/
Mesh_BezierPatch::~Mesh_BezierPatch()
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Mesh_BezierPatch::~Mesh_BezierPatch - Destructor" << Log::endl;
#endif
}

/*************/
void Mesh_BezierPatch::switchMeshes(bool control)
{
    if (control)
        _bufferMesh = _bezierControl;
    else
        _bufferMesh = _bezierMesh;

    updateTimestamp();
}

/*************/
void Mesh_BezierPatch::update()
{
    if (_patchUpdated)
    {
        updatePatch();
        _patchUpdated = false;
    }

    Mesh::update();
}

/*************/
void Mesh_BezierPatch::init()
{
    _type = "mesh_bezierPatch";

    registerAttributes();

    createPatch();
}

/*************/
void Mesh_BezierPatch::createPatch(int width, int height)
{
    width = std::max(2, width);
    height = std::max(2, height);

    Patch patch;

    for (int v = 0; v < height; ++v)
    {
        glm::vec2 position;
        glm::vec2 uv;

        uv.y = (float)v / ((float)height - 1.f);
        position.y = uv.y * 2.f - 1.f;

        for (int u = 0; u < width; ++u)
        {
            uv.x = (float)u / ((float)width - 1.f);
            position.x = uv.x * 2.f - 1.f;

            patch.vertices.push_back(position);
            patch.uvs.push_back(uv);
        }
    }
    _patch = patch;
    _size = glm::ivec2(width, height);
    _patchUpdated = true;

    MeshContainer mesh;
    for (int v = 0; v < height - 1; ++v)
    {
        for (int u = 0; u < width - 1; ++u)
        {
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + v * (width + 1)], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + 1 + v * (width + 1)], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + (v + 1) * (width + 1)], 0.0, 1.0));

            mesh.vertices.push_back(glm::vec4(patch.vertices[u + 1 + v * (width + 1)], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + 1 + (v + 1) * (width + 1)], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + (v + 1) * (width + 1)], 0.0, 1.0));

            mesh.uvs.push_back(patch.uvs[u + v * (width + 1)]);
            mesh.uvs.push_back(patch.uvs[u + 1 + v * (width + 1)]);
            mesh.uvs.push_back(patch.uvs[u + (v + 1) * (width + 1)]);

            mesh.uvs.push_back(patch.uvs[u + 1 + v * (width + 1)]);
            mesh.uvs.push_back(patch.uvs[u + 1 + (v + 1) * (width + 1)]);
            mesh.uvs.push_back(patch.uvs[u + (v + 1) * (width + 1)]);

            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
        }
    }
    _bezierControl = mesh;
}

/*************/
void Mesh_BezierPatch::updatePatch()
{
    vector<glm::vec2> vertices;
    vector<glm::vec2> uvs;

    // Compute the vertices positions
    for (int v = 0; v < _patchResolution; ++v)
    {
        glm::vec2 uv;

        uv.y = (float)v / ((float)_patchResolution - 1.f);

        for (int u = 0; u < _patchResolution; ++u)
        {
            uv.x = (float)u / ((float)_patchResolution - 1.f);

            glm::vec2 vertex {0.f, 0.f};
            for (int j = 0; j < _size.y; ++j)
            {
                for (int i = 0; i < _size.x ; ++i)
                {
                    float factor = (float)binomialCoeff(_size.y - 1, j) * pow(uv.y, (float)j) * pow(1.f - uv.y, (float)_size.y - 1.f - (float)j)
                                 * (float)binomialCoeff(_size.x - 1, i) * pow(uv.x, (float)i) * pow(1.f - uv.x, (float)_size.x - 1.f - (float)i);
                    vertex += factor * _patch.vertices[i + j * _size.x];
                }
            }

            vertices.push_back(vertex);
            uvs.push_back(uv);
        }
    }

    // Create the mesh
    MeshContainer mesh;
    for (int v = 0; v < _patchResolution - 1; ++v)
    {
        for (int u = 0; u < _patchResolution - 1; ++u)
        {
            mesh.vertices.push_back(glm::vec4(vertices[u + v * _patchResolution], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(vertices[u + 1 + v * _patchResolution], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(vertices[u + (v + 1) * _patchResolution], 0.0, 1.0));

            mesh.vertices.push_back(glm::vec4(vertices[u + 1 + v * _patchResolution], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(vertices[u + 1 + (v + 1) * _patchResolution], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(vertices[u + (v + 1) * _patchResolution], 0.0, 1.0));

            mesh.uvs.push_back(uvs[u + v * _patchResolution]);
            mesh.uvs.push_back(uvs[u + 1 + v * _patchResolution]);
            mesh.uvs.push_back(uvs[u + (v + 1) * _patchResolution]);

            mesh.uvs.push_back(uvs[u + 1 + v * _patchResolution]);
            mesh.uvs.push_back(uvs[u + 1 + (v + 1) * _patchResolution]);
            mesh.uvs.push_back(uvs[u + (v + 1) * _patchResolution]);

            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
        }
    }

    _bufferMesh = mesh;
    _bezierMesh = mesh;

    updateTimestamp();
    _meshUpdated = true;
}

/*************/
void Mesh_BezierPatch::registerAttributes()
{
}

} // end of namespace
