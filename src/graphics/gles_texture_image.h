#ifndef SPLASH_GLES_TEXTURE_IMAGE_H
#define SPLASH_GLES_TEXTURE_IMAGE_H

#include "graphics/texture_image.h"

namespace Splash {
    class GLESTexture_Image final: public Texture_Image 
    {
	public:
	    explicit GLESTexture_Image(RootObject* root): Texture_Image(root) 
	{ }

	    GLESTexture_Image(const GLESTexture_Image&) = delete;
	    GLESTexture_Image& operator=(const GLESTexture_Image&) = delete;
	    GLESTexture_Image(GLESTexture_Image&&) = delete;
	    GLESTexture_Image& operator=(GLESTexture_Image&&) = delete;

	    // Lists the supported combinations of internal formats, formats, and texture types: https://docs.gl/es3/glTexStorage2D
	    virtual void initFromPixelFormat(int width, int height) 
	    {

		// OpenGL ES doesn't support 16 bpc (bit per channel) RGBA textures, so we treat them as 8 bpc
		if (_pixelFormat == "RGBA" || _pixelFormat == "RGBA16")
		{
		    _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
		    _texInternalFormat = GL_RGBA8;
		    _texFormat = GL_RGBA;

		    // OpenGL 4 vs ES 3.1: GL_UNSIGNED_INT_8_8_8_8_REV seems to be unavailable
		    // The docs say to use GL_RGBA8, GL_RGBA, and GL_UNSIGNED_BYTE for the internal format,
		    // texture format, and type respectively.
		    _texType = GL_UNSIGNED_BYTE;
		}
		else if (_pixelFormat == "sRGBA")
		{
		    _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
		    _texInternalFormat = GL_SRGB8_ALPHA8;
		    _texFormat = GL_RGBA;
		    _texType = GL_UNSIGNED_BYTE;
		}
		else if (_pixelFormat == "RGB")
		{
		    _spec = ImageBufferSpec(width, height, 3, 24, ImageBufferSpec::Type::UINT8, "RGB");
		    _texInternalFormat = GL_RGBA8;
		    _texFormat = GL_RGB;
		    _texType = GL_UNSIGNED_BYTE;
		}
		else if (_pixelFormat == "R16")
		{
		    _spec = ImageBufferSpec(width, height, 1, 16, ImageBufferSpec::Type::UINT16, "R");
		    _texInternalFormat = GL_R16;
		    _texFormat = GL_RED;
		    _texType = GL_UNSIGNED_SHORT;
		}
		else if (_pixelFormat == "YUYV" || _pixelFormat == "UYVY")
		{
		    _spec = ImageBufferSpec(width, height, 3, 16, ImageBufferSpec::Type::UINT8, _pixelFormat);
		    _texInternalFormat = GL_RG8;
		    _texFormat = GL_RG;
		    _texType = GL_UNSIGNED_SHORT;
		}
		else if (_pixelFormat == "D")
		{
		    // OpenGL ES supports only GL_DEPTH_COMPONENT32F for float values,
		    // even though we're using 24bit float value, this works fine.
		    _spec = ImageBufferSpec(width, height, 1, 24, ImageBufferSpec::Type::FLOAT, "R");
		    _texInternalFormat = GL_DEPTH_COMPONENT32F;
		    _texFormat = GL_DEPTH_COMPONENT;
		    _texType = GL_FLOAT;
		} else {
		    _spec.width = width;
		    _spec.height = height;

		    Log::get() << Log::WARNING << "GLESTexture_Image::" << __FUNCTION__ << " - The given pixel format (" << _pixelFormat << ") does not match any of the supported types. Will use default values." << Log::endl;
		}
	    }

