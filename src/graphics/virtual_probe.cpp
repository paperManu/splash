#include "./graphics/virtual_probe.h"

#include <glm/gtc/matrix_transform.hpp>

#include "./graphics/api/filter_gfx_impl.h"
#include "./utils/scope_guard.h"

using namespace glm;

namespace Splash
{

/*************/
VirtualProbe::VirtualProbe(RootObject* root)
    : Texture(root)
{
    _type = "virtual_probe";
    _renderingPriority = Priority::PRE_CAMERA;
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    _gfxImpl = _renderer->createFilterGfxImpl();
    setupFBO();

    // One projection matrix to render them all
    _faceProjectionMatrix = getProjectionMatrix(90.0, 0.1, 1000.0, _width, _height, 0.5, 0.5);
}

/*************/
VirtualProbe::~VirtualProbe()
{
    if (!_root)
        return;
}

/*************/
void VirtualProbe::bind()
{
    _outFbo->getColorTexture()->bind();
}

/*************/
void VirtualProbe::unbind()
{
    _outFbo->getColorTexture()->unbind();
}

/*************/
std::unordered_map<std::string, Values> VirtualProbe::getShaderUniforms() const
{
    auto spec = _outFbo->getColorTexture()->getSpec();
    std::unordered_map<std::string, Values> uniforms;
    uniforms["size"] = {static_cast<float>(_spec.width), static_cast<float>(spec.height)};
    return uniforms;
}

/*************/
bool VirtualProbe::linkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (std::dynamic_pointer_cast<Object>(obj))
    {
        auto obj3D = std::dynamic_pointer_cast<Object>(obj);
        _objects.push_back(obj3D);
        return true;
    }

    return false;
}

/*************/
void VirtualProbe::unlinkIt(const std::shared_ptr<GraphObject>& obj)
{
    auto objIterator = find_if(_objects.begin(), _objects.end(), [&](const std::weak_ptr<Object> o) {
        if (o.expired())
            return false;
        auto object = o.lock();
        if (object == obj)
            return true;
        return false;
    });

    if (objIterator != _objects.end())
        _objects.erase(objIterator);

    Texture::unlinkIt(obj);
}

/*************/
void VirtualProbe::render()
{
#ifdef DEBUGGL
    gfx::Renderer::setGlMsgCallbackData(getGlMsgCallbackDataPtr());
    OnScopeExit
    {
        gfx::Renderer::setGlMsgCallbackData(_root->getGlMsgCallbackDataPtr());
    };
#endif

    if (_newWidth != 0 && _newHeight != 0)
    {
        setOutputSize(_newWidth, _newHeight);
        _newWidth = 0;
        _newHeight = 0;
    }

    _gfxImpl->setupViewport(_cubemapSize, _cubemapSize);
    _gfxImpl->enableMultisampling();
    _gfxImpl->enableCubemapRendering();

    if (!_fbo || !_outFbo)
        return;

    // First pass: render to the cubemap
    _fbo->bindDraw();

    for (auto& o : _objects)
    {
        auto obj = o.lock();
        if (!obj)
            continue;

        Values previousFill;
        obj->getAttribute("fill", previousFill);
        if (!previousFill.empty() && previousFill[0].as<std::string>() != "object_cubemap")
            obj->setAttribute("fill", {"object_cubemap"});
        else
            previousFill.clear();

        obj->activate();

        obj->setViewProjectionMatrix(computeViewMatrix(), _faceProjectionMatrix);
        obj->draw();
        obj->deactivate();

        if (!previousFill.empty())
            obj->setAttribute("fill", previousFill);
    }
    _fbo->unbindDraw();

    // Second pass: render the projected cubemap
    _outFbo->bindDraw();
    _fbo->getColorTexture()->generateMipmap();

    _gfxImpl->setupViewport(_width, _height);

    _screen->activate();
    auto screenShader = _screen->getShader();
    screenShader->setAttribute("uniform", {"_projectionType", static_cast<int>(_projectionType)});
    screenShader->setAttribute("uniform", {"_sphericalFov", _sphericalFov});
    _screen->draw();
    _screen->deactivate();

    _outFbo->unbindDraw();
    _gfxImpl->disableMultisampling();
    _gfxImpl->disableCubemapRendering();
}

