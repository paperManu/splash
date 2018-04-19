#include "./mesh/mesh_bezierpatch.h"

#include "./utils/log.h"

using namespace std;

namespace Splash
{

/*************/
Mesh_BezierPatch::Mesh_BezierPatch(RootObject* root)
    : Mesh(root)
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
    lock_guard<mutex> lockPatch(_patchMutex);

    if (control)
        _bufferMesh = _bezierControl;
    else
        _bufferMesh = _bezierMesh;

    updateTimestamp();
    _meshUpdated = true;
}

/*************/
void Mesh_BezierPatch::update()
{
    if (_patchUpdated)
    {
        updatePatch();
        _patchUpdated = false;
    }

    lock_guard<mutex> lockPatch(_patchMutex);
    Mesh::update();
}

/*************/
void Mesh_BezierPatch::init()
{
    _type = "mesh_bezierPatch";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    createPatch();
}

/*************/
void Mesh_BezierPatch::createPatch(int width, int height)
{
    unique_lock<mutex> lock(_patchMutex);

    width = std::max(2, width);
    height = std::max(2, height);

    // Check whether the current patch has the same size
    if (_bezierControl.vertices.size() != 0 && _patch.size.x == width && _patch.size.y == height)
        return;

    Patch patch;
    patch.size = glm::ivec2(width, height);

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
        }
    }

    lock.unlock();
    createPatch(patch);
}

/*************/
void Mesh_BezierPatch::createPatch(Patch& patch)
{
    if (patch.size.x * patch.size.y != static_cast<int>(patch.vertices.size()))
        return;

    lock_guard<mutex> lock(_patchMutex);

    int width = patch.size.x;
    int height = patch.size.y;

    // Set uv coordinates to the patch
    patch.uvs.resize(patch.vertices.size());
    for (int v = 0; v < height; ++v)
        for (int u = 0; u < width; ++u)
            patch.uvs[u + v * width] = glm::vec2((float)u / ((float)width - 1.f), (float)v / ((float)height - 1.f));

    _patch = patch;
    _patchUpdated = true;

    MeshContainer mesh;
    for (int v = 0; v < height - 1; ++v)
    {
        for (int u = 0; u < width - 1; ++u)
        {
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + v * width], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + 1 + v * width], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + (v + 1) * width], 0.0, 1.0));

            mesh.vertices.push_back(glm::vec4(patch.vertices[u + 1 + (v + 1) * width], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + (v + 1) * width], 0.0, 1.0));
            mesh.vertices.push_back(glm::vec4(patch.vertices[u + 1 + v * width], 0.0, 1.0));

            mesh.uvs.push_back(patch.uvs[u + v * width]);
            mesh.uvs.push_back(patch.uvs[u + 1 + v * width]);
            mesh.uvs.push_back(patch.uvs[u + (v + 1) * width]);

            mesh.uvs.push_back(patch.uvs[u + 1 + (v + 1) * width]);
            mesh.uvs.push_back(patch.uvs[u + (v + 1) * width]);
            mesh.uvs.push_back(patch.uvs[u + 1 + v * width]);

            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
            mesh.normals.push_back(glm::vec3(0.0, 0.0, 1.0));
        }
    }
    _bezierControl = mesh;

    updateTimestamp();
}

/*************/
void Mesh_BezierPatch::updatePatch()
{
    lock_guard<mutex> lock(_patchMutex);

    vector<glm::vec2> vertices;
    vector<glm::vec2> uvs;

    // Update the binomial coefficients if needed
    if (_patch.size != _binomialDimensions)
    {
        _binomialCoeffsX.clear();
        _binomialCoeffsY.clear();

        for (int i = 0; i < _patch.size.x; ++i)
            _binomialCoeffsX.push_back((float)binomialCoeff(_patch.size.x - 1, i));

        for (int i = 0; i < _patch.size.y; ++i)
            _binomialCoeffsY.push_back((float)binomialCoeff(_patch.size.y - 1, i));

        _binomialDimensions = _patch.size;
    }

    // Compute the vertices positions
    for (int v = 0; v < _patchResolution; ++v)
    {
        glm::vec2 uv;

        uv.y = (float)v / ((float)_patchResolution - 1.f);

        for (int u = 0; u < _patchResolution; ++u)
        {
            uv.x = (float)u / ((float)_patchResolution - 1.f);

            glm::vec2 vertex{0.f, 0.f};
            for (int j = 0; j < _patch.size.y; ++j)
            {
                for (int i = 0; i < _patch.size.x; ++i)
                {
                    float factor = _binomialCoeffsY[j] * pow(uv.y, (float)j) * pow(1.f - uv.y, (float)_patch.size.y - 1.f - (float)j) * _binomialCoeffsX[i] * pow(uv.x, (float)i) *
                                   pow(1.f - uv.x, (float)_patch.size.x - 1.f - (float)i);
                    vertex += factor * _patch.vertices[i + j * _patch.size.x];
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
    Mesh::registerAttributes();

    addAttribute("patchControl",
        [&](const Values& args) {
            auto width = args[0].as<uint32_t>();
            auto height = args[1].as<uint32_t>();

            if (args.size() - 2 != height * width)
                return false;

            Patch patch;
            patch.size = glm::ivec2(width, height);
            for (uint32_t p = 2; p < args.size(); ++p)
                patch.vertices.push_back(glm::vec2(args[p].as<Values>()[0].as<float>(), args[p].as<Values>()[1].as<float>()));

            createPatch(patch);

            return true;
        },
        [&]() -> Values {
            Values v;
            v.push_back(_patch.size.x);
            v.push_back(_patch.size.y);

            for (uint32_t i = 0; i < _patch.vertices.size(); ++i)
            {
                Values vertex{_patch.vertices[i].x, _patch.vertices[i].y};
                v.emplace_back(vertex);
            }

            return v;
        },
        {'n', 'n'});
    setAttributeDescription("patchControl", "Set the control points positions");

    addAttribute("patchSize",
        [&](const Values& args) {
            createPatch(std::max(args[0].as<int>(), 2), std::max(args[1].as<int>(), 2));
            return true;
        },
        [&]() -> Values {
            return {_patch.size.x, _patch.size.y};
        },
        {'n', 'n'});
    setAttributeDescription("patchSize", "Set the Bezier patch control resolution");

    addAttribute("patchResolution",
        [&](const Values& args) {
            _patchResolution = std::max(4, args[0].as<int>());
            _patchUpdated = true;
            updateTimestamp();
            return true;
        },
        [&]() -> Values { return {_patchResolution}; },
        {'n'});
    setAttributeDescription("patchResolution", "Set the Bezier patch final resolution");
}

} // end of namespace