	    virtual void setGLTextureParameters() const {
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

			if (_filtering)
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

	    virtual void initOpenGLTexture(const GLvoid* data) 
	    {
		    // Create and initialize the texture
		    if (glIsTexture(_glTex))
			glDeleteTextures(1, &_glTex);

		    glGenTextures(1, &_glTex);

		    if (_multisample > 1)
			_textureType = GL_TEXTURE_2D_MULTISAMPLE;
		    else if (_cubemap)
			_textureType = GL_TEXTURE_CUBE_MAP;
		    else
			_textureType = GL_TEXTURE_2D;

		    setGLTextureParameters();

		    if (_multisample > 1)
			glTexStorage2DMultisample(_textureType, _multisample, _texInternalFormat, _spec.width, _spec.height, false);
		    else if (_cubemap == true)
			glTexStorage2D(GL_TEXTURE_CUBE_MAP, _texLevels, _texInternalFormat, _spec.width, _spec.height);
		    else
		    {
			glTexStorage2D(GL_TEXTURE_2D, _texLevels, _texInternalFormat, _spec.width, _spec.height);

			if (data)
			    glTexSubImage2D(_textureType, 0, 0, 0, _spec.width, _spec.height, _texFormat, _texType, data);
		    }
	    }

	    virtual void bind() final
	    {
		glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
		glActiveTexture(_activeTexture);
		glBindTexture(GL_TEXTURE_2D, _glTex);
	    }

	    void unbind() final
	    {
#ifdef DEBUG
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
#endif
		_lastDrawnTimestamp = Timer::getTime();
	    }

	    virtual void generateMipmap() const final
	    {
		glBindTexture(GL_TEXTURE_2D, _glTex);
		glGenerateMipmap(GL_TEXTURE_2D);
	    }

	    virtual void getTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) const final 
	    {
		// Silence "unused parameter" warnings/errors.
		(void)texture;
		(void)level;
		(void)format;
		(void)type;
		(void)bufSize;
		(void)pixels;

		// TODO: Figure out some way to replicate OpenGL 4.5's API call.
		assert(false && "unimplemented yet!");
	    }

	    virtual void getTextureLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) const final {
		glBindTexture(target, _glTex);
		glGetTexLevelParameteriv(target, level, pname, params);
	    }

	    virtual void getTextureParameteriv(GLenum target, GLenum pname, GLint* params) const final
	    {
		glBindTexture(target, _glTex);
		glGetTexParameteriv(target, pname, params);
	    }

	    void update() final
	    {
		// If _img is nullptr, this texture is not set from an Image
		if (_img.expired())
		    return;

		const auto img = _img.lock();

		if (img->getTimestamp() == _spec.timestamp)
		    return;

		if (_multisample > 1)
		{
		    Log::get() << Log::ERROR << "GLESTexture_Image::" << __FUNCTION__ << " - Texture " << _name << " is multisampled, and can not be set from an image" << Log::endl;
		    return;
		}

		img->update();
		const auto imgLock = img->getReadLock();

		// Gets the spec, if the image format is one of the compressed ones, updates some spec values,
		// and returns true to indicate that the image is compressed.
		auto spec = img->getSpec();
		const bool isCompressed = updateCompressedSpec(spec);
		const auto internalAndDataFormat = updateInternalAndDataFormat(isCompressed, spec, img);

		if(!internalAndDataFormat) 
		    return;

		const auto [internalFormat, dataFormat] = internalAndDataFormat.value();

		const GLenum glChannelOrder = getChannelOrder(spec);
		const int imageDataSize = spec.rawSize();
		// Update the textures if the format changed
		if (spec != _spec || !spec.videoFrame)
		{
		    updateGLTextureParameters(isCompressed);
		    reallocateAndInitGLTexture(isCompressed, internalFormat, spec, glChannelOrder, dataFormat,  img, imageDataSize);
		    if(!swapPBOs(spec, imageDataSize, img))
			return;
		}
		// Update the content of the texture, i.e the image
		else
		{
		    updateTextureFromPBO(isCompressed, internalFormat, spec, glChannelOrder, dataFormat,  img, imageDataSize);
		}

		_spec.timestamp = spec.timestamp;

		Values flip, flop;
		img->getAttribute("flip", flip);
		img->getAttribute("flop", flop);
		updateShaderUniforms(spec, flip, flop);

		if (_filtering && !isCompressed)
		    generateMipmap();
	    }