/*************/
glm::dmat4 VirtualProbe::computeViewMatrix() const
{
    return glm::inverse(glm::translate(glm::dmat4(1.f), _position) * glm::rotate(glm::dmat4(1.f), _rotation.z * M_PI / 180.0, glm::dvec3(0.0, 0.0, 1.0)) *
                        glm::rotate(glm::dmat4(1.f), _rotation.y * M_PI / 180.0, glm::dvec3(0.0, 1.0, 0.0)) *
                        glm::rotate(glm::dmat4(1.f), _rotation.x * M_PI / 180.0, glm::dvec3(1.0, 0.0, 0.0)));
}

/*************/
void VirtualProbe::setupFBO()
{
    _fbo = _renderer->createFramebuffer();
    _fbo->setSize(_width, _height);
    _fbo->setCubemap(true);
    _fbo->getColorTexture()->setAttribute("clampToEdge", {true});
    _fbo->getColorTexture()->setAttribute("filtering", {false});

    _outFbo = _renderer->createFramebuffer();
    _outFbo->setSize(_width, _height);

    _screen = std::make_unique<Object>(_root);
    _screen->setAttribute("fill", {"cubemap_projection"});
    auto virtualScreen = _renderer->createGeometry(_root);
    _screen->addGeometry(virtualScreen);
    _screen->addTexture(_fbo->getColorTexture());
}

/*************/
void VirtualProbe::setOutputSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    // We keep the desired width and height
    _width = width;
    _height = height;

    // Find the best render size depending on the projection type
    switch (_projectionType)
    {
    case Equirectangular:
        width = width / 4;
        height = height / 2;
        break;
    case Spherical:
        width = static_cast<float>(width) * _sphericalFov / 360.f;
        height = static_cast<float>(height) * _sphericalFov / 360.f;
        break;
    }

    _cubemapSize = std::max(width, height);
    _fbo->setSize(_cubemapSize, _cubemapSize);
    _outFbo->setSize(_width, _height);
}

/*************/
void VirtualProbe::registerAttributes()
{
    addAttribute(
        "position",
        [&](const Values& args) {
            _position = dvec3(args[0].as<float>(), args[1].as<float>(), args[2].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_position.x, _position.y, _position.z};
        },
        {'r', 'r', 'r'});
    setAttributeDescription("position", "Set the virtual probe position");

    addAttribute(
        "projection",
        [&](const Values& args) {
            auto type = args[0].as<std::string>();
            if (type == "equirectangular")
                _projectionType = Equirectangular;
            else if (type == "spherical")
                _projectionType = Spherical;
            else
                _projectionType = Equirectangular;

            // Force size update to match new projection
            _newWidth = _width;
            _newHeight = _height;

            return true;
        },
        [&]() -> Values {
            switch (_projectionType)
            {
            case Equirectangular:
                return {"equirectangular"};
            case Spherical:
                return {"spherical"};
            default:
                return {""};
            }
        },
        {'s'});
    setAttributeDescription("projection", "Projection type, can be equirectangular or spherical");

    addAttribute(
        "rotation",
        [&](const Values& args) {
            _rotation = dvec3(args[0].as<float>(), args[1].as<float>(), args[2].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_rotation.x, _rotation.y, _rotation.z};
        },
        {'r', 'r', 'r'});
    setAttributeDescription("rotation", "Set the virtual probe rotation");

    addAttribute(
        "size",
        [&](const Values& args) {
            _newWidth = args[0].as<int>();
            _newHeight = args[1].as<int>();
            return true;
        },
        [&]() -> Values {
            return {_width, _height};
        },
        {'i', 'i'});
    setAttributeDescription("size", "Set the render size");

    addAttribute(
        "sphericalFov",
        [&](const Values& args) {
            auto value = args[0].as<float>();
            if (value < 1.0 || value > 360.0)
                return false;
            _sphericalFov = value;
            return true;
        },
        [&]() -> Values { return {_sphericalFov}; },
        {'r'});
    setAttributeDescription("sphericalFov", "Field of view for the spherical projection");
}

} // namespace Splash
