/*
 * Copyright (C) 2023 Splash authors
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

#ifndef SPLASH_GFX_TEXTURE_IMAGE_IMPL_H
#define SPLASH_GFX_TEXTURE_IMAGE_IMPL_H

#include "./core/imagebuffer.h"
#include "./image/image.h"

namespace Splash::gfx
{

class Texture_ImageImpl
{
  public:
    virtual ~Texture_ImageImpl() = default;

    /**
     * Bind this texture
     */
    virtual void bind() = 0;

    /**
     * Unbind this texture
     */
    virtual void unbind() = 0;

    /**
     * Generate the mipmaps for the texture
     */
    virtual void generateMipmap() const = 0;

    /**
     * Get the id of the texture (API dependent)
     * \return Return the texture id
     */
    virtual uint getTexId() const = 0;

    /**
     * Enable / disable clamp to edge
     * \param active If true, enables clamping
     */
    virtual void setClampToEdge(bool active) = 0;

    /**
     * Read the texture and returns an Image
     * \param level The mipmap level we wish to read the texture at.
     * \return Return the image
     */
    virtual std::shared_ptr<Image> read(RootObject* root, int mipmapLevel, ImageBufferSpec spec) const = 0;

    /**
     * Resets the texture on the GPU side using the given parameters.
     * \param width Texture width
     * \param height Texture height
     * \param pixelFormat Determines the number of channels, type of channels, bits per channel, etc.
     * \param multisample The multisampling level for the texture
     * \param cubemap Whether or not the texture should be a cubemap
     * \param filtering Whether or not the texture should be filtered (mipmapped)
     *
     */
    virtual ImageBufferSpec reset(int width, int height, std::string pixelFormat, ImageBufferSpec spec, int multisample, bool cubemap, bool filtering) = 0;

    /**
     * \return Whether or not the given spec is for a compressed texture (API dependent)
     */
    virtual bool isCompressed(const ImageBufferSpec& spec) const = 0;

    /*
     * Updates the GPU texture given an image, its spec, the owning CPU-side texture spec, and other data.
     * \param img The image to read data from
     * \param imgSpec the spec of the passed image. Passed to the function since the getter is called in the owning `Texture_Image`, each call of the getter creates a mutex.
     * \param textureSpec the spec of the CPU-side `Texture_Image`
     * \param filter Whether or not the texture should have filtering enabled
     * \return A pair of {textureUpdated, optional(updatedSpec)}, the former denotes whether or not the GPU-side texture was successfully updated, the latter contains the data of
     * the updated spec if an update was required. The updated spec is a modified version of the passed in `imgSpec` which mainly depends on whether or not the texture is
     * compressed.
     */
    virtual std::pair<bool, std::optional<ImageBufferSpec>> update(std::shared_ptr<Image> img, ImageBufferSpec imgSpec, const ImageBufferSpec& textureSpec, bool filtering) = 0;

  public:
    // Maximum number of mipmap levels
    constexpr inline static int _texLevels = 4;
};
} // namespace Splash::gfx

#endif