	    virtual bool updatePbos(int width, int height, int bytes) final
	    {
		glDeleteBuffers(2, _pbos);

		// TODO: I think this is the most lenient option, might be worth it to see how others affect things
		constexpr auto bufferUsageFlags = GL_STATIC_DRAW;
		auto imageDataSize = width * height * bytes;

		glGenBuffers(2, _pbos);

		constexpr GLenum bufferType = GL_PIXEL_UNPACK_BUFFER;

		glBindBuffer(bufferType, _pbos[0]);
		glBufferData(bufferType, imageDataSize, 0, bufferUsageFlags);

		glBindBuffer(bufferType, _pbos[1]);
		glBufferData(bufferType, imageDataSize, 0, bufferUsageFlags);

		glBindBuffer(bufferType, 0);

		return true;
	    }

	    virtual void generateMipmap() final {
		glBindTexture(GL_TEXTURE_2D, _glTex);
		glGenerateMipmap(GL_TEXTURE_2D);
	    }

	private:
	    bool updateCompressedSpec(ImageBufferSpec& spec) const {

		// If the texture is compressed, we need to modify a few values
		bool isCompressed = false;

		if (spec.format == "RGB_DXT1")
		{
		    isCompressed = true;
		    spec.height *= 2;
		    spec.channels = 3;
		}
		else if (spec.format == "RGBA_DXT5")
		{
		    isCompressed = true;
		    spec.channels = 4;
		}
		else if (spec.format == "YCoCg_DXT5")
		{
		    isCompressed = true;
		}

		return isCompressed;
	    }

