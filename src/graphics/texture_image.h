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
#include "./graphics/texture.h"
#include "./image/image.h"
#include "./utils/cgutils.h"

namespace Splash
{

class Texture_Image : public Texture
{
  public:
    /**
     * Bind this texture
     */
    virtual void bind() override = 0;

    /**
     * Unbind this texture
     */
    virtual void unbind() override = 0;

    /**
     * Generate the mipmaps for the texture
     */
    virtual void generateMipmap() const = 0;

    /**
     * Computed the mean value for the image
     * \return Return the mean RGB value
     */
    RgbValue getMeanValue() const;

    /**
     * Get the id of the gl texture
     * \return Return the texture id
     */
    GLuint getTexId() const final { return _glTex; }

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
     * \return Return the image
     */
    std::shared_ptr<Image> read();

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
    void setClampToEdge(bool active) { _glTextureWrap = active ? GL_CLAMP_TO_EDGE : GL_REPEAT; }

    /**
     * Update the texture according to the owned Image
     */
    void update() final;

  protected:
    explicit Texture_Image(RootObject* root);

    /**
     * Destructor
     */
    virtual ~Texture_Image();

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

    /**
     * Uses `_pixelFormat` to initialize `_spec`, `_texInternalFormat`, `_texFormat`, and `_texType`. Different graphics APIs might have different initializations due to supporting
     * different combinations of the format, internal format, and type.
     * \param width Width of the texture
     * \param height Height of the texture
     * \sa getPixelFormatToInitTable()
     */
    void initFromPixelFormat(int width, int height);

    /**
     * Used only in `getPixelFormatToInitTable` at the moment. Can be replaced by an `std::tuple`, but the field names might be helpful.
     */
    struct InitTuple
    {
        uint numChannels, bitsPerChannel;
        ImageBufferSpec::Type pixelBitFormat;
        std::string stringName;
        GLenum texInternalFormat, texFormat, texType;
    };

    /**
     * Note that this table is constant throughout the life of the program, so it might be opimized further if it proves to be a bottleneck.
     * \return A graphics API specific table to initialize `_spec`, `_texInternalFormat`, `_texFormat`, and `_texType` depending on `_pixelFormat`.
     * \sa https://docs.gl/es3/glTexStorage2D
     * \sa https://docs.gl/gl4/glTexStorage2D
     */
    virtual std::unordered_map<std::string, InitTuple> getPixelFormatToInitTable() const = 0;

    /**
     * Wrapper for OpenGL's `getTextureLevelParameteriv` and `getTexLevelParameteriv`
     */
    virtual void getTextureLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) const = 0;

    /**
     * Wrapper for OpenGL's `getTextureParameteriv` and `getTexParameteriv`
     */
    virtual void getTextureParameteriv(GLenum target, GLenum pname, GLint* params) const = 0;

    /**
     * Reads an  OpenGL texture into a pre-allocated buffer on the CPU. Deviates from the OpenGL API by taking both the texture ID and texture type. This is needed to accomodate
     * OpenGL ES.
     */
    virtual void getTextureImage(GLuint textureId, GLenum textureType, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) const = 0;

    /**
     * Sets up texture wrapping, minification and magnification filters, anistropy, and packing/unpacking alignment. Note that all of the OpenGL calls used in this function are
     * supported by OpenGL 4.5 and OpenGL ES, so no need to virtualize them.
     */
    void setGLTextureParameters() const;

    /**
     * Sets `_textureType` based on whether the texture is multisampled, is a cube map, or just an ordinary 2D texture.
     */
    void setTextureTypeFromOptions();

    /**
     * Allocates the texture on the GPU depending on its type (multisampled, cubemap, 2D). Can optionally initialize 2D textures through `data`.
     */
    void allocateGLTexture();

    /**
     * Deletes the texture if it already exists, initializes `_textureType`, generates and sets the OpenGL texture parameters, then allocates space on the GPU for the texture.
     * \sa setTextureTypeFromOptions(), setGLTextureParameters(), and allocateGLTexture().
     */
    void initOpenGLTexture();

    /**
     * Given an `ImageBufferSpec`, updates its channels and/or height depending on the spec's format. Supports `RGB_DXT1`, `RGBA_DXT5`, and `YCoCg_DXT5` formats.
     * \param spec The spec we wish to update.
     * \return Whether the format is a compressed one, can potentially indicate that `spec` was updated.
     */
    bool updateCompressedSpec(ImageBufferSpec& spec) const;

