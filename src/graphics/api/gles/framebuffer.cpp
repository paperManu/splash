#include "./graphics/api/gles/framebuffer.h"

#include "./graphics/api/gles/texture_image_gfx_impl.h"
#include "./graphics/texture_image.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash::gfx::gles
{
/*************/
Framebuffer::Framebuffer()
{
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);

    if (!_depthTexture)
    {
        _depthTexture = std::make_shared<Texture_Image>(nullptr, std::make_unique<gfx::gles::Texture_ImageGfxImpl>());
        assert(_depthTexture != nullptr);
        _depthTexture->reset(_width, _height, "D", _multisample);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->getTexId(), 0);
    }

    if (!_colorTexture)
    {
        _colorTexture = std::make_shared<Texture_Image>(nullptr, std::make_unique<gfx::gles::Texture_ImageGfxImpl>());
        assert(_colorTexture != nullptr);
        _colorTexture->reset(_width, _height, "RGBA", _multisample);
        _colorTexture->setAttribute("clampToEdge", {true});
        _colorTexture->setAttribute("filtering", {false});
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture->getTexId(), 0);
    }

    GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, fboBuffers);
    const auto status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        Log::get() << Log::ERROR << "Framebuffer::" << __FUNCTION__ << " - Error while initializing render framebuffer object: " << status << Log::endl;
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    else
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Framebuffer::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;
#endif
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
void Framebuffer::blit(const Splash::gfx::Framebuffer* dst)
{
    const auto destFbo = dynamic_cast<const Splash::gfx::gles::Framebuffer*>(dst);
    assert(destFbo != nullptr);
    glBlitNamedFramebuffer(
        getFboId(), destFbo->getFboId(), 0, 0, getWidth(), getHeight(), 0, 0, destFbo->getWidth(), destFbo->getHeight(), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

/*************/
float Framebuffer::getDepthAt(float x, float y)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);

    GLfloat depth = 0.f;
    glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return depth;
}

/*************/
void Framebuffer::setRenderingParameters()
{
    auto spec = _colorTexture->getSpec();

    _depthTexture->reset(spec.width, spec.height, "D", _multisample, _cubemap);

    if (_srgb)
        _colorTexture->reset(spec.width, spec.height, "sRGBA", _multisample, _cubemap);
    else
        _colorTexture->reset(spec.width, spec.height, "RGBA", _multisample, _cubemap);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->getTexId(), 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture->getTexId(), 0);
}

/*************/
void Framebuffer::setSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    _depthTexture->setResizable(true);
    _depthTexture->setAttribute("size", {width, height});
    _depthTexture->setResizable(_automaticResize);

    _colorTexture->setResizable(true);
    _colorTexture->setAttribute("size", {width, height});
    _colorTexture->setResizable(_automaticResize);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->getTexId(), 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture->getTexId(), 0);

    _width = width;
    _height = height;
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

} // namespace Splash::gfx::gles