#include "./graphics/warp.h"

#include <fstream>

#include "./core/scene.h"
#include "./graphics/texture_image.h"
#include "./utils/cgutils.h"
#include "./utils/log.h"
#include "./utils/scope_guard.h"
#include "./utils/timer.h"

#define CONTROL_POINT_SCALE 0.02
#define WORLDMARKER_SCALE 0.0003
#define MARKER_SET                                                                                                                                                                 \
    {                                                                                                                                                                              \
        1.0, 0.5, 0.0, 1.0                                                                                                                                                         \
    }

namespace Splash
{

/*************/
Warp::Warp(RootObject* root)
    : Texture(root)
{
    init();
}

/*************/
void Warp::init()
{
    _type = "warp";
    _renderingPriority = Priority::POST_CAMERA;
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    // Intialize FBO, textures and everything OpenGL
    setupFBO();

    loadDefaultModels();
}

/*************/
Warp::~Warp()
{
    if (!_root)
        return;

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Warp::~Warp - Destructor" << Log::endl;
#endif
}

/*************/
void Warp::bind()
{
    _fbo->getColorTexture()->bind();
}

/*************/
std::unordered_map<std::string, Values> Warp::getShaderUniforms() const
{
    auto spec = _fbo->getColorTexture()->getSpec();
    std::unordered_map<std::string, Values> uniforms;
    uniforms["size"] = {static_cast<float>(_spec.width), static_cast<float>(spec.height)};
    return uniforms;
}

/*************/
bool Warp::linkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (!_inTexture.expired() && !_inCamera.expired())
        return false;

    if (auto camera = std::dynamic_pointer_cast<Camera>(obj); camera != nullptr)
    {
        _screen->addTexture(camera->getTexture());
        _inCamera = camera;

        return true;
    }
    else if (auto texture = std::dynamic_pointer_cast<Texture>(obj); texture != nullptr)
    {
        _screen->addTexture(texture);
        _inTexture = texture;

        return true;
    }

    return false;
}

/*************/
void Warp::unbind()
{
    _fbo->getColorTexture()->unbind();
}

/*************/
void Warp::unlinkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (auto camera = std::dynamic_pointer_cast<Camera>(obj); camera != nullptr)
    {
        auto inCamera = _inCamera.lock();
        if (inCamera == camera)
        {
            _screen->removeTexture(camera->getTexture());
            _inCamera.reset();
        }
    }
    else if (auto texture = std::dynamic_pointer_cast<Texture>(obj); texture != nullptr)
    {
        auto inTexture = _inTexture.lock();
        if (inTexture == texture)
        {
            _screen->removeTexture(texture);
            _inTexture.reset();
        }
    }
}

/*************/
void Warp::render()
{
#ifdef DEBUGGL
    glDebugMessageCallback(Renderer::glMsgCallback, reinterpret_cast<void*>(this));

    OnScopeExit
    {
        glDebugMessageCallback(Renderer::glMsgCallback, reinterpret_cast<void*>(_root));
    };
#endif

    std::shared_ptr<Texture> input(nullptr);

    if (!_inCamera.expired())
    {
        auto camera = _inCamera.lock();
        input = camera->getTexture();
    }
    else if (!_inTexture.expired())
    {
        input = _inTexture.lock();
    }
    else
    {
        return;
    }

    auto inputSpec = input->getSpec();
    if (inputSpec != _spec)
    {
        _spec = inputSpec;
        _fbo->setSize(inputSpec.width, inputSpec.height);
    }

    _fbo->bindDraw();
    glViewport(0, 0, _spec.width, _spec.height);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _screen->activate();
    _screen->draw();
    _screen->deactivate();

    if (_showControlPoints)
    {
        _screen->setAttribute("fill", {"warpControl"});
        _screenMesh->switchMeshes(true);

        _screen->activate();
        _screen->draw();
        _screen->deactivate();

        _screen->setAttribute("fill", {"warp"});
        _screenMesh->switchMeshes(false);

        if (_selectedControlPointIndex != -1)
        {
            auto scene = dynamic_cast<Scene*>(_root);
            assert(scene != nullptr);

            auto pointModel = scene->getObjectLibrary().getModel("3d_marker");

            auto controlPoints = _screenMesh->getControlPoints();
            auto point = controlPoints[_selectedControlPointIndex];

            pointModel->setAttribute("position", {point.x, point.y, 0.f});
            pointModel->setAttribute("rotation", {0.f, 90.f, 0.f});
            pointModel->setAttribute("scale", {CONTROL_POINT_SCALE});
            pointModel->activate();
            pointModel->setViewProjectionMatrix(glm::dmat4(1.f), glm::dmat4(1.f));
            pointModel->draw();
            pointModel->deactivate();
        }
    }

    _fbo->unbindDraw();

    auto colorTexture = _fbo->getColorTexture();
    colorTexture->generateMipmap();
    if (_grabMipmapLevel >= 0)
    {
        _mipmapBuffer = colorTexture->grabMipmap(_grabMipmapLevel).getRawBuffer();
        auto spec = colorTexture->getSpec();
        _mipmapBufferSpec = {spec.width, spec.height, spec.channels, spec.bpp, spec.format};
    }

    colorTexture->setTimestamp(input->getTimestamp());
    _spec.timestamp = input->getTimestamp();
}

