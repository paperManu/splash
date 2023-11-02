/*
 * Copyright (C) 2023 Emmanuel Durand
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
 * @texture_image.h
 * The Texture_Image base class. Contains normal and virtual functions needed to use both OpenGL 4.5 and OpenGL ES textures.
 */

#ifndef SPLASH_TEXTURE_IMAGE_H
#define SPLASH_TEXTURE_IMAGE_H

#include <chrono>
#include <future>
#include <glm/glm.hpp>
#include <list>
#include <memory>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./graphics/api/texture_image_impl.h"
#include "./graphics/texture.h"
#include "./image/image.h"
#include "./utils/cgutils.h"

namespace Splash
{

class Texture_Image : public Texture
{
  public:
    explicit Texture_Image(RootObject* root, std::unique_ptr<gfx::Texture_ImageImpl> gfxImpl);

    /**
     * Destructor
     */
    virtual ~Texture_Image();

    /**
     * Bind this texture
     */
    virtual void bind() override { _gfxImpl->bind(); }

    /**
     * Unbind this texture
     */
    virtual void unbind() override
    {
        _gfxImpl->unbind();
        _lastDrawnTimestamp = Timer::getTime();
    };

    /**
     * Generate the mipmaps for the texture
     */
    virtual void generateMipmap() const { _gfxImpl->generateMipmap(); };

    /**
     * Get the id of the texture (API dependant)
     * \return Return the texture id
     */
    GLuint getTexId() const final { return _gfxImpl->getTexId(); }

    /**
     * Computed the mean value for the image
     * \return Return the mean RGB value
     */
    RgbValue getMeanValue() const;

    /**
     * Get the shader parameters related to this texture. Texture should be locked first.
     * \return Return the shader uniforms
     */
    std::unordered_map<std::string, Values> getShaderUniforms() const final;

    /**
     * Grab the texture to the host memory, at the given mipmap level
     * \param level Mipmap level to grab
     * \return Return the image data in an ImageBuffer
     */
    ImageBuffer grabMipmap(unsigned int level = 0) const;

    /**
     * Read the texture and returns an Image
     * \param level The mipmap level we wish to read the texture at.
     * \return Return the image
     */
    std::shared_ptr<Image> read(int level = 0) const;

    /**
     * Set the buffer size / type / internal format
     * \param width Width
     * \param height Height
     * \param pixelFormat String describing the pixel format. Accepted values are RGB, RGBA, sRGBA, RGBA16, R16, YUYV, UYVY, D
     * \param data Pointer to data to use to initialize the texture
     * \param multisample Sample count for MSAA
     * \param cubemap True to request a cubemap
     */
    void reset(int width, int height, const std::string& pixelFormat, int multisample = 0, bool cubemap = false);

    /**
     * Modify the size of the texture
     * \param width Width
     * \param height Height
     */
    void resize(int width, int height);

    /**
     * Enable / disable clamp to edge
     * \param active If true, enables clamping
     */
    void setClampToEdge(bool active) { _gfxImpl->setClampToEdge(active); }

    /**
     * Update the texture according to the owned Image
     */
    void update() final;

    /**
     * Clears and updates the following uniforms: `flip`, `flop`, and `encoding`.
     */
    void updateShaderUniforms(const ImageBufferSpec& spec, const std::shared_ptr<Image> img);

  protected:
    /**
     * Constructors/operators
     */
    Texture_Image(const Texture_Image&) = delete;
    Texture_Image& operator=(const Texture_Image&) = delete;
    Texture_Image(Texture_Image&&) = delete;
    Texture_Image& operator=(Texture_Image&&) = delete;

    /**
     * Sets the specified buffer as the texture on the device
     * \param img Image to set the texture from
     */
    Texture_Image& operator=(const std::shared_ptr<Image>& img);

    /**
     * Try to link the given GraphObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkIt(const std::shared_ptr<GraphObject>& obj) final;

    /**
     * Unlink a given object
     * \param obj Object to unlink from
     */
    void unlinkIt(const std::shared_ptr<GraphObject>& obj) final;

  protected:
    enum ColorEncoding : int32_t
    {
        RGB = 0,
        BGR = 1,
        UYVY = 2,
        YUYV = 3,
        YCoCg = 4
    };

    int64_t _lastDrawnTimestamp{0};

    std::string _pixelFormat{"RGBA"};
    int _multisample{0};
    bool _cubemap{false}, _filtering;

    std::weak_ptr<Image> _img;

    // Parameters to send to the shader
    std::unordered_map<std::string, Values> _shaderUniforms;

    /**
     * Initialization
     */
    void init();

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();

  private:
    std::unique_ptr<gfx::Texture_ImageImpl> _gfxImpl;
};

} // namespace Splash

#endif // SPLASH_TEXTURE_H
