#include "./controller/texcoordgenerator.h"

#include <calimiro/calimiro.h>

namespace Splash
{

/*************/
TexCoordGenerator::TexCoordGenerator(RootObject* root)
    : ControllerObject(root)
{
    _type = "TexCoordGenerator";
    _renderingPriority = GraphObject::Priority::POST_WINDOW;
    registerAttributes();
}

/*************/
calimiro::Geometry TexCoordGenerator::generateTexCoordFromGeometry(calimiro::Geometry& geometry)
{
    assert(geometry.vertices().size() != 0);

    Utils::CalimiroLogger logger;
    std::unique_ptr<calimiro::TexCoordGen> generator;
    switch (_method)
    {
    default:
    case calimiro::TexCoordUtils::texCoordMethod::ORTHOGRAPHIC:
        generator = std::make_unique<calimiro::TexCoordGenOrthographic>(
            &logger, geometry.vertices(), _eyePosition, glm::normalize(_eyeOrientation), _horizonRotation, _flipHorizontal, _flipVertical);
        break;
    case calimiro::TexCoordUtils::texCoordMethod::PLANAR:
        generator = std::make_unique<calimiro::TexCoordGenPlanar>(
            &logger, geometry.vertices(), _eyePosition, glm::normalize(_eyeOrientation), _horizonRotation, _flipHorizontal, _flipVertical);
        break;
    case calimiro::TexCoordUtils::texCoordMethod::SPHERIC:
        generator = std::make_unique<calimiro::TexCoordGenSpheric>(
            &logger, geometry.vertices(), _eyePosition, glm::normalize(_eyeOrientation), _horizonRotation, _flipHorizontal, _flipVertical, _fov);
        break;
    case calimiro::TexCoordUtils::texCoordMethod::EQUIRECTANGULAR:
        generator = std::make_unique<calimiro::TexCoordGenEquirectangular>(
            &logger, geometry.vertices(), _eyePosition, glm::normalize(_eyeOrientation), _horizonRotation, _flipHorizontal, _flipVertical, _fov);
        break;
    }
    return geometry.computeTextureCoordinates(generator.get());
}

/*************/
void TexCoordGenerator::generateTexCoordOnMesh()
{
    if (_meshName.empty())
    {
        Log::get() << Log::WARNING << "TexCoordGenerator::" << __FUNCTION__ << " - meshName is invalid (Might not have been initialized) - use the attribute \"meshName\""
                   << Log::endl;
        return;
    }

    const auto mesh = getObjectPtr(_meshName);
    if (mesh == nullptr)
    {
        Log::get() << Log::WARNING << "TexCoordGenerator::" << __FUNCTION__ << " - mesh is invalid (nullptr)" << Log::endl;
        return;
    }

    // Read the file
    Utils::CalimiroLogger _logger;
    const auto file = getObjectAttribute(mesh->getName(), "file")[0].as<std::string>();

    // Populate uv
    calimiro::Obj objFileRead(&_logger);
    bool tmpReadSuccess = objFileRead.readMesh(file); // Read instead of pulling directly from mesh to remove duplicate vertices created for faces
    if (!tmpReadSuccess)
    {
        Log::get() << Log::WARNING << "TexCoordGenerator::" << __FUNCTION__ << " - failed reading mesh from file \"" << file << "\"" << Log::endl;
        return;
    }
    auto geometryWithTexCoord = generateTexCoordFromGeometry(objFileRead);

    // Write new file
    calimiro::Obj objFile(&_logger, geometryWithTexCoord);
    bool tmpWriteSuccess = objFile.writeMesh(file);

    if (!tmpWriteSuccess)
    {
        Log::get() << Log::WARNING << "TexCoordGenerator::" << __FUNCTION__ << " - failed writing file \"" << file << "\"" << Log::endl;
        return;
    }

    // Load new file
    if (_replaceMesh)
        setObjectAttribute(mesh->getName(), "file", {file});
}

/*************/
void TexCoordGenerator::registerAttributes()
{
    addAttribute(
        "method",
        [&](const Values& args) {
            auto val = args[0].as<std::string>();
            for (auto const& method : calimiro::TexCoordUtils::getTexCoordMethodList())
                if (val.find(calimiro::TexCoordUtils::getTexCoordMethodTitle(method)) == 0)
                    _method = method;
            return true;
        },
        [&]() -> Values { return {calimiro::TexCoordUtils::getTexCoordMethodTitle(_method)}; },
        {'s'});
    setAttributeDescription("method", "Method for computing texture coordinates");

    addAttribute(
        "eyePosition",
        [&](const Values& args) {
            _eyePosition = glm::vec3(args[0].as<float>(), args[1].as<float>(), args[2].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_eyePosition.x, _eyePosition.y, _eyePosition.z};
        },
        {'r', 'r', 'r'});
    setAttributeDescription("eyePosition", "Eye position for computing uv coordinates");

    addAttribute(
        "eyeOrientation",
        [&](const Values& args) {
            _eyeOrientation = glm::vec3(args[0].as<float>(), args[1].as<float>(), args[2].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_eyeOrientation.x, _eyeOrientation.y, _eyeOrientation.z};
        },
        {'r', 'r', 'r'});
    setAttributeDescription("eyeOrientation", "Eye orientation for computing uv coordinates");

    addAttribute("normalizeEyeOrientation",
        [&](const Values&) {
            _eyeOrientation = glm::normalize(_eyeOrientation);
            return true;
        },
        {});
    setAttributeDescription("normalizeEyeOrientation", "Normalize the vector for eye orientation");

    addAttribute(
        "replaceMesh",
        [&](const Values& args) {
            _replaceMesh = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_replaceMesh}; },
        {'b'});
    setAttributeDescription("replaceMesh", "Replace the selected mesh with the new one with computed texture coordinates");

    addAttribute(
        "fov",
        [&](const Values& args) {
            _fov = std::clamp(args[0].as<float>(), 0.f, 360.f);
            return true;
        },
        [&]() -> Values { return {_fov}; },
        {'r'});
    setAttributeDescription("fov", "Range of modelisation of a spheric object, in degrees");

    addAttribute(
        "horizonRotation",
        [&](const Values& args) {
            _horizonRotation = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_horizonRotation}; },
        {'r'});
    setAttributeDescription("horizonRotation", "Additional rotation to align texture coordinates with horizon");

    addAttribute(
        "flipHorizontal",
        [&](const Values& args) {
            _flipHorizontal = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_flipHorizontal}; },
        {'b'});
    setAttributeDescription("flipHorizontal", "Mirror the texture coordinates on the horizontal axis");

    addAttribute(
        "flipVertical",
        [&](const Values& args) {
            _flipVertical = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_flipVertical}; },
        {'b'});
    setAttributeDescription("flipVertical", "Mirror the texture coordinates on the vertical axis");

    addAttribute(
        "meshName",
        [&](const Values& args) {
            _meshName = args[0].as<std::string>();
            return true;
        },
        [&]() -> Values { return {_meshName}; },
        {'s'});
    setAttributeDescription("meshname", "The name of the mesh on which to generate texture coordinates");

    addAttribute("generate",
        [&](const Values&) {
            generateTexCoordOnMesh();
            return true;
        },
        {});
    setAttributeDescription("generate", "Generate texture coordinates on the selected mesh");
}

} // namespace Splash
