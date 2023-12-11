#include "./mesh/mesh_bezierpatch.h"

#include "./utils/log.h"

namespace Splash
{

/*************/
Mesh_BezierPatch::Mesh_BezierPatch(RootObject* root)
    : Mesh(root)
{
    _type = "mesh_bezierPatch";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    auto patch = createPatch();
    updateBezierFromPatch(patch);
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

    Mesh::update();
}

/*************/
Mesh_BezierPatch::Patch Mesh_BezierPatch::createPatch(int width, int height)
{
    width = std::max(2, width);
    height = std::max(2, height);

    Patch patch;
    patch.size = glm::ivec2(width, height);

    for (int v = 0; v < height; ++v)
    {
        glm::vec2 position;
        glm::vec2 uv;

        uv.y = static_cast<float>(v) / static_cast<float>(height - 1);
        position.y = uv.y * 2.f - 1.f;

        for (int u = 0; u < width; ++u)
        {
            uv.x = static_cast<float>(u) / static_cast<float>(width - 1);
            position.x = uv.x * 2.f - 1.f;
            patch.vertices.push_back(position);
        }
    }

    return patch;
}

/*************/
void Mesh_BezierPatch::updateBezierFromPatch(Patch& patch)
{
    if (patch.size.x * patch.size.y != static_cast<int>(patch.vertices.size()))
        return;

    const uint32_t width = patch.size.x;
    const uint32_t height = patch.size.y;

    // Set uv coordinates to the patch
    patch.uvs.resize(patch.vertices.size());
    for (uint32_t v = 0; v < height; ++v)
        for (uint32_t u = 0; u < width; ++u)
            patch.uvs[u + v * width] = glm::vec2(static_cast<float>(u) / static_cast<float>(width - 1), static_cast<float>(v) / static_cast<float>(height - 1));

    _patch = patch;
    _patchUpdated = true;

    MeshContainer mesh;
    for (uint32_t v = 0; v < height - 1; ++v)
    {
        for (uint32_t u = 0; u < width - 1; ++u)
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

            for (size_t i = 0; i < 6; ++i)
                mesh.normals.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
        }
    }

    _bezierControl = mesh;
    updateTimestamp();
}

/*************/
void Mesh_BezierPatch::updatePatch()
{
    std::vector<glm::vec2> vertices;
    std::vector<glm::vec2> uvs;

    // Update the binomial coefficients if needed
    if (_patch.size != _binomialDimensions)
    {
        _binomialCoeffsX.clear();
        _binomialCoeffsY.clear();

        for (int i = 0; i < _patch.size.x; ++i)
            _binomialCoeffsX.push_back(static_cast<float>(binomialCoeff(_patch.size.x - 1, i)));

        for (int i = 0; i < _patch.size.y; ++i)
            _binomialCoeffsY.push_back(static_cast<float>(binomialCoeff(_patch.size.y - 1, i)));

        _binomialDimensions = _patch.size;
    }

    // Compute the vertices positions
    for (int v = 0; v < _patchResolution; ++v)
    {
        glm::vec2 uv;

        uv.y = static_cast<float>(v) / static_cast<float>(_patchResolution - 1);

        for (int u = 0; u < _patchResolution; ++u)
        {
            uv.x = static_cast<float>(u) / static_cast<float>(_patchResolution - 1);

            glm::vec2 vertex{0.f, 0.f};
            for (int32_t j = 0; j < _patch.size.y; ++j)
            {
                for (int32_t i = 0; i < _patch.size.x; ++i)
                {
                    const auto iAsFloat = static_cast<float>(i);
                    const auto jAsFloat = static_cast<float>(j);
                    float factor = _binomialCoeffsY[j] * powf(uv.y, jAsFloat) * powf(1.f - uv.y, static_cast<float>(_patch.size.y) - 1.f - jAsFloat) * _binomialCoeffsX[i] *
                                   powf(uv.x, iAsFloat) * powf(1.f - uv.x, static_cast<float>(_patch.size.x) - 1.f - iAsFloat);
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

            for (size_t i = 0; i < 6; ++i)
                mesh.normals.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
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

    addAttribute(
        "patchControl",
        [&](const Values& args) {
            const auto width = args[0].as<uint32_t>();
            const auto height = args[1].as<uint32_t>();

            if (args.size() - 2 != height * width)
                return false;

            Patch patch;
            patch.size = glm::ivec2(width, height);
            for (size_t p = 2; p < args.size(); ++p)
                patch.vertices.emplace_back(glm::vec2(args[p].as<Values>()[0].as<float>(), args[p].as<Values>()[1].as<float>()));

            // Check whether the patch is different from the current one
            // This implies recomputing the default patch with the same size
            auto defaultPatch = createPatch(height, width);
            bool isDefaultPatch = true;
            for (uint32_t y = 0; y < height; ++y)
                for (uint32_t x = 0; x < width; ++x)
                {
                    const auto index = y * width + x;
                    if (patch.vertices[index] != defaultPatch.vertices[index])
                        isDefaultPatch = false;
                }

            if (isDefaultPatch)
            {
                _patchModified = false;
                return true;
            }

            updateBezierFromPatch(patch);
            _patchModified = true;

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
        {'i', 'i'});
    setAttributeDescription("patchControl", "Set the control points positions");

    addAttribute(
        "patchSize",
        [&](const Values& args) {
            const auto width = args[0].as<int>();
            const auto height = args[1].as<int>();

            if (width == _patch.size.x && height == _patch.size.y)
                return true;

            auto patch = createPatch(std::max(width, 2), std::max(height, 2));
            updateBezierFromPatch(patch);
            _patchModified = false;
            return true;
        },
        [&]() -> Values {
            return {_patch.size.x, _patch.size.y};
        },
        {'i', 'i'});
    setAttributeDescription("patchSize", "Set the Bezier patch control resolution");

    addAttribute(
        "patchResolution",
        [&](const Values& args) {
            _patchResolution = std::max(4, args[0].as<int>());
            _patchUpdated = true;
            updateTimestamp();
            return true;
        },
        [&]() -> Values { return {_patchResolution}; },
        {'i'});
    setAttributeDescription("patchResolution", "Set the Bezier patch final resolution");

    addAttribute("resetPatch",
        [&](const Values&) {
            auto patch = createPatch(_patch.size.x, _patch.size.y);
            _patch = Patch();
            updateBezierFromPatch(patch);
            _patchModified = false;
            return true;
        },
        {});
    setAttributeDescription("resetPatch", "Reset the control patch to the default one, considering its current size");
}

} // namespace Splash
