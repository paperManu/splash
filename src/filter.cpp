#include "filter.h"

#include "cgUtils.h"
#include "log.h"
#include "scene.h"
#include "timer.h"
#include "texture_image.h"

using namespace std;

namespace Splash {

/*************/
Filter::Filter(std::weak_ptr<RootObject> root)
       : Texture(root)
{
    init();
}

/*************/
void Filter::init()
{
    _type = "filter";
    registerAttributes();

    // If the root object weak_ptr is expired, this means that
    // this object has been created outside of a World or Scene.
    // This is used for getting documentation "offline"
    if (_root.expired())
        return;

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
}

/*************/
Filter::~Filter()
{
    if (_root.expired())
        return;

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
void Filter::unlinkFrom(std::shared_ptr<BaseObject> obj)
{
    if (dynamic_pointer_cast<Texture>(obj).get() != nullptr)
    {
        if (_inTexture.expired())
            return;

        auto inTex = _inTexture.lock();
        auto tex = dynamic_pointer_cast<Texture>(obj);

        _screen->removeTexture(tex);
        if (tex->getName() == inTex->getName())
            _inTexture.reset();
    }
    else if (dynamic_pointer_cast<Image>(obj).get() != nullptr)
    {
        auto textureName = getName() + "_" + obj->getName() + "_tex";
        auto tex = _root.lock()->unregisterObject(textureName);

        if (tex)
        {
            tex->unlinkFrom(obj);
            unlinkFrom(tex);
        }
    }

    Texture::unlinkFrom(obj);
}

/*************/
void Filter::update()
{
    if (_inTexture.expired())
        return;

    if (_updateColorDepth)
        updateColorDepth();

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
    auto shader = _screen->getShader();

    for (auto& weakObject : _linkedObjects)
    {
        auto scene = dynamic_pointer_cast<Scene>(_root.lock());

        auto obj = weakObject.lock();
        if (obj)
        {
            if (obj->getType() == "image")
            {
                Values remainingTime, duration;
                obj->getAttribute("duration", duration);
                obj->getAttribute("remaining", remainingTime);
                if (remainingTime.size() == 1)
                    shader->setAttribute("uniform", {"_filmRemaining", remainingTime[0].asFloat()});
                if (duration.size() == 1)
                    shader->setAttribute("uniform", {"_filmDuration", duration[0].asFloat()});
            }
        }
    }

    shader->setAttribute("uniform", {"_blackLevel", _blackLevel});
    shader->setAttribute("uniform", {"_brightness", _brightness});
    shader->setAttribute("uniform", {"_contrast", _contrast});
    shader->setAttribute("uniform", {"_colorBalance", _colorBalance.x, _colorBalance.y});
    shader->setAttribute("uniform", {"_saturation", _saturation});
}

/*************/
void Filter::setOutput()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);

    _outTexture = make_shared<Texture_Image>(_root);
    _outTexture->reset(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, nullptr);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _outTexture->getTexId(), 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Setup the virtual screen
    _screen = make_shared<Object>(_root);
    _screen->setAttribute("fill", {"filter"});
    GeometryPtr virtualScreen = make_shared<Geometry>(_root);
    _screen->addGeometry(virtualScreen);
}

/*************/
void Filter::updateColorDepth()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    auto spec = _outTexture->getSpec();
    if (_render16bits)
        _outTexture->reset(GL_TEXTURE_2D, 0, GL_RGBA16, spec.width, spec.height, 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
    else
        _outTexture->reset(GL_TEXTURE_2D, 0, GL_RGBA, spec.width, spec.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, nullptr);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _outTexture->getTexId(), 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    _updateColorDepth = false;
}

/*************/
void Filter::registerAttributes()
{
    addAttribute("16bits", [&](const Values& args) {
        bool render16bits = args[0].asInt();
        if (render16bits != _render16bits)
        {
            _render16bits = render16bits;
            _updateColorDepth = true;
        }
        return true;
    }, [&]() -> Values {
        return {(int)_render16bits};
    }, {'n'});
    setAttributeDescription("16bits", "Set to 1 for the filter to be rendered in 16bits per component (otherwise 8bpc)");

    addAttribute("blackLevel", [&](const Values& args) {
        _blackLevel = args[0].asFloat();
        _blackLevel = std::max(0.f, std::min(1.f, _blackLevel));
        return true;
    }, [&]() -> Values {
        return {_blackLevel};
    }, {'n'});
    setAttributeDescription("blackLevel", "Set the black level for the linked texture");

    addAttribute("brightness", [&](const Values& args) {
        _brightness = args[0].asFloat();
        _brightness = std::max(0.f, std::min(2.f, _brightness));
        return true;
    }, [&]() -> Values {
        return {_brightness};
    }, {'n'});
    setAttributeDescription("brightness", "Set the brightness for the linked texture");

    addAttribute("contrast", [&](const Values& args) {
        _contrast = args[0].asFloat();
        _contrast = std::max(0.f, std::min(2.f, _contrast));
        return true;
    }, [&]() -> Values {
        return {_contrast};
    }, {'n'});
    setAttributeDescription("contrast", "Set the contrast for the linked texture");

    addAttribute("colorTemperature", [&](const Values& args) {
        _colorTemperature = args[0].asFloat();
        _colorTemperature = std::max(0.f, std::min(16000.f, _colorTemperature));
        _colorBalance = colorBalanceFromTemperature(_colorTemperature);
        return true;
    }, [&]() -> Values {
        return {_colorTemperature};
    }, {'n'});
    setAttributeDescription("colorTemperature", "Set the color temperature correction for the linked texture");

    addAttribute("saturation", [&](const Values& args) {
        _saturation = args[0].asFloat();
        _saturation = std::max(0.f, std::min(2.f, _saturation));
        return true;
    }, [&]() -> Values {
        return {_saturation};
    }, {'n'});
    setAttributeDescription("saturation", "Set the saturation for the linked texture");
}

} // end of namespace