/*************/
int Warp::pickControlPoint(glm::vec2 p, glm::vec2& v)
{
    float distance = std::numeric_limits<float>::max();

    _screenMesh->switchMeshes(true);
    _screenMesh->update();

    auto vertices = _screenMesh->getControlPoints();
    int index = -1;
    for (uint32_t i = 0; i < vertices.size(); ++i)
    {
        float dist = glm::length(p - vertices[i]);
        if (dist < distance)
        {
            v = vertices[i];
            distance = dist;
            index = i;
        }
    }

    _screenMesh->switchMeshes(false);

    return index;
}

/*************/
void Warp::loadDefaultModels()
{
    std::map<std::string, std::string> files{{"3d_marker", "3d_marker.obj"}};

    auto scene = dynamic_cast<Scene*>(_root);
    assert(scene != nullptr);

    for (auto& file : files)
    {
        if (!scene->getObjectLibrary().loadModel(file.first, file.second))
            continue;

        auto object = scene->getObjectLibrary().getModel(file.first);
        assert(object != nullptr);

        object->setAttribute("fill", {"color"});
    }
}

/*************/
void Warp::setupFBO()
{
    _fbo = std::make_unique<Framebuffer>(_root);
    _fbo->setSixteenBpc(true);

    // Setup the virtual screen
    _screen = std::make_shared<Object>(_root);
    _screen->setAttribute("fill", {"warp"});
    auto virtualScreen = std::make_shared<Geometry>(_root);
    _screenMesh = std::make_shared<Mesh_BezierPatch>(_root);
    virtualScreen->linkTo(_screenMesh);
    _screen->addGeometry(virtualScreen);
}

/*************/
void Warp::registerAttributes()
{
    Texture::registerAttributes();

    addAttribute(
        "patchControl",
        [&](const Values& args)
        {
            if (!_screenMesh)
                return false;
            return _screenMesh->setAttribute("patchControl", args) != BaseObject::SetAttrStatus::failure;
        },
        [&]() -> Values
        {
            if (!_screenMesh)
                return {};

            Values v;
            _screenMesh->getAttribute("patchControl", v);
            return v;
        },
        {});
    setAttributeDescription("patchControl", "Set the control points positions");

    addAttribute(
        "patchResolution",
        [&](const Values& args)
        {
            if (!_screenMesh)
                return false;
            return _screenMesh->setAttribute("patchResolution", args) != BaseObject::SetAttrStatus::failure;
        },
        [&]() -> Values
        {
            if (!_screenMesh)
                return {};

            Values v;
            _screenMesh->getAttribute("patchResolution", v);
            return v;
        },
        {'i'});
    setAttributeDescription("patchResolution", "Set the Bezier patch final resolution");

    addAttribute(
        "patchSize",
        [&](const Values& args)
        {
            if (!_screenMesh)
                return false;
            Values size = {std::min(8, args[0].as<int>()), std::min(8, args[1].as<int>())};
            return _screenMesh->setAttribute("patchSize", size) != BaseObject::SetAttrStatus::failure;
        },
        [&]() -> Values
        {
            if (!_screenMesh)
                return {};

            Values v;
            _screenMesh->getAttribute("patchSize", v);
            return v;
        },
        {'i', 'i'});
    setAttributeDescription("patchSize", "Set the Bezier patch control resolution");

    addAttribute(
        "size",
        [&](const Values&) { return true; },
        [&]() -> Values
        {
            if (_fbo)
                return {_fbo->getWidth(), _fbo->getHeight()};
            else
                return {0, 0};
        },
        {'i', 'i'});
    setAttributeDescription("size", "Size of the rendered output");

    // Show the Bezier patch describing the warp
    // Also resets the selected control point if hidden
    addAttribute("showControlLattice",
        [&](const Values& args)
        {
            _showControlPoints = args[0].as<bool>();
            if (!_showControlPoints)
                _selectedControlPointIndex = -1;
            return true;
        },
        {'b'});
    setAttributeDescription("showControlLattice", "If true, show the control lattice");

    // Show a single control point
    addAttribute("showControlPoint",
        [&](const Values& args)
        {
            auto index = args[0].as<int>();
            if (index < 0 || index >= static_cast<int>(_screenMesh->getControlPoints().size()))
                _selectedControlPointIndex = -1;
            else
                _selectedControlPointIndex = index;
            return true;
        },
        {'i'});
    setAttributeDescription("showControlPoint", "Show the control point given its index");

    //
    // Mipmap capture
    addAttribute(
        "grabMipmapLevel",
        [&](const Values& args)
        {
            _grabMipmapLevel = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_grabMipmapLevel}; },
        {'i'});
    setAttributeDescription("grabMipmapLevel", "If set to 0 or superior, sync the rendered texture to the 'buffer' attribute, at the given mipmap level");

    addAttribute(
        "buffer", [&](const Values&) { return true; }, [&]() -> Values { return {_mipmapBuffer}; }, {});
    setAttributeDescription("buffer", "Getter attribute which gives access to the mipmap image, if grabMipmapLevel is greater or equal to 0");

    addAttribute(
        "bufferSpec", [&](const Values&) { return true; }, [&]() -> Values { return _mipmapBufferSpec; }, {});
    setAttributeDescription("bufferSpec", "Getter attribute to the specs of the attribute buffer");
}

} // namespace Splash
