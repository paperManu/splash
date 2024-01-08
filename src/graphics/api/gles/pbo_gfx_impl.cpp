#include "./graphics/api/gles/pbo_gfx_impl.h"

#include <cassert>

#include "./graphics/api/renderer.h"
#include "./graphics/texture.h"

namespace Splash::gfx::gles
{

/*************/
PboGfxImpl::PboGfxImpl(std::size_t size)
    : gfx::PboGfxImpl(size)
{
    glGenFramebuffers(1, &_fbo);
}

/*************/
PboGfxImpl::~PboGfxImpl()
{
    if (glIsFramebuffer(_fbo))
        glDeleteFramebuffers(1, &_fbo);
}

/*************/
void PboGfxImpl::activatePixelPack(std::size_t /*index*/)
{
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
}

/*************/
void PboGfxImpl::deactivatePixelPack()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*************/
void PboGfxImpl::packTexture(Texture* texture)
{
    const auto spec = texture->getSpec();
    texture->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->getTexId(), 0);
    if (_image == nullptr || _image->getSpec() != spec)
        _image = std::make_unique<Image>(nullptr, spec);
    glReadPixels(0, 0, spec.width, spec.height, GL_RGBA, GL_UNSIGNED_BYTE, const_cast<GLvoid*>(_image->data()));
    texture->unbind();
}

/*************/
uint8_t* PboGfxImpl::mapRead(std::size_t /*index*/)
{
    return static_cast<uint8_t*>(const_cast<void*>(_image->data()));
}

} // namespace Splash::gfx::gles
