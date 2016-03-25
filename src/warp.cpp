#include "warp.h"

#include "cgUtils.h"
#include "log.h"
#include "scene.h"
#include "timer.h"
#include "texture_image.h"

using namespace std;

namespace Splash {

/*************/
Warp::Warp(RootObjectWeakPtr root)
       : Texture(root)
{
    init();
}

/*************/
void Warp::init()
{
    _type = "warp";

    // Intialize FBO, textures and everything OpenGL
    glGetError();
    glGenFramebuffers(1, &_fbo);

    setOutput();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    GLenum _status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
	{
        Log::get() << Log::WARNING << "Warp::" << __FUNCTION__ << " - Error while initializing framebuffer object: " << _status << Log::endl;
		return;
	}
    else
        Log::get() << Log::MESSAGE << "Warp::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    GLenum error = glGetError();
    if (error)
    {
        Log::get() << Log::WARNING << "Warp::" << __FUNCTION__ << " - Error while binding framebuffer" << Log::endl;
        _isInitialized = false;
    }
    else
    {
        Log::get() << Log::MESSAGE << "Warp::" << __FUNCTION__ << " - Warp correctly initialized" << Log::endl;
        _isInitialized = true;
    }

    registerAttributes();
}

/*************/
Warp::~Warp()
{
#ifdef DEBUG
    Log::get()<< Log::DEBUGGING << "Warp::~Warp - Destructor" << Log::endl;
#endif

    glDeleteFramebuffers(1, &_fbo);
}

/*************/
void Warp::bind()
{
    _outTexture->bind();
}

/*************/
unordered_map<string, Values> Warp::getShaderUniforms() const
{
    unordered_map<string, Values> uniforms;
    return uniforms;
}

/*************/
bool Warp::linkTo(std::shared_ptr<BaseObject> obj)
{
    // Mandatory before trying to link
    if (!obj || !Texture::linkTo(obj))
        return false;

    if (dynamic_pointer_cast<Camera>(obj).get() != nullptr)
    {
        auto camera = _inCamera.lock();
        if (camera)
        {
            auto textures = camera->getTextures();
            for (auto& tex : textures)
                _screen->removeTexture(tex);
        }

        camera = dynamic_pointer_cast<Camera>(obj);
        auto textures = camera->getTextures();
        for (auto& tex : textures)
            _screen->addTexture(tex);
        _inCamera = camera;

        return true;
    }

    return true;
}

/*************/
void Warp::unbind()
{
    _outTexture->unbind();
}

/*************/
bool Warp::unlinkFrom(std::shared_ptr<BaseObject> obj)
{
    if (dynamic_pointer_cast<Camera>(obj).get() != nullptr)
    {
        if (_inCamera.expired())
            return false;

        auto inCamera = _inCamera.lock();
        auto camera = dynamic_pointer_cast<Camera>(obj);

        if (inCamera != camera)
            return false;

        auto textures = camera->getTextures();
        for (auto& tex : textures)
            _screen->removeTexture(tex);

        if (camera->getName() == inCamera->getName())
            _inCamera.reset();

        return true;
    }

    return Texture::unlinkFrom(obj);
}

/*************/
void Warp::update()
{
    if (_inCamera.expired())
        return;

    auto camera = _inCamera.lock();
    auto input = camera->getTextures()[0];

    _outTextureSpec = input->getSpec();
    _outTexture->resize(_outTextureSpec.width, _outTextureSpec.height);
    glViewport(0, 0, _outTextureSpec.width, _outTextureSpec.height);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, fboBuffers);
    glDisable(GL_DEPTH_TEST);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _screen->activate();
    updateUniforms();
    _screen->draw();
    _screen->deactivate();

    if (_showControlPoints)
    {
        _screen->setAttribute("fill", {"warpControl"});
        _screenMesh->switchMeshes(true);

        _screen->activate();
        updateUniforms();
        _screen->draw();
        _screen->deactivate();

        _screen->setAttribute("fill", {"warp"});
        _screenMesh->switchMeshes(false);
    }

    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    _outTexture->generateMipmap();
}

/*************/
void Warp::updateUniforms()
{
    auto shader = _screen->getShader();
}
/*************/
int Warp::pickControlPoint(glm::vec2 p, glm::vec2& v)
{
    float distance = numeric_limits<float>::max();
    glm::vec2 closestVertex;

    _screenMesh->switchMeshes(true);
    _screenMesh->update();

    auto vertices = _screenMesh->getControlPoints();
    int index = -1;
    for (int i = 0; i < vertices.size(); ++i)
    {
        float dist = glm::length(p - vertices[i]);
        if (dist < distance)
        {
            closestVertex = vertices[i];
            distance = dist;
            index = i;
        }
    }

    v = closestVertex;

    _screenMesh->switchMeshes(false);

    return index;
}


/*************/
void Warp::setOutput()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);

    _outTexture = make_shared<Texture_Image>();
    _outTexture->reset(GL_TEXTURE_2D, 0, GL_RGBA16, 512, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _outTexture->getTexId(), 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Setup the virtual screen
    _screen = make_shared<Object>();
    _screen->setAttribute("fill", {"warp"});
    GeometryPtr virtualScreen = make_shared<Geometry>();
    _screenMesh = make_shared<Mesh_BezierPatch>();
    virtualScreen->linkTo(_screenMesh);
    _screen->addGeometry(virtualScreen);
}

/*************/
void Warp::registerAttributes()
{
    _attribFunctions["patchControl"] = AttributeFunctor([&](const Values& args) {
        if (!_screenMesh)
            return false;
        return _screenMesh->setAttribute("patchControl", args);
    }, [&]() -> Values {
        if (!_screenMesh)
            return {};

        Values v;
        _screenMesh->getAttribute("patchControl", v);
        return v;
    });

    _attribFunctions["patchResolution"] = AttributeFunctor([&](const Values& args) {
        if (!_screenMesh)
            return false;
        return _screenMesh->setAttribute("patchResolution", args);
    }, [&]() -> Values {
        if (!_screenMesh)
            return {};

        Values v;
        _screenMesh->getAttribute("patchResolution", v);
        return v;
    });

    _attribFunctions["patchSize"] = AttributeFunctor([&](const Values& args) {
        if (!_screenMesh)
            return false;
        return _screenMesh->setAttribute("patchSize", args);
    }, [&]() -> Values {
        if (!_screenMesh)
            return {};

        Values v;
        _screenMesh->getAttribute("patchSize", v);
        return v;
    });

    _attribFunctions["showControlPoints"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;
        _showControlPoints = args[0].asInt();
        return true;
    });
}

} // end of namespace
