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

#ifndef SPLASH_GFX_GL_BASE_TEXTURE_IMAGE_IMPL_H
#define SPLASH_GFX_GL_BASE_TEXTURE_IMAGE_IMPL_H

#include "graphics/api/texture_image_impl.h"

namespace Splash::gfx
{

class GlBaseTexture_ImageImpl : public Texture_ImageImpl
{
  public:
    virtual ~GlBaseTexture_ImageImpl()
    {
        glDeleteTextures(1, &_glTex);
        glDeleteBuffers(2, _pbos);
    }

    // Should be implemented by OpenGL or GLES implementations
    // Listed out here for explicitness, can be removed as they're declared in `Texture_ImageImpl`.
    virtual void bind() override = 0;
    virtual void unbind() override = 0;
    virtual void generateMipmap() const override = 0;

    virtual uint getTexId() const override { return _glTex; }

    virtual void setClampToEdge(bool active) override { _glTextureWrap = active ? GL_CLAMP_TO_EDGE : GL_REPEAT; }

    std::shared_ptr<Image> read(RootObject* root, int mipmapLevel, ImageBufferSpec spec) const override
    {
        // Make sure read the width and height of the current mipmap level to avoid allocating
        // extra unneeded memory.
        GLint width = 0, height = 0;
        glBindTexture(_textureType, _glTex);
        getTextureLevelParameteriv(_textureType, mipmapLevel, GL_TEXTURE_WIDTH, &width);
        getTextureLevelParameteriv(_textureType, mipmapLevel, GL_TEXTURE_HEIGHT, &height);

        spec.width = width;
        spec.height = height;

        auto img = std::make_shared<Image>(root, spec);
        getTextureImage(_glTex, _textureType, mipmapLevel, _texFormat, _texType, img->getSpec().rawSize(), (GLvoid*)img->data());
        return img;
    }

    ImageBufferSpec reset(int width, int height, std::string pixelFormat, ImageBufferSpec spec, GLint multisample, bool cubemap, bool filtering) override
    {
        auto newSpec = initFromPixelFormat(width, height, pixelFormat, spec);
        initOpenGLTexture(multisample, cubemap, newSpec, filtering);

        return newSpec;
    }

    bool isCompressed(const ImageBufferSpec& spec) const override { return spec.format == "RGB_DXT1" || spec.format == "RGBA_DXT5" || spec.format == "YCoCg_DXT5"; }

    // TODO: Perhaps tidy up the control flow a bit?
    std::pair<bool, std::optional<ImageBufferSpec>> update(std::shared_ptr<Image> img, ImageBufferSpec imgSpec, const ImageBufferSpec& textureSpec, bool filtering) override
    {
        // Must be called before `updateCompressedSpec`, as it will change the height, which is involved in `rawSize`.
        // If you call `rawSize` after `updateCompressedSpec`, you might get an incorrect `imageDataSize` (depends on
        // the format, check the function for which formats update the height), causing you to read off the buffer in
        // `swapPBOs` and segfault.
        const int imageDataSize = imgSpec.rawSize();

        const bool isCompressed = this->isCompressed(imgSpec);
        if (isCompressed)
            updateCompressedSpec(imgSpec);

        const auto internalAndDataFormat = updateInternalAndDataFormat(isCompressed, imgSpec, img);

        if (!internalAndDataFormat)
            return {false, std::nullopt};

        const auto [internalFormat, dataFormat] = internalAndDataFormat.value();

        const GLenum glChannelOrder = getChannelOrder(imgSpec);
        // Update the textures if the format changed
        if (imgSpec != textureSpec || !imgSpec.videoFrame)
        {
            updateGLTextureParameters(isCompressed, filtering);
            reallocateAndInitGLTexture(isCompressed, internalFormat, imgSpec, glChannelOrder, dataFormat, img, imageDataSize);

            auto pbosUpdatedSuccesfully = updatePBOs(imgSpec, imageDataSize, img);
            if (!pbosUpdatedSuccesfully)
                return {false, std::nullopt};
            else
                return {true, imgSpec}; // Should be updated be previus calls, mainly `updateCompressedSpec`
        }
        // Update the content of the texture, i.e the image
        else
        {
            updateTextureFromImage(isCompressed, internalFormat, imgSpec, glChannelOrder, dataFormat, img, imageDataSize);
            return {true, std::nullopt};
        }
    }

