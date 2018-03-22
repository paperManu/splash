#include "./framebuffer.h"

#include "./utils/log.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{
/*************/
Framebuffer::Framebuffer(RootObject* root) : BaseObject(root)
{
    glCreateFramebuffers(1, &_fbo);

    if (!_depthTexture)
    {
        _depthTexture = make_shared<Texture_Image>(_root, _width, _height, "D", nullptr, _multisample);
        glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture->getTexId(), 0);
    }

    if (!_colorTexture)
    {
        _colorTexture = make_shared<Texture_Image>(_root);
        _colorTexture->setAttribute("clampToEdge", {1});
        _colorTexture->setAttribute("filtering", {0});
        _colorTexture->reset(_width, _height, _16bits ? "RGBA16" : "RGBA", nullptr, _multisample);
        glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);
    }

    GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glNamedFramebufferDrawBuffers(_fbo, 1, fboBuffers);
    GLenum status = glCheckNamedFramebufferStatus(_fbo, GL_DRAW_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        Log::get() << Log::ERROR << "Framebuffer::" << __FUNCTION__ << " - Error while initializing render framebuffer object: " << status << Log::endl;
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    else
    {
        Log::get() << Log::DEBUGGING << "Framebuffer::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;
    }
}

/*************/
Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &_fbo);
}

/*************/
void Framebuffer::bindDraw()
{
    if (_fbo)
    {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_previousFbo);
        if (_multisample)
            glEnable(GL_MULTISAMPLE);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    }
}

/*************/
void Framebuffer::bindRead()
{
    if (_fbo)
    {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_previousFbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    }
}

/*************/
void Framebuffer::blit(const Framebuffer& src, const Framebuffer& dst)
{
    glBlitNamedFramebuffer(
        src.getFboId(), dst.getFboId(), 0, 0, src.getWidth(), src.getHeight(), 0, 0, dst.getWidth(), dst.getHeight(), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

/*************/
float Framebuffer::getDepthAt(float x, float y)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    float depth = 0.f;
    glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return depth;
}

/*************/
void Framebuffer::setParameters(int multisample, bool sixteenbpc, bool srgb, bool cubemap)
{
    _multisample = multisample;
    _16bits = sixteenbpc;
    _srgb = srgb;

    auto spec = _colorTexture->getSpec();

    _depthTexture->reset(spec.width, spec.height, "D", nullptr, _multisample, cubemap);

    if (_srgb)
        _colorTexture->reset(spec.width, spec.height, "sRGBA", nullptr, _multisample, cubemap);
    else if (_16bits)
        _colorTexture->reset(spec.width, spec.height, "RGBA16", nullptr, _multisample, cubemap);
    else
        _colorTexture->reset(spec.width, spec.height, "RGBA", nullptr, _multisample, cubemap);

    glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture->getTexId(), 0);
    glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);
}

/*************/
void Framebuffer::setSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    _depthTexture->setResizable(true);
    _depthTexture->setAttribute("size", {width, height});
    _depthTexture->setResizable(_automaticResize);
    glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture->getTexId(), 0);

    _colorTexture->setResizable(true);
    _colorTexture->setAttribute("size", {width, height});
    _colorTexture->setResizable(_automaticResize);
    glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);

    _width = width;
    _height = height;
}

/*************/
void Framebuffer::setResizable(bool resizable)
{
    _automaticResize = resizable;
    _depthTexture->setResizable(_automaticResize);
    _colorTexture->setResizable(_automaticResize);
}

/*************/
void Framebuffer::unbindDraw()
{
    if (_fbo)
    {
        int currentFbo = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFbo);
        if (static_cast<GLuint>(currentFbo) != _fbo)
        {
            Log::get() << Log::WARNING << "Framebuffer::" << __FUNCTION__ << " - Cannot unbind a FBO which is not bound" << Log::endl;
            return;
        }

        if (_multisample)
            glDisable(GL_MULTISAMPLE);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _previousFbo);
    }
}

/*************/
void Framebuffer::unbindRead()
{
    if (_fbo)
    {
        int currentFbo = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &currentFbo);
        if (static_cast<GLuint>(currentFbo) != _fbo)
        {
            Log::get() << Log::WARNING << "Framebuffer::" << __FUNCTION__ << " - Cannot unbind a FBO which is not bound" << Log::endl;
            return;
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, _previousFbo);
    }
}

} // end of namespace
