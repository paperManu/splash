#include "filter.h"

#include "log.h"
#include "scene.h"
#include "timer.h"
#include "texture_image.h"

using namespace std;

namespace Splash {

/*************/
Filter::Filter(RootObjectWeakPtr root)
       : Texture(root)
{
    init();
}

/*************/
void Filter::init()
{
    _type = "filter";

    // Intialize FBO, textures and everything OpenGL
    glGetError();
    glGenFramebuffers(1, &_fbo);

    setOutput();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    GLenum _status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
	{
        Log::get() << Log::WARNING << "Filter::" << __FUNCTION__ << " - Error while initializing framebuffer object: " << _status << Log::endl;
		return;
	}
    else
        Log::get() << Log::MESSAGE << "Filter::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    GLenum error = glGetError();
    if (error)
    {
        Log::get() << Log::WARNING << "Filter::" << __FUNCTION__ << " - Error while binding framebuffer" << Log::endl;
        _isInitialized = false;
    }
    else
    {
        Log::get() << Log::MESSAGE << "Filter::" << __FUNCTION__ << " - Filter correctly initialized" << Log::endl;
        _isInitialized = true;
    }

    registerAttributes();
}

/*************/
Filter::~Filter()
{
#ifdef DEBUG
    Log::get()<< Log::DEBUGGING << "Filter::~Filter - Destructor" << Log::endl;
#endif

    glDeleteFramebuffers(1, &_fbo);
}

/*************/
void Filter::bind()
{
    _outTexture->bind();
}

/*************/
unordered_map<string, Values> Filter::getShaderUniforms() const
{
    unordered_map<string, Values> uniforms;
    return uniforms;
}

/*************/
bool Filter::linkTo(std::shared_ptr<BaseObject> obj)
{
    // Mandatory before trying to link
    if (!obj || !Texture::linkTo(obj))
        return false;

    if (dynamic_pointer_cast<Texture>(obj).get() != nullptr)
    {
        if (!_inTexture.expired())
            _screen->removeTexture(_inTexture.lock());

        auto tex = dynamic_pointer_cast<Texture>(obj);
        _screen->addTexture(tex);
        _inTexture = tex;

        return true;
    }
    else if (dynamic_pointer_cast<Image>(obj).get() != nullptr)
    {
        auto tex = make_shared<Texture_Image>(_root);
        tex->setName(getName() + "_" + obj->getName() + "_tex");
        if (tex->linkTo(obj))
        {
            _root.lock()->registerObject(tex);
            return linkTo(tex);
        }
        else
        {
            return false;
        }
    }

    return true;
}

/*************/
void Filter::unbind()
{
    _outTexture->unbind();
}

/*************/
bool Filter::unlinkFrom(std::shared_ptr<BaseObject> obj)
{
    auto type = obj->getType();
    if (type.find("texture") != string::npos)
    {
        auto inTex = _inTexture.lock();
        TexturePtr tex = dynamic_pointer_cast<Texture>(obj);
        if (tex->getName() == inTex->getName())
            _inTexture.reset();
    }
    else if (type.find("image") != string::npos)
    {
        auto textureName = getName() + "_" + obj->getName() + "_tex";
        auto tex = _root.lock()->unregisterObject(textureName);

        if (!tex)
            return false;
        else if (tex->unlinkFrom(obj))
            unlinkFrom(tex);
        else
            return false;
    }

    return Texture::unlinkFrom(obj);
}

/*************/
void Filter::update()
{
    if (_inTexture.expired())
        return;

    auto input = _inTexture.lock();
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

    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    _outTexture->generateMipmap();
}

/*************/
void Filter::updateUniforms()
{
    for (auto& weakObject : _linkedObjects)
    {
        auto scene = dynamic_pointer_cast<Scene>(_root.lock());
        auto shader = _screen->getShader();

        auto obj = weakObject.lock();
        if (obj)
        {
            if (obj->getType() == "image")
            {
                Values remainingTime;
                obj->getAttribute("remaining", remainingTime);
                if (remainingTime.size() == 1)
                    shader->setAttribute("uniform", {"_filmRemaining", remainingTime[0].asFloat()});
            }
        }
    }
}

/*************/
void Filter::setOutput()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);

    _outTexture = make_shared<Texture_Image>();
    _outTexture->reset(GL_TEXTURE_2D, 0, GL_RGBA16, 512, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _outTexture->getTexId(), 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Setup the virtual screen
    _screen = make_shared<Object>();
    _screen->setAttribute("fill", {"filter"});
    GeometryPtr virtualScreen = make_shared<Geometry>();
    _screen->addGeometry(virtualScreen);
}

/*************/
void Filter::registerAttributes()
{
}

} // end of namespace
