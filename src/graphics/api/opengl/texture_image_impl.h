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

/*
 * @opengl_texture_image.h
 * Contains OpenGL 4.5 specific functionality for textures.
 */

#ifndef SPLASH_GFX_OPENGL_TEXTURE_IMAGE_H
#define SPLASH_GFX_OPENGL_TEXTURE_IMAGE_H

#include "./graphics/api/texture_image_impl.h"

namespace Splash::gfx::opengl
{

class Texture_ImageImpl final : public Splash::gfx::Texture_ImageImpl
{
  private:
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

  public:
    /**
     * Constructor
     */
    Texture_ImageImpl() = default;

    /**
     * Destructor
     */
    virtual ~Texture_ImageImpl() override
    {
        glDeleteTextures(1, &_glTex);
        glDeleteBuffers(2, _pbos);
    }

    Texture_ImageImpl(const Texture_ImageImpl&) = delete;
    Texture_ImageImpl& operator=(const Texture_ImageImpl&) = delete;
    Texture_ImageImpl(Texture_ImageImpl&&) = delete;
    Texture_ImageImpl& operator=(Texture_ImageImpl&&) = delete;

    virtual std::unordered_map<std::string, InitTuple> getPixelFormatToInitTable() const final;

    /**
     * Bind this texture
     */
    virtual void bind() final;

    /**
     * Unbind this texture
     */
    virtual void unbind() final;

    /**
     * Generate the mipmaps for the texture
     */
    virtual void generateMipmap() const final;

    /**
     * Get the id of the texture (API dependent)
     * \return Return the texture id
     */
    virtual uint getTexId() const override { return _glTex; }

    /**
     * Enable / disable clamp to edge
     * \param active If true, enables clamping
     */
    virtual void setClampToEdge(bool active) override { _glTextureWrap = active ? GL_CLAMP_TO_EDGE : GL_REPEAT; }

    /**
     * Read the texture and returns an Image
     * \param level The mipmap level we wish to read the texture at.
     * \return Return the image
     */
    std::shared_ptr<Image> read(RootObject* root, int mipmapLevel, ImageBufferSpec spec) const override;

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
    ImageBufferSpec reset(int width, int height, std::string pixelFormat, ImageBufferSpec spec, GLint multisample, bool cubemap, bool filtering) override;

    /**
     * \return Whether or not the given spec is for a compressed texture (API dependent)
     */
    bool isCompressed(const ImageBufferSpec& spec) const override;

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
    std::pair<bool, std::optional<ImageBufferSpec>> update(std::shared_ptr<Image> img, ImageBufferSpec imgSpec, const ImageBufferSpec& textureSpec, bool filtering) override;

  private:
    GLubyte* _pbosPixels[2];
    int _pboUploadIndex{0};

    GLuint _glTex{0};
    GLuint _pbos[2];
    GLuint _textureType{GL_TEXTURE_2D};
    GLint _texInternalFormat{GL_RGBA};
    GLint _glTextureWrap{GL_REPEAT};
    GLenum _texFormat{GL_RGB};
    GLenum _texType{GL_UNSIGNED_BYTE};

    // And some temporary attributes
    GLint _activeTexture{0}; // Texture unit to which the texture is bound

    /**
     * Reads an  OpenGL texture into a pre-allocated buffer on the CPU. Deviates from the OpenGL API by taking both the texture ID and texture type. This is needed to accomodate
     * OpenGL ES. Should almost never be used on its own due to its unsafety. Should be wrapped to return some RAII container to avoid leaking memory or accessing invalid memory.
     *
     * \sa read() For a safer and higher level method.
     */
    virtual void getTextureImage(GLuint textureId, GLenum /*textureType*/, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) const final;

    /**
     * Wrapper for OpenGL's `getTextureLevelParameteriv` and `getTexLevelParameteriv`.
     */
    virtual void getTextureLevelParameteriv(GLenum /*target*/, GLint level, GLenum pname, GLint* params) const final;

    /**
     * Wrapper for OpenGL's `getTextureParameteriv` and `getTexParameteriv`
     */
    virtual void getTextureParameteriv(GLenum /*target*/, GLenum pname, GLint* params) const final;

    /**
     * Update the pbos according to the parameters
     * \param width Width
     * \param height Height
     * \param bytes Bytes per pixel
     * \return Return true if all went well
     */
    virtual bool reallocatePBOs(int width, int height, int bytes) final;

    /**
     * \return Given a spec (more specifically, its number of channels and type), returns a pair of `{internalFormat, dataFormat}` if the spec is supported. Otherwise, returns an
     * empty optional. Can update `_texFormat` depending on the number of channels and type for OpenGL ES.
     */
    virtual std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb);