	    std::optional<std::pair<GLenum, GLenum>> updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) 
	    {
		GLenum internalFormat;
		GLenum dataFormat = GL_UNSIGNED_BYTE;

		if (spec.channels == 4 && spec.type == ImageBufferSpec::Type::UINT8)
		{
		    dataFormat = GL_UNSIGNED_BYTE;
		    _texFormat = GL_RGBA; // Not sure if we're supposed to modify data members here..
		    if (srgb[0].as<bool>())
			internalFormat = GL_SRGB8_ALPHA8;
		    else
			internalFormat = GL_RGBA;
		}
		else if (spec.channels == 3 && spec.type == ImageBufferSpec::Type::UINT8)
		{
		    dataFormat = GL_UNSIGNED_BYTE;
		    if (srgb[0].as<bool>())
			internalFormat = GL_SRGB8_ALPHA8;
		    else
			internalFormat = GL_RGBA8;
		}
		else if (spec.channels == 2 && spec.type == ImageBufferSpec::Type::UINT8)
		{
		    _texFormat = GL_RG;
		    dataFormat = GL_UNSIGNED_BYTE;
		    internalFormat = GL_RG8;
		}
		else if (spec.channels == 1 && spec.type == ImageBufferSpec::Type::UINT16)
		{
		    dataFormat = GL_UNSIGNED_SHORT;
		    internalFormat = GL_R16;
		}
		else
		{
		    Log::get() << Log::WARNING << "GLESTexture_Image::" << __FUNCTION__ << " - Unknown uncompressed format" << Log::endl;
		    return {};
		}

		return {{ internalFormat, dataFormat }};
	    }

	    // Doesn't actually change the data format, not sure if it should be kept for uniformity, or removed.
	    std::optional<std::pair<GLenum, GLenum>> updateCompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) const
	    {
		GLenum internalFormat;
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
		    Log::get() << Log::WARNING << "GLESTexture_Image::" << __FUNCTION__ << " - Unknown compressed format" << Log::endl;
		    return {};
		}

		return {{ internalFormat, dataFormat }};
	    }


	    void updateGLTextureParameters(bool isCompressed) 
	    {
		// glTexStorage2D is immutable, so we have to delete the texture first
		glDeleteTextures(1, &_glTex);
		glGenTextures(1, &_glTex);
		glBindTexture(_textureType, _glTex);

		glTexParameteri(_textureType, GL_TEXTURE_WRAP_S, _glTextureWrap);
		glTexParameteri(_textureType, GL_TEXTURE_WRAP_T, _glTextureWrap);

		if (_filtering)
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

	    void reallocateAndInitGLTexture(bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize) const
	    {
		// Create or update the texture parameters
		if (!isCompressed)
		{
#ifdef DEBUG
		    Log::get() << Log::DEBUGGING << "GLESTexture_Image::" << __FUNCTION__ << " - Creating a new texture" << Log::endl;
#endif

		    glTexStorage2D(_textureType, _texLevels, internalFormat, spec.width, spec.height);
		    glTexSubImage2D(_textureType, 0, 0, 0, spec.width, spec.height, glChannelOrder, dataFormat, img->data());
		}
		else
		{
#ifdef DEBUG
		    Log::get() << Log::DEBUGGING << "TGLESexture_Image::" << __FUNCTION__ << " - Creating a new compressed texture" << Log::endl;
#endif

		    glTexStorage2D(_textureType, _texLevels, internalFormat, spec.width, spec.height);
		    glCompressedTexSubImage2D(_textureType, 0, 0, 0, spec.width, spec.height, internalFormat, imageDataSize, img->data());
		}
	    }

	    void readFromPBOIntoImage(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const
	    {
		// Fill one of the PBOs right now
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboId);
		auto pixels = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, imageDataSize, GL_MAP_WRITE_BIT);

		if (pixels != nullptr)
		    memcpy((void*)pixels, img->data(), imageDataSize);

		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	    }

	    bool swapPBOs(const ImageBufferSpec& spec, int imageDataSize, std::shared_ptr<Image> img) 
	    {
		if (!updatePbos(spec.width, spec.height, spec.pixelBytes()))
		    return false;

		// Fill one of the PBOs right now
		readFromPBOIntoImage(_pbos[0], imageDataSize, img);

		// And copy it to the second PBO
		glBindBuffer(GL_COPY_READ_BUFFER, _pbos[0]);
		glBindBuffer(GL_COPY_WRITE_BUFFER, _pbos[1]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, imageDataSize);
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
		_spec = spec;

		return true;
	    }

	    void updateTextureFromPBO(bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize) 
	    {

		// Copy the pixels from the current PBO to the texture
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[_pboUploadIndex]);
		glBindTexture(_textureType, _glTex);

		if (!isCompressed)
		    glTexSubImage2D(_textureType, 0, 0, 0, spec.width, spec.height, glChannelOrder, dataFormat, 0);
		else
		    glCompressedTexSubImage2D(_textureType, 0, 0, 0, spec.width, spec.height, internalFormat, imageDataSize, 0);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		_pboUploadIndex = (_pboUploadIndex + 1) % 2;

		// Fill the next PBO with the image pixels
		readFromPBOIntoImage(_pbos[_pboUploadIndex], imageDataSize, img);
	    }

	    void updateShaderUniforms(const ImageBufferSpec& spec, const Values& flip, const Values& flop)
	    {
		// If needed, specify some uniforms for the shader which will use this texture
		_shaderUniforms.clear();

		// Presentation parameters
		_shaderUniforms["flip"] = flip;
		_shaderUniforms["flop"] = flop;

		// Specify the color encoding
		if (spec.format.find("RGB") != std::string::npos)
		    _shaderUniforms["encoding"] = {ColorEncoding::RGB};
		else if (spec.format.find("BGR") != std::string::npos)
		    _shaderUniforms["encoding"] = {ColorEncoding::BGR};
		else if (spec.format == "UYVY")
		    _shaderUniforms["encoding"] = {ColorEncoding::UYVY};
		else if (spec.format == "YUYV")
		    _shaderUniforms["encoding"] = {ColorEncoding::YUYV};
		else if (spec.format == "YCoCg_DXT5")
		    _shaderUniforms["encoding"] = {ColorEncoding::YCoCg};
		else
		    _shaderUniforms["encoding"] = {ColorEncoding::RGB}; // Default case: RGB
	    }

	    std::optional<std::pair<GLenum, GLenum>> updateInternalAndDataFormat(bool isCompressed, const ImageBufferSpec& spec, std::shared_ptr<Image> img) 
	    {
		// Updates `internalFormat` and `dataFormat` depending on whether the texture is compressed and the spec data.
		Values srgb;
		img->getAttribute("srgb", srgb);

		if(isCompressed)
		    return updateCompressedInternalAndDataFormat(spec, srgb);
		else 
		    return updateUncompressedInternalAndDataFormat(spec, srgb);
	    }

	};
}

#endif
