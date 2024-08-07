#include "./graphics/api/opengl/framebuffer_gfx_impl.h"

#include "./graphics/api/opengl/texture_image_gfx_impl.h"
#include "./graphics/texture_image.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash::gfx::opengl
{

/*************/
FramebufferGfxImpl::FramebufferGfxImpl()
{
    glCreateFramebuffers(1, &_fbo);

    if (!_depthTexture)
    {
        _depthTexture = std::make_shared<Texture_Image>(nullptr, std::make_unique<gfx::opengl::Texture_ImageGfxImpl>());
        assert(_depthTexture != nullptr);
        _depthTexture->reset(_width, _height, "D", _multisample);
        glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture->getTexId(), 0);
    }

    if (!_colorTexture)
    {
        _colorTexture = std::make_shared<Texture_Image>(nullptr, std::make_unique<gfx::opengl::Texture_ImageGfxImpl>());
        assert(_colorTexture != nullptr);
        _colorTexture->reset(_width, _height, "RGBA", _multisample);
        _colorTexture->setAttribute("clampToEdge", {true});
        _colorTexture->setAttribute("filtering", {false});
        glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);
    }

    GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glNamedFramebufferDrawBuffers(_fbo, 1, fboBuffers);
    const auto status = glCheckNamedFramebufferStatus(_fbo, GL_DRAW_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        Log::get() << Log::ERROR << "FramebufferGfxImpl::" << __FUNCTION__ << " - Error while initializing render framebuffer object: " << status << Log::endl;
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    else
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "FramebufferGfxImpl::" << __FUNCTION__ << " - FramebufferGfxImpl object successfully initialized" << Log::endl;
#endif
    }
}

/*************/
FramebufferGfxImpl::~FramebufferGfxImpl()
{
    glDeleteFramebuffers(1, &_fbo);
}

/*************/
void FramebufferGfxImpl::bindDraw()
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
void FramebufferGfxImpl::bindRead()
{
    if (_fbo)
    {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_previousFbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    }
}

/*************/
void FramebufferGfxImpl::blit(const Splash::gfx::FramebufferGfxImpl* dst)
{
    const auto destFbo = dynamic_cast<const Splash::gfx::opengl::FramebufferGfxImpl*>(dst);
    assert(destFbo != nullptr);
    glBlitNamedFramebuffer(
        getFboId(), destFbo->getFboId(), 0, 0, getWidth(), getHeight(), 0, 0, destFbo->getWidth(), destFbo->getHeight(), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

/*************/
float FramebufferGfxImpl::getDepthAt(float x, float y)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);

    GLfloat depth = 0.f;
    glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return depth;
}

/*************/
void FramebufferGfxImpl::setRenderingParameters()
{
    auto spec = _colorTexture->getSpec();

    _depthTexture->reset(spec.width, spec.height, "D", _multisample, _cubemap);

    if (_srgb)
        _colorTexture->reset(spec.width, spec.height, "sRGBA", _multisample, _cubemap);
    else if (_16bits)
        _colorTexture->reset(spec.width, spec.height, "RGBA16", _multisample, _cubemap);
    else
        _colorTexture->reset(spec.width, spec.height, "RGBA", _multisample, _cubemap);

    glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture->getTexId(), 0);
    glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _colorTexture->getTexId(), 0);
}

/*************/
void FramebufferGfxImpl::setSize(int width, int height)
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
void FramebufferGfxImpl::unbindDraw()
{
    if (_fbo)
    {
        int currentFbo = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFbo);
        if (static_cast<GLuint>(currentFbo) != _fbo)
        {
            Log::get() << Log::WARNING << "FramebufferGfxImpl::" << __FUNCTION__ << " - Cannot unbind a FBO which is not bound" << Log::endl;
            return;
        }

        if (_multisample)
            glDisable(GL_MULTISAMPLE);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _previousFbo);
    }
}

/*************/
void FramebufferGfxImpl::unbindRead()
{
    if (_fbo)
    {
        int currentFbo = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &currentFbo);
        if (static_cast<GLuint>(currentFbo) != _fbo)
        {
            Log::get() << Log::WARNING << "FramebufferGfxImpl::" << __FUNCTION__ << " - Cannot unbind a FBO which is not bound" << Log::endl;
            return;
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, _previousFbo);
    }
}

} // namespace Splash::gfx::opengl
