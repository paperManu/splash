#include "./virtual_probe.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace std;
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

    setupFBO();

    // One projection matrix to render them all
    _faceProjectionMatrix = getProjectionMatrix(90.0, 0.1, 1000.0, _width, _height, 0.5, 0.5);
}

/*************/
VirtualProbe::~VirtualProbe()
{
    if (!_root)
        return;

    glDeleteFramebuffers(2, _fbo);
}

/*************/
void VirtualProbe::bind()
{
    _outColorTexture->bind();
}

/*************/
void VirtualProbe::unbind()
{
    _outColorTexture->unbind();
}

/*************/
unordered_map<string, Values> VirtualProbe::getShaderUniforms() const
{
    unordered_map<string, Values> uniforms;
    return uniforms;
}

/*************/
bool VirtualProbe::linkTo(const shared_ptr<BaseObject>& obj)
{
    // Mandatory before trying to link
    if (!obj || !Texture::linkTo(obj))
        return false;

    if (dynamic_pointer_cast<Object>(obj).get() != nullptr)
    {
        auto obj3D = dynamic_pointer_cast<Object>(obj);
        _objects.push_back(obj3D);
        return true;
    }

    return true;
}

/*************/
void VirtualProbe::unlinkFrom(const std::shared_ptr<BaseObject>& obj)
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

    Texture::unlinkFrom(obj);
}

/*************/
void VirtualProbe::render()
{
    if (_newWidth != 0 && _newHeight != 0)
    {
        setOutputSize(_newWidth, _newHeight);
        _newWidth = 0;
        _newHeight = 0;
    }

    if (!_colorTexture)
        return;

    glViewport(0, 0, _cubemapSize, _cubemapSize);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // First pass: render to the cubemap
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo[0]);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto& o : _objects)
    {
        auto obj = o.lock();
        if (!obj)
            continue;

        Values previousFill;
        obj->getAttribute("fill", previousFill);
        if (!previousFill.empty() && previousFill[0].as<string>() != "object_cubemap")
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

    // Second pass: render the projected cubemap
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo[1]);
    _colorTexture->generateMipmap();

    glViewport(0, 0, _width, _height);
    glClearColor(1.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _screen->activate();
    auto screenShader = _screen->getShader();
    screenShader->setAttribute("uniform", {"_projectionType", static_cast<int>(_projectionType)});
    screenShader->setAttribute("uniform", {"_sphericalFov", _sphericalFov});
    _screen->draw();
    _screen->deactivate();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_MULTISAMPLE);
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
    glCreateFramebuffers(2, _fbo);

    if (!_depthTexture)
    {
        _depthTexture = make_unique<Texture_Image>(_root, _width, _height, "D", nullptr, 0, true);
        _depthTexture->setName("depthCube");
        glNamedFramebufferTexture(_fbo[0], GL_DEPTH_ATTACHMENT, _depthTexture->getTexId(), 0);
    }

    if (!_colorTexture)
    {
        _colorTexture = make_unique<Texture_Image>(_root);
        _colorTexture->setName("colorCube");
        _colorTexture->setAttribute("clampToEdge", {1});
        _colorTexture->setAttribute("filtering", {0});
        _colorTexture->reset(_width, _height, "RGBA", nullptr, 0, true);
        glNamedFramebufferTexture(_fbo[0], GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);
    }

    if (!_outDepthTexture)
    {
        _outDepthTexture = make_unique<Texture_Image>(_root, _width, _height, "D", nullptr);
        glNamedFramebufferTexture(_fbo[1], GL_DEPTH_ATTACHMENT, _outDepthTexture->getTexId(), 0);
    }

    if (!_outColorTexture)
    {
        _outColorTexture = make_unique<Texture_Image>(_root, _width, _height, "RGBA", nullptr);
        glNamedFramebufferTexture(_fbo[1], GL_COLOR_ATTACHMENT0, _outColorTexture->getTexId(), 0);
    }

    GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glNamedFramebufferDrawBuffers(_fbo[0], 1, fboBuffers);
    GLenum status = glCheckNamedFramebufferStatus(_fbo[0], GL_DRAW_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        Log::get() << Log::ERROR << "VirtualProbe::" << __FUNCTION__ << " - Error while initializing render framebuffer object: " << status << Log::endl;
    else
        Log::get() << Log::DEBUGGING << "VirtualProbe::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;

    glNamedFramebufferDrawBuffers(_fbo[1], 1, fboBuffers);
    status = glCheckNamedFramebufferStatus(_fbo[1], GL_DRAW_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        Log::get() << Log::ERROR << "VirtualProbe::" << __FUNCTION__ << " - Error while initializing output framebuffer object: " << status << Log::endl;
    else
        Log::get() << Log::DEBUGGING << "VirtualProbe::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;

    _screen = make_unique<Object>(_root);
    _screen->setAttribute("fill", {"cubemap_projection"});
    auto virtualScreen = make_shared<Geometry>(_root);
    _screen->addGeometry(virtualScreen);
    _screen->addTexture(_colorTexture);
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

    _depthTexture->setResizable(1);
    _depthTexture->setAttribute("size", {_cubemapSize, _cubemapSize});
    _depthTexture->setResizable(0);
    glNamedFramebufferTexture(_fbo[0], GL_DEPTH_ATTACHMENT, _depthTexture->getTexId(), 0);

    _colorTexture->setResizable(1);
    _colorTexture->setAttribute("size", {_cubemapSize, _cubemapSize});
    _colorTexture->setResizable(0);
    glNamedFramebufferTexture(_fbo[0], GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);

    _outDepthTexture->setResizable(1);
    _outDepthTexture->setAttribute("size", {_width, _height});
    _outDepthTexture->setResizable(0);
    glNamedFramebufferTexture(_fbo[1], GL_DEPTH_ATTACHMENT, _outDepthTexture->getTexId(), 0);

    _outColorTexture->setResizable(1);
    _outColorTexture->setAttribute("size", {_width, _height});
    _outColorTexture->setResizable(0);
    glNamedFramebufferTexture(_fbo[1], GL_COLOR_ATTACHMENT0, _outColorTexture->getTexId(), 0);
}

/*************/
void VirtualProbe::registerAttributes()
{
    addAttribute("position",
        [&](const Values& args) {
            _position = dvec3(args[0].as<float>(), args[1].as<float>(), args[2].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_position.x, _position.y, _position.z};
        },
        {'n', 'n', 'n'});
    setAttributeDescription("position", "Set the virtual probe position");

    addAttribute("projection",
        [&](const Values& args) {
            auto type = args[0].as<string>();
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

    addAttribute("rotation",
        [&](const Values& args) {
            _rotation = dvec3(args[0].as<float>(), args[1].as<float>(), args[2].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_rotation.x, _rotation.y, _rotation.z};
        },
        {'n', 'n', 'n'});
    setAttributeDescription("rotation", "Set the virtual probe rotation");

    addAttribute("size",
        [&](const Values& args) {
            _newWidth = args[0].as<int>();
            _newHeight = args[1].as<int>();
            return true;
        },
        [&]() -> Values {
            return {_width, _height};
        },
        {'n', 'n'});
    setAttributeDescription("size", "Set the render size");

    addAttribute("sphericalFov",
        [&](const Values& args) {
            auto value = args[0].as<float>();
            if (value < 1.0 || value > 360.0)
                return false;
            _sphericalFov = value;
            return true;
        },
        [&]() -> Values { return {_sphericalFov}; },
        {'n'});
    setAttributeDescription("sphericalFov", "Field of view for the spherical projection");
}

} // end of namespace