  protected:
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
     * Wrapper for OpenGL's `getTextureLevelParameteriv` and `getTexLevelParameteriv`.
     */
    virtual void getTextureLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) const = 0;

    /**
     * Wrapper for OpenGL's `getTextureParameteriv` and `getTexParameteriv`
     */
    virtual void getTextureParameteriv(GLenum target, GLenum pname, GLint* params) const = 0;

    /**
     * Reads an  OpenGL texture into a pre-allocated buffer on the CPU. Deviates from the OpenGL API by taking both the texture ID and texture type. This is needed to accomodate
     * OpenGL ES. Should almost never be used on its own due to its unsafety. Should be wrapped to return some RAII container to avoid leaking memory or accessing invalid memory.
     *
     * \sa read() For a safer and higher level method.
     */
    virtual void getTextureImage(GLuint textureId, GLenum textureType, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) const = 0;

    /**
     * \return Given a spec (more specifically, its number of channels and type), returns a pair of `{internalFormat, dataFormat}` if the spec is supported. Otherwise, returns an
     * empty optional. Can update `_texFormat` depending on the number of channels and type for OpenGL ES.
     * TODO: Check if updating `_texFormat` in OpenGL ES is still necessary.
     */
    virtual std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) = 0;

    /**
     * Updates the specified PBO with the data of the given image.
     */
    virtual void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const = 0;

    /**
     * Copies the data between `_pbos[0]` and `_pbos[1]`.
     */
    virtual void copyPixelsBetweenPBOs(int imageDataSize) const = 0;

    /**
     * Update the pbos according to the parameters
     * \param width Width
     * \param height Height
     * \param bytes Bytes per pixel
     * \return Return true if all went well
     */
    virtual bool reallocatePBOs(int width, int height, int bytes) = 0;

  private:
    /**
     * Deletes the texture if it already exists, initializes `_textureType`, generates and sets the OpenGL texture parameters, then allocates space on the GPU for the texture.
     * \sa setTextureTypeFromOptions(), setGLTextureParameters(), and allocateGLTexture().
     */
    void initOpenGLTexture(GLint multisample, bool cubemap, const ImageBufferSpec& spec, bool filtering)
    {
        // Create and initialize the texture
        if (glIsTexture(_glTex))
            glDeleteTextures(1, &_glTex);

        setTextureTypeFromOptions(multisample, cubemap);

        glGenTextures(1, &_glTex);

        setGLTextureParameters(filtering);

        allocateGLTexture(multisample, spec);
    }

    /**
     * Sets up texture wrapping, minification and magnification filters, anistropy, and packing/unpacking alignment. Note that all of the OpenGL calls used in this function are
     * supported by OpenGL 4.5 and OpenGL ES, so no need to virtualize them.
     */
    void setGLTextureParameters(bool filtering) const
    {
        glBindTexture(_textureType, _glTex);

        if (_texInternalFormat == GL_DEPTH_COMPONENT)
        {
            glTexParameteri(_textureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(_textureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(_textureType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(_textureType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else
        {
            glTexParameteri(_textureType, GL_TEXTURE_WRAP_S, _glTextureWrap);
            glTexParameteri(_textureType, GL_TEXTURE_WRAP_T, _glTextureWrap);

            // Anisotropic filtering. Not in core, but available on any GPU capable of running Splash
            // See https://www.khronos.org/opengl/wiki/Sampler_Object#Anisotropic_filtering
            float maxAnisoFiltering;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisoFiltering);
            glTexParameterf(_textureType, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisoFiltering);

            if (filtering)
            {
                glTexParameteri(_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(_textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glTexParameteri(_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(_textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
        }
    }

    /**
     * Allocates the texture on the GPU depending on its type (multisampled, cubemap, 2D). Can optionally initialize 2D textures through `data`.
     */
    void allocateGLTexture(GLint multisample, const ImageBufferSpec& spec)
    {
        switch (_textureType)
        {
        case GL_TEXTURE_2D_MULTISAMPLE:
            glTexStorage2DMultisample(_textureType, multisample, _texInternalFormat, spec.width, spec.height, false);
            break;

        case GL_TEXTURE_CUBE_MAP:
            glTexStorage2D(GL_TEXTURE_CUBE_MAP, _texLevels, _texInternalFormat, spec.width, spec.height);
            break;

        default:
            glTexStorage2D(GL_TEXTURE_2D, _texLevels, _texInternalFormat, spec.width, spec.height);
            break;
        }
    }

    /**
     * Sets `_textureType` based on whether the texture is multisampled, is a cube map, or just an ordinary 2D texture.
     */
    void setTextureTypeFromOptions(GLint multisample, bool cubemap)
    {
        if (multisample > 1)
            _textureType = GL_TEXTURE_2D_MULTISAMPLE;
        else if (cubemap)
            _textureType = GL_TEXTURE_CUBE_MAP;
        else
            _textureType = GL_TEXTURE_2D;
    }

    /**
     * Given an `ImageBufferSpec`, updates its channels and/or height depending on the spec's format. Supports `RGB_DXT1`, `RGBA_DXT5`, and `YCoCg_DXT5` formats.
     * \param spec The spec we wish to update.
     * \return Whether the format is a compressed one, can potentially indicate that `spec` was updated.
     */
    void updateCompressedSpec(ImageBufferSpec& spec) const
    {
        if (spec.format == "RGB_DXT1")
        {
            spec.height *= 2;
            spec.channels = 3;
        }
        else if (spec.format == "RGBA_DXT5")
        {
            spec.channels = 4;
        }
    }

    /**
     * Wrapper to dispach either `updateCompressedInternalAndDataFormat` or `updateUncompressedInternalAndDataFormat` depending on `isCompressed`.
     * \return a pair of `{internalFormat, dataFormat}` if the spec is supported, an empty optional otherwise.
     */
    std::optional<std::pair<GLenum, GLenum>> updateInternalAndDataFormat(bool isCompressed, const ImageBufferSpec& spec, std::shared_ptr<Image> img)
    {
        Values srgb;
        img->getAttribute("srgb", srgb);

        if (isCompressed)
            return updateCompressedInternalAndDataFormat(spec, srgb);
        else
            return updateUncompressedInternalAndDataFormat(spec, srgb);
    }

    /**
     * Reads the data from the given image and uploads it to the GPU through PBOs, reallocating space if needed.
     * \sa updateTextureFromImage().
     */
    void updateTextureFromImage(
        bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[_pboUploadIndex]);
        glBindTexture(_textureType, _glTex);

        // (Re-)allocate space for the texture
        if (!isCompressed)
            glTexSubImage2D(_textureType, 0, 0, 0, spec.width, spec.height, glChannelOrder, dataFormat, 0);
        else
            glCompressedTexSubImage2D(_textureType, 0, 0, 0, spec.width, spec.height, internalFormat, imageDataSize, 0);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        _pboUploadIndex = (_pboUploadIndex + 1) % 2;

        // Fill the next PBO with the image pixels
        readFromImageIntoPBO(_pbos[_pboUploadIndex], imageDataSize, img);
    }

    /**
     * Get GL channel order according to spec.format
     * \param spec Specification
     * \return Return the GL channel order (GL_RGBA, GL_BGRA, ...)
     */
    GLenum getChannelOrder(const ImageBufferSpec& spec)
    {
        GLenum glChannelOrder = GL_RGB;

        // We don't want to let the driver convert from BGR to RGB, as this can lead in
        // some cases to mediocre performances.
        if (spec.format == "RGB" || spec.format == "BGR" || spec.format == "RGB_DXT1")
            glChannelOrder = GL_RGB;
        else if (spec.format == "RGBA" || spec.format == "BGRA" || spec.format == "RGBA_DXT5")
            glChannelOrder = GL_RGBA;
        else if (spec.format == "YUYV" || spec.format == "UYVY")
            glChannelOrder = GL_RG;
        else if (spec.channels == 1)
            glChannelOrder = GL_RED;
        else if (spec.channels == 3)
            glChannelOrder = GL_RGB;
        else if (spec.channels == 4)
            glChannelOrder = GL_RGBA;

        return glChannelOrder;
    }

    /**
     * Re-allocates the texture and sets values for vertical/horizontal wrapping, and minification and magnification filters.
     */
    void updateGLTextureParameters(bool isCompressed, bool filtering)
    {
        // glTexStorage2D is immutable, so we have to delete the texture first
        glDeleteTextures(1, &_glTex);
        glGenTextures(1, &_glTex);
        glBindTexture(_textureType, _glTex);

        glTexParameteri(_textureType, GL_TEXTURE_WRAP_S, _glTextureWrap);
        glTexParameteri(_textureType, GL_TEXTURE_WRAP_T, _glTextureWrap);

        if (filtering)
        {
            if (isCompressed)
                glTexParameteri(_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            else
                glTexParameteri(_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(_textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            glTexParameteri(_textureType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(_textureType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
    }

    /**
     * Reallocates space on the GPU for the texture and initializes its contents with the given image's.
     */
    void reallocateAndInitGLTexture(
        bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize) const
    {
        // Create or update the texture parameters
        if (!isCompressed)
        {
#ifdef DEBUG
            Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Creating a new texture" << Log::endl;
#endif

            glTexStorage2D(_textureType, _texLevels, internalFormat, spec.width, spec.height);
            glTexSubImage2D(_textureType, 0, 0, 0, spec.width, spec.height, glChannelOrder, dataFormat, img->data());
        }
        else
        {
#ifdef DEBUG
            Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Creating a new compressed texture" << Log::endl;
#endif

            glTexStorage2D(_textureType, _texLevels, internalFormat, spec.width, spec.height);
            glCompressedTexSubImage2D(_textureType, 0, 0, 0, spec.width, spec.height, internalFormat, imageDataSize, img->data());
        }
    }

    /**
     * Updates PBO sizes if needed, reads from the given image into one, and copies the data into the second.
     * \return true if updating the PBOs went well, false otherwise.
     * \sa reallocatedPBOs(), readFromImageIntoPBO(), copyPixelsBetweenPBOs().
     */
    bool updatePBOs(const ImageBufferSpec& spec, int imageDataSize, std::shared_ptr<Image> img)
    {
        if (!reallocatePBOs(spec.width, spec.height, spec.pixelBytes()))
            return false;

        // Fill one of the PBOs right now
        readFromImageIntoPBO(_pbos[0], imageDataSize, img);

        // And copy it to the second PBO
        copyPixelsBetweenPBOs(imageDataSize);

        return true;
    }

    /**
     * Uses `_pixelFormat` to initialize `_spec`, `_texInternalFormat`, `_texFormat`, and `_texType`. Different graphics APIs might have different initializations due to supporting
     * different combinations of the format, internal format, and type.
     * \param width Width of the texture
     * \param height Height of the texture
     * \sa getPixelFormatToInitTable()
     */
    ImageBufferSpec initFromPixelFormat(int width, int height, std::string pixelFormat, ImageBufferSpec spec)
    {
        const auto pixelFormatToInit = getPixelFormatToInitTable();
        const bool containsPixelFormat = pixelFormatToInit.find(pixelFormat) != pixelFormatToInit.end();
        if (containsPixelFormat)
        {
            const auto initTuple = pixelFormatToInit.at(pixelFormat);
            _texInternalFormat = initTuple.texInternalFormat;
            _texFormat = initTuple.texFormat;
            _texType = initTuple.texType;

            spec = ImageBufferSpec(width, height, initTuple.numChannels, initTuple.bitsPerChannel, initTuple.pixelBitFormat, initTuple.stringName);
        }
        else
        {
            spec.width = width;
            spec.height = height;

            Log::get() << Log::WARNING << "Texture_Image::" << __FUNCTION__ << " - The given pixel format (" << pixelFormat
                       << ") does not match any of the supported types. Will use default values." << Log::endl;
        }

        return spec;
    }

    /**
     * \return Given a spec (more specifically, its format), returns a pair of `{internalFormat, dataFormat}` if the spec is supported. Otherwise, returns an empty optional.
     * Supports `RGB_DXT1`, `RGBA_DXT5`, and `YCoCg_DXT5` formats. Note that `dataFormat` is always `GL_UNSIGNED_BYTE`.
     */
    std::optional<std::pair<GLenum, GLenum>> updateCompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) const
    {
        GLenum internalFormat;
        // Doesn't actually change the data format, not sure if it should be kept for uniformity with its compressed counterpart, or removed.
        GLenum dataFormat = GL_UNSIGNED_BYTE;

        if (spec.format == "RGB_DXT1")
        {
            if (srgb[0].as<bool>())
                internalFormat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
            else
                internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        }
        else if (spec.format == "RGBA_DXT5")
        {
            if (srgb[0].as<bool>())
                internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
            else
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        }
        else if (spec.format == "YCoCg_DXT5")
        {
            internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        }
        else
        {
            Log::get() << Log::WARNING << "Texture_Image::" << __FUNCTION__ << " - Unknown compressed format" << Log::endl;
            return {};
        }

        return {{internalFormat, dataFormat}};
    }

  protected:
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
};

}; // namespace Splash::gfx

#endif