    /**
     * Updates the specified PBO with the data of the given image.
     * \param pboId ID of the PBO to read into
     * \param int imageDataSize Size of the data to read
     * \param img Image to read from
     */
    virtual void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const;

    /**
     * Copies the data between `_pbos[0]` and `_pbos[1]`.
     * \param imageDataSize Size of the data to copy
     */
    virtual void copyPixelsBetweenPBOs(int imageDataSize) const final;

    /**
     * Deletes the texture if it already exists, initializes `_textureType`, generates and sets the OpenGL texture parameters, then allocates space on the GPU for the texture.
     * \sa setTextureTypeFromOptions(), setGLTextureParameters(), and allocateGLTexture().
     */
    void initOpenGLTexture(GLint multisample, bool cubemap, const ImageBufferSpec& spec, bool filtering);

    /**
     * Sets up texture wrapping, minification and magnification filters, anistropy, and packing/unpacking alignment. Note that all of the OpenGL calls used in this function are
     * supported by OpenGL 4.5 and OpenGL ES, so no need to virtualize them.
     */
    void setGLTextureParameters(bool filtering) const;

    /**
     * Allocates the texture on the GPU depending on its type (multisampled, cubemap, 2D). Can optionally initialize 2D textures through `data`.
     */
    void allocateGLTexture(GLint multisample, const ImageBufferSpec& spec);

    /**
     * Sets `_textureType` based on whether the texture is multisampled, is a cube map, or just an ordinary 2D texture.
     */
    void setTextureTypeFromOptions(GLint multisample, bool cubemap);

    /**
     * Given an `ImageBufferSpec`, updates its channels and/or height depending on the spec's format. Supports `RGB_DXT1`, `RGBA_DXT5`, and `YCoCg_DXT5` formats.
     * \param spec The spec we wish to update.
     * \return Whether the format is a compressed one, can potentially indicate that `spec` was updated.
     */
    void updateCompressedSpec(ImageBufferSpec& spec) const;

    /**
     * Wrapper to dispach either `updateCompressedInternalAndDataFormat` or `updateUncompressedInternalAndDataFormat` depending on `isCompressed`.
     * \return a pair of `{internalFormat, dataFormat}` if the spec is supported, an empty optional otherwise.
     */
    std::optional<std::pair<GLenum, GLenum>> updateInternalAndDataFormat(bool isCompressed, const ImageBufferSpec& spec, std::shared_ptr<Image> img);

    /**
     * Reads the data from the given image and uploads it to the GPU through PBOs, reallocating space if needed.
     */
    void updateTextureFromImage(
        bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize);

    /**
     * Get GL channel order according to spec.format
     * \param spec Specification
     * \return Return the GL channel order (GL_RGBA, GL_BGRA, ...)
     */
    GLenum getChannelOrder(const ImageBufferSpec& spec);

    /**
     * Re-allocates the texture and sets values for vertical/horizontal wrapping, and minification and magnification filters.
     */
    void updateGLTextureParameters(bool isCompressed, bool filtering);

    /**
     * Reallocates space on the GPU for the texture and initializes its contents with the given image's.
     */
    void reallocateAndInitGLTexture(
        bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize) const;

    /**
     * Updates PBO sizes if needed, reads from the given image into one, and copies the data into the second.
     * \return true if updating the PBOs went well, false otherwise.
     * \sa reallocatedPBOs(), readFromImageIntoPBO(), copyPixelsBetweenPBOs().
     */
    bool updatePBOs(const ImageBufferSpec& spec, int imageDataSize, std::shared_ptr<Image> img);

    /**
     * Uses `_pixelFormat` to initialize `_spec`, `_texInternalFormat`, `_texFormat`, and `_texType`. Different graphics APIs might have different initializations due to supporting
     * different combinations of the format, internal format, and type.
     * \param width Width of the texture
     * \param height Height of the texture
     * \sa getPixelFormatToInitTable()
     */
    ImageBufferSpec initFromPixelFormat(int width, int height, std::string pixelFormat, ImageBufferSpec spec);

    /**
     * \return Given a spec (more specifically, its format), returns a pair of `{internalFormat, dataFormat}` if the spec is supported. Otherwise, returns an empty optional.
     * Supports `RGB_DXT1`, `RGBA_DXT5`, and `YCoCg_DXT5` formats. Note that `dataFormat` is always `GL_UNSIGNED_BYTE`.
     */
    std::optional<std::pair<GLenum, GLenum>> updateCompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) const;
};

} // namespace Splash::gfx::opengl

#endif