    /**
     * \return Given a spec (more specifically, its format), returns a pair of `{internalFormat, dataFormat}` if the spec is supported. Otherwise, returns an empty optional.
     * Supports `RGB_DXT1`, `RGBA_DXT5`, and `YCoCg_DXT5` formats. Note that `dataFormat` is always `GL_UNSIGNED_BYTE`.
     */
    std::optional<std::pair<GLenum, GLenum>> updateCompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) const;

    /**
     * \return Given a spec (more specifically, its number of channels and type), returns a pair of `{internalFormat, dataFormat}` if the spec is supported. Otherwise, returns an
     * empty optional. Can update `_texFormat` depending on the number of channels and type for OpenGL ES.
     * TODO: Check if updating `_texFormat` in OpenGL ES is still necessary.
     */
    virtual std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) = 0;

    /**
     * Wrapper to dispach either `updateCompressedInternalAndDataFormat` or `updateUncompressedInternalAndDataFormat` depending on `isCompressed`.
     * \return a pair of `{internalFormat, dataFormat}` if the spec is supported, an empty optional otherwise.
     */
    std::optional<std::pair<GLenum, GLenum>> updateInternalAndDataFormat(bool isCompressed, const ImageBufferSpec& spec, std::shared_ptr<Image> img);

    /**
     * Re-allocates the texture and sets values for vertical/horizontal wrapping, and minification and magnification filters.
     */
    void updateGLTextureParameters(bool isCompressed);

    /**
     * Reallocates space on the GPU for the texture and initializes its contents with the given image's.
     */
    void reallocateAndInitGLTexture(
        bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize) const;

    /**
     * Updates PBO sizes if needed, reads from the given image into one, and copies the data into the second.
     * \return true if updating the PBOs went well, false otherwise.
     * \sa updatePBOs(), readFromImageIntoPBO(), copyPixelsBetweenPBOs().
     */
    bool updatePBOs(const ImageBufferSpec& spec, int imageDataSize, std::shared_ptr<Image> img);

    /**
     * Updates the specified PBO with the data of the given image.
     */
    virtual void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const = 0;

    /**
     * Reads the data from the given image and uploads it to the GPU through PBOs, reallocating space if needed.
     * \sa updateTextureFromImage().
     */
    void updateTextureFromImage(
        bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize);

    /**
     * Copies the data between `_pbos[0]` and `_pbos[1]`.
     */
    virtual void copyPixelsBetweenPBOs(int imageDataSize) const = 0;

    /**
     * Clears and updates the following uniforms: `flip`, `flop`, and `encoding`.
     */
    void updateShaderUniforms(const ImageBufferSpec& spec, const std::shared_ptr<Image> img);

  protected:
    enum ColorEncoding : int32_t
    {
        RGB = 0,
        BGR = 1,
        UYVY = 2,
        YUYV = 3,
        YCoCg = 4
    };

    GLuint _glTex{0};
    GLuint _pbos[2];
    // Can either be GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP
    GLuint _textureType{GL_TEXTURE_2D};

    int _multisample{0};
    bool _cubemap{false};
    int _pboUploadIndex{0};
    int64_t _lastDrawnTimestamp{0};

    // Store some texture parameters
    static const int _texLevels = 4;
    bool _filtering{false};
    GLenum _texFormat{GL_RGB}, _texType{GL_UNSIGNED_BYTE};
    std::string _pixelFormat{"RGBA"};
    GLint _texInternalFormat{GL_RGBA};
    GLint _glTextureWrap{GL_REPEAT};

    // And some temporary attributes
    GLint _activeTexture{0}; // Texture unit to which the texture is bound

    std::weak_ptr<Image> _img;

    // Parameters to send to the shader
    std::unordered_map<std::string, Values> _shaderUniforms;

    /**
     * Initialization
     */
    void init();

    /**
     * Get GL channel order according to spec.format
     * \param spec Specification
     * \return Return the GL channel order (GL_RGBA, GL_BGRA, ...)
     */
    GLenum getChannelOrder(const ImageBufferSpec& spec);

    /**
     * Update the pbos according to the parameters
     * \param width Width
     * \param height Height
     * \param bytes Bytes per pixel
     * \return Return true if all went well
     */
    virtual bool reallocatePBOs(int width, int height, int bytes) = 0;

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_TEXTURE_H
