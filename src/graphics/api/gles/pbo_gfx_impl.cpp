#include "./graphics/api/gles/pbo_gfx_impl.h"

#include <cassert>

#include "./graphics/api/renderer.h"
#include "./graphics/texture.h"

namespace Splash::gfx::gles
{

/*************/
PboGfxImpl::PboGfxImpl(std::size_t size)
    : gfx::PboGfxImpl(size)
    , _pbos(size, 0)
{
}

/*************/
PboGfxImpl::~PboGfxImpl()
{
    if (_mappedPixels)
        glUnmapNamedBuffer(_pbos[_pboMapReadIndex]);

    glDeleteBuffers(static_cast<GLint>(_pboCount), _pbos.data());
}

/*************/
void PboGfxImpl::activatePixelPack(std::size_t index)
{
    assert(index < _pboCount);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[index]);
}

/*************/
void PboGfxImpl::deactivatePixelPack()
{
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

/*************/
void PboGfxImpl::packTexture(Texture* texture)
{
    const auto spec = texture->getSpec();

    // Nvidia hardware and/or drivers do not like much copying to a PBO
    // from a named texture, it prefers the old way for some unknown reason.
    // Hence the special treatment, even though it's not modern-ish.
    // For reference, the behavior with Nvidia hardware is that the grabbed
    // image is always completely black. It works correctly with AMD and Intel
    // hardware using Mesa driver.
    if (gfx::Renderer::getPlatformVendor() == Constants::GL_VENDOR_NVIDIA)
    {
        texture->bind();
        if (spec.bpp == 64)
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
        else if (spec.bpp == 32)
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
        else if (spec.bpp == 24)
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        else if (spec.bpp == 16 && spec.channels != 1)
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_UNSIGNED_SHORT, 0);
        else if (spec.bpp == 16 && spec.channels == 1)
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_SHORT, 0);
        else if (spec.bpp == 8)
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
        texture->unbind();
    }
    else
    {
        if (spec.bpp == 64)
            glGetTextureImage(texture->getTexId(), 0, GL_RGBA, GL_UNSIGNED_SHORT, 0, 0);
        else if (spec.bpp == 32)
            glGetTextureImage(texture->getTexId(), 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 0, 0);
        else if (spec.bpp == 24)
            glGetTextureImage(texture->getTexId(), 0, GL_RGB, GL_UNSIGNED_BYTE, 0, 0);
        else if (spec.bpp == 16 && spec.channels != 1)
            glGetTextureImage(texture->getTexId(), 0, GL_RG, GL_UNSIGNED_SHORT, 0, 0);
        else if (spec.bpp == 16 && spec.channels == 1)
            glGetTextureImage(texture->getTexId(), 0, GL_RED, GL_UNSIGNED_SHORT, 0, 0);
        else if (spec.bpp == 8)
            glGetTextureImage(texture->getTexId(), 0, GL_RED, GL_UNSIGNED_BYTE, 0, 0);
    }
}

/*************/
uint8_t* PboGfxImpl::mapRead(std::size_t index)
{
    _pboMapReadIndex = static_cast<GLint>(index);
    _mappedPixels = static_cast<uint8_t*>(glMapNamedBufferRange(_pbos[_pboMapReadIndex], 0, static_cast<GLint>(_bufferSize), GL_MAP_READ_BIT));
    return _mappedPixels;
}

/*************/
void PboGfxImpl::unmapRead()
{
    assert(_mappedPixels != nullptr);
    glUnmapNamedBuffer(_pbos[_pboMapReadIndex]);
    _mappedPixels = nullptr;
}

/*************/
void PboGfxImpl::updatePBOs(std::size_t size)
{
    if (size == _bufferSize)
        return;

    if (!_pbos.empty())
        glDeleteBuffers(static_cast<GLint>(_pboCount), _pbos.data());

    glCreateBuffers(static_cast<GLint>(_pboCount), _pbos.data());

    for (const auto pbo : _pbos)
        glNamedBufferData(pbo, static_cast<GLsizeiptr>(size), 0, GL_STREAM_READ);

    _bufferSize = size;
}

} // namespace Splash::gfx::gles
