/*
 * Copyright (C) 2013 Emmanuel Durand
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
 * @texture.h
 * The Texture_Image base class
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
    void reset(int width, int height, const std::string& pixelFormat, const GLvoid* data, int multisample = 0, bool cubemap = false);

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
    ~Texture_Image();

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

    void initFromPixelFormat(int width, int height);
    
    struct InitTuple;
    virtual std::unordered_map<std::string, InitTuple> getPixelFormatToInitTable() const = 0;

    virtual void getTextureLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) const = 0;
    virtual void getTextureParameteriv(GLenum target, GLenum pname, GLint* params) const = 0;

    // Assumes that `pixels` points to a buffer with enough space to house all the texture's data.
    // Also, deviates from the OpenGL API by taking both the texture ID and texture type. This is needed to accomodate OpenGL ES.
    virtual void getTextureImage(GLuint textureId, GLenum textureType, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) const = 0; 

    // glBindTexture, glTexParameteri, and glPixelStorei are all supported by both OpenGL 4.5 and OpenGL ES 3.2, so no need to virtualize any of them.
    virtual void setGLTextureParameters() const;

    void setTextureTypeFromOptions();

    void allocateGLTexture(const GLvoid* data);

    void initOpenGLTexture(const GLvoid* data);

    bool updateCompressedSpec(ImageBufferSpec& spec) const;
    
    virtual std::optional<std::pair<GLenum, GLenum>> updateCompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) const;

    virtual std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) = 0;

    std::optional<std::pair<GLenum, GLenum>> updateInternalAndDataFormat(bool isCompressed, const ImageBufferSpec& spec, std::shared_ptr<Image> img);

    void updateGLTextureParameters(bool isCompressed);

    void reallocateAndInitGLTexture(bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize) const;

    bool swapPBOs(const ImageBufferSpec& spec, int imageDataSize, std::shared_ptr<Image> img);

    virtual void readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const = 0;

    void updateTextureFromPBO(bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize);

    virtual void copyPixelsBetweenPBOs(int imageDataSize) const = 0;

    void updateShaderUniforms(const ImageBufferSpec& spec, const std::shared_ptr<Image> img);

  protected:
    enum ColorEncoding : int32_t
    {
        RGB=0,
        BGR=1,
        UYVY=2,
        YUYV=3,
        YCoCg=4
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
    virtual bool updatePbos(int width, int height, int bytes) = 0;

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();

    struct InitTuple {
	uint numChannels, bitsPerChannel;
	ImageBufferSpec::Type pixelBitFormat;
	std::string stringName;
	GLenum texInternalFormat, texFormat, texType;
    };
};

} // namespace Splash

#endif // SPLASH_TEXTURE_H
