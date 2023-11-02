/*
 * Copyright (C) 2023 Tarek Yasser
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @opengl_texture_image.h
 * Contains OpenGL 4.5 specific functionality for textures.
 */

#ifndef SPLASH_GFX_OPENGL_TEXTURE_IMAGE_H
#define SPLASH_GFX_OPENGL_TEXTURE_IMAGE_H

#include "./graphics/api/gl_base_texture_image_impl.h"

namespace Splash::gfx::opengl
{

class Texture_ImageImpl final : public GlBaseTexture_ImageImpl
{
  public:
    explicit Texture_ImageImpl() = default;

    Texture_ImageImpl(const Texture_ImageImpl&) = delete;
    Texture_ImageImpl& operator=(const Texture_ImageImpl&) = delete;
    Texture_ImageImpl(Texture_ImageImpl&&) = delete;
    Texture_ImageImpl& operator=(Texture_ImageImpl&&) = delete;

    virtual std::unordered_map<std::string, InitTuple> getPixelFormatToInitTable() const final;

    virtual void bind() final;

    virtual void unbind() final;

    virtual void generateMipmap() const final;

    virtual void getTextureImage(GLuint textureId, GLenum /*textureType*/, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) const final;

    virtual void getTextureLevelParameteriv(GLenum /*target*/, GLint level, GLenum pname, GLint* params) const final;

    virtual void getTextureParameteriv(GLenum /*target*/, GLenum pname, GLint* params) const final;

    virtual bool reallocatePBOs(int width, int height, int bytes) final;

    virtual std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb);

    virtual void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const;

    virtual void copyPixelsBetweenPBOs(int imageDataSize) const final;

  private:
    GLubyte* _pbosPixels[2];
};

} // namespace Splash::gfx::opengl

#endif
