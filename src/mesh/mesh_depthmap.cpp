#include "./mesh/mesh_depthmap.h"

#include <glm/gtc/type_ptr.hpp>

#include "./image/image.h"
#include "./utils/log.h"

namespace Splash
{
/*************/
Mesh_Depthmap::Mesh_Depthmap(RootObject* root)
    : Mesh(root)
{
    _type = "mesh_depthmap";

    registerAttributes();
}

/*************/
bool Mesh_Depthmap::linkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (!obj)
        return false;

    if (auto image = std::dynamic_pointer_cast<Image>(obj))
    {
        if (image == _depthMap)
            return false;

        if (_depthMap == nullptr)
        {
            _depthMap = image;
            return true;
        }
    }

    return false;
}

/*************/
void Mesh_Depthmap::unlinkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (auto image = std::dynamic_pointer_cast<Image>(obj))
    {
        if (image == _depthMap)
            _depthMap.reset();
    }
}

/*************/
void Mesh_Depthmap::update()
{
    if (_depthMap)
    {
        const auto currentDepthTimestamp = _depthMap->getTimestamp();

        if (_depthMap && _depthUpdateTimestamp != currentDepthTimestamp)
        {
            const auto depthBuffer = _depthMap->get();
            const auto spec = _depthMap->getSpec();

            if (spec.channels != 1 || spec.format != "R")
                return;

            // Generate the image buffer to store depth in meters
            auto metersBuffer = std::vector<float>(static_cast<size_t>(spec.width * spec.height));

            // First convert depth map to meters
            // For now we only support 16bits depth information
            // "Should be enough for anyone".
            switch (auto bpp = depthBuffer.getSpec().bpp)
            {
            default:
                Log::get() << Log::DEBUGGING << "Mesh_Depthmap::" << __FUNCTION__ << " - Bit depth " << bpp << " not supported" << Log::endl;
                assert(false);
                return;
            case 16:
                // When represented as uint16, depth is stored in mm
                auto depthPtr = reinterpret_cast<const uint16_t*>(_depthMap->data());
                for (uint32_t y = 0; y < spec.height; ++y)
                {
                    const auto lineShift = y * spec.width;
                    for (uint32_t x = 0; x < spec.width; ++x)
                    {
                        const auto pixel = x + lineShift;
                        metersBuffer[pixel] = static_cast<float>(depthPtr[pixel]) / 1000.f;
                    }
                }
                break;
            }

            // Then convert the resulted converted depth map to a point cloud
            auto pointcloud = std::vector<glm::vec3>(static_cast<size_t>(spec.width * spec.height));
            for (uint32_t y = 0; y < spec.height; ++y)
            {
                const auto lineShift = y * spec.width;
                for (uint32_t x = 0; x < spec.width; ++x)
                {
                    const auto pixel = x + lineShift;
                    const auto depthValue = metersBuffer[pixel];
                    auto& point = pointcloud[pixel];

                    point.z = depthValue;
                    // Do not bother computing other coords if depth is 0.f
                    if (depthValue == 0.f)
                    {
                        point.x = 0.f;
                        point.y = 0.f;
                    }
                    else
                    {
                        point.x = (static_cast<float>(x) - _intrinsics.cx) / _intrinsics.fx * depthValue;
                        point.y = (static_cast<float>(y) - _intrinsics.cy) / _intrinsics.fy * depthValue;
                    }
                }
            }

            // Then convert the point cloud to a mesh
            // This is done very roughly, based on the initial pixels order
            MeshContainer mesh;
            for (uint32_t y = 0; y < spec.height - 1; ++y)
            {
                const auto lineShift = y * spec.width;
                const auto nextLineShift = lineShift + spec.width;
                for (uint32_t x = 0; x < spec.width - 1; ++x)
                {
                    // Construct the upper left triangle of a 2x2 pixels block
                    const auto& p1 = pointcloud[x + lineShift];
                    const auto& p2 = pointcloud[x + 1 + lineShift];
                    const auto& p3 = pointcloud[x + nextLineShift];

                    const auto resolution = glm::vec2(spec.width, spec.height);
                    const auto u1 = glm::vec2(x, y) / resolution;
                    const auto u2 = glm::vec2(x + 1, y) / resolution;
                    const auto u3 = glm::vec2(x, y + 1) / resolution;
                    const auto u4 = glm::vec2(x + 1, y + 1) / resolution;

                    if (p1.z != 0.f && p2.z != 0.f && p3.z != 0.f)
                    {
                        mesh.vertices.emplace_back(p1.x, p1.y, p1.z, 1.f);
                        mesh.vertices.emplace_back(p2.x, p2.y, p2.z, 1.f);
                        mesh.vertices.emplace_back(p3.x, p3.y, p3.z, 1.f);

                        const auto n1 = glm::normalize(glm::cross(p2 - p1, p3 - p1));
                        mesh.normals.emplace_back(n1.x, n1.y, n1.z, 0.f);
                        mesh.normals.emplace_back(n1.x, n1.y, n1.z, 0.f);
                        mesh.normals.emplace_back(n1.x, n1.y, n1.z, 0.f);

                        mesh.uvs.emplace_back(u1.x, u1.y);
                        mesh.uvs.emplace_back(u2.x, u2.y);
                        mesh.uvs.emplace_back(u3.x, u3.y);
                    }

                    // Construct the lower right triangle of a 2x2 pixels block
                    const auto& p4 = pointcloud[x + 1 + lineShift];
                    const auto& p5 = pointcloud[x + 1 + nextLineShift];
                    const auto& p6 = pointcloud[x + nextLineShift];

                    if (p4.z != 0.f && p5.z != 0.f && p6.z != 0.f)
                    {
                        mesh.vertices.emplace_back(p4.x, p4.y, p4.z, 1.f);
                        mesh.vertices.emplace_back(p5.x, p5.y, p5.z, 1.f);
                        mesh.vertices.emplace_back(p6.x, p6.y, p6.z, 1.f);

                        const auto n2 = glm::normalize(glm::cross(p5 - p4, p6 - p4));
                        mesh.normals.emplace_back(n2.x, n2.y, n2.z, 0.f);
                        mesh.normals.emplace_back(n2.x, n2.y, n2.z, 0.f);
                        mesh.normals.emplace_back(n2.x, n2.y, n2.z, 0.f);

                        mesh.uvs.emplace_back(u2.x, u2.y);
                        mesh.uvs.emplace_back(u4.x, u4.y);
                        mesh.uvs.emplace_back(u3.x, u3.y);
                    }
                }
            }

            _depthUpdateTimestamp = currentDepthTimestamp;

            _bufferMesh = mesh;
            _meshUpdated = true;
            updateTimestamp();
        }
    }

    Mesh::update();
}

/*************/
void Mesh_Depthmap::registerAttributes()
{
    Mesh::registerAttributes();

    addAttribute(
        "intrinsics",
        [&](const Values& args) {
            _intrinsics.cx = args[0].as<float>();
            _intrinsics.cy = args[1].as<float>();
            _intrinsics.fx = args[2].as<float>();
            _intrinsics.fy = args[3].as<float>();
            return true;
        },
        [&]() -> Values {
            return {_intrinsics.cx, _intrinsics.cy, _intrinsics.fx, _intrinsics.fy};
        },
        {'r', 'r', 'r', 'r'});
    setAttributeDescription("intrinsics", "Set the camera's intrinsic parameters: cx, cy, fx, fy");

    addAttribute(
        "maxDepthDistance",
        [&](const Values& args) {
            _maxDepthDistance = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_maxDepthDistance}; },
        {'r'});
    setAttributeDescription("maxDepthDistance", "Set the maximum depth representable by depth map values");
}

} // namespace Splash
