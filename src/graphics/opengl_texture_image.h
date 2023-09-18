#ifndef SPLASH_OPENGL_TEXTURE_IMAGE_H
#define SPLASH_OPENGL_TEXTURE_IMAGE_H

#include "./graphics/texture_image.h"

namespace Splash 
{
    class OpenGLTexture_Image final: public Texture_Image 
    {
	public:
	    explicit OpenGLTexture_Image(RootObject* root): Texture_Image(root) 
	{ }

	    OpenGLTexture_Image(const OpenGLTexture_Image&) = delete;
	    OpenGLTexture_Image& operator=(const OpenGLTexture_Image&) = delete;
	    OpenGLTexture_Image(OpenGLTexture_Image&&) = delete;
	    OpenGLTexture_Image& operator=(OpenGLTexture_Image&&) = delete;

	    virtual void reset(int width, int height, const std::string& pixelFormat, const GLvoid* data, int multisample = 0, bool cubemap = false) final 
	    {
		if (width == 0 || height == 0)
		{
#ifdef DEBUG
		    Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Texture size is null" << Log::endl;
#endif
		    return;
		}

		// Fill texture parameters
		_spec.width = width;
		_spec.height = height;
		_pixelFormat = pixelFormat.empty() ? "RGBA" : pixelFormat;
		_multisample = multisample;
		_cubemap = multisample == 0 ? cubemap : false;

		if (_pixelFormat == "RGBA")
		{
		    _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
		    _texInternalFormat = GL_RGBA8;
		    _texFormat = GL_RGBA;
		    _texType = GL_UNSIGNED_INT_8_8_8_8_REV;
		}
		else if (_pixelFormat == "sRGBA")
		{
		    _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
		    _texInternalFormat = GL_SRGB8_ALPHA8;
		    _texFormat = GL_RGBA;
		    _texType = GL_UNSIGNED_INT_8_8_8_8_REV;
		}
		else if (_pixelFormat == "RGBA16")
		{
		    _spec = ImageBufferSpec(width, height, 4, 64, ImageBufferSpec::Type::UINT16, "RGBA");
		    _texInternalFormat = GL_RGBA16;
		    _texFormat = GL_RGBA;
		    _texType = GL_UNSIGNED_SHORT;
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
		    _spec = ImageBufferSpec(width, height, 1, 24, ImageBufferSpec::Type::FLOAT, "R");
		    _texInternalFormat = GL_DEPTH_COMPONENT24;
		    _texFormat = GL_DEPTH_COMPONENT;
		    _texType = GL_FLOAT;
		}

		// Create and initialize the texture
		if (glIsTexture(_glTex))
		    glDeleteTextures(1, &_glTex);

		if (_multisample > 1)
		    _textureType = GL_TEXTURE_2D_MULTISAMPLE;
		else if (_cubemap)
		    _textureType = GL_TEXTURE_CUBE_MAP;
		else
		    _textureType = GL_TEXTURE_2D;

		glCreateTextures(_textureType, 1, &_glTex);

		if (_texInternalFormat == GL_DEPTH_COMPONENT)
		{
		    glTextureParameteri(_glTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		    glTextureParameteri(_glTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		    glTextureParameteri(_glTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		    glTextureParameteri(_glTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		else
		{
		    glTextureParameteri(_glTex, GL_TEXTURE_WRAP_S, _glTextureWrap);
		    glTextureParameteri(_glTex, GL_TEXTURE_WRAP_T, _glTextureWrap);

		    // Anisotropic filtering. Not in core, but available on any GPU capable of running Splash
		    // See https://www.khronos.org/opengl/wiki/Sampler_Object#Anisotropic_filtering
		    float maxAnisoFiltering;
		    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisoFiltering);
		    glTextureParameterf(_glTex, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisoFiltering);

		    if (_filtering)
		    {
			glTextureParameteri(_glTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTextureParameteri(_glTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    }
		    else
		    {
			glTextureParameteri(_glTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(_glTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    }

		    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		    glPixelStorei(GL_PACK_ALIGNMENT, 4);
		}

		if (_multisample > 1)
		{
		    glTextureStorage2DMultisample(_glTex, _multisample, _texInternalFormat, width, height, false);
		}
		else if (_cubemap == true)
		{
		    glTextureStorage2D(_glTex, _texLevels, _texInternalFormat, width, height);
		}
		else
		{
		    glTextureStorage2D(_glTex, _texLevels, _texInternalFormat, width, height);
		    if (data)
			glTextureSubImage2D(_glTex, 0, 0, 0, width, height, _texFormat, _texType, data);
		}

#ifdef DEBUG
		Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Reset the texture to size " << width << "x" << height << Log::endl;
#endif
	    }

	    virtual void bind() final
	    {
		glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
		_activeTexture = _activeTexture - GL_TEXTURE0;
		glBindTextureUnit(_activeTexture, _glTex);
	    }

	    void unbind() final
	    {
#ifdef DEBUG
		glBindTextureUnit(_activeTexture, 0);
#endif
		_lastDrawnTimestamp = Timer::getTime();
	    }

	    virtual void generateMipmap() const final
	    {
		glGenerateTextureMipmap(_glTex);
	    }

	    virtual void getTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) const final 
	    {
		glGetTextureImage(texture, level, format, type, bufSize, pixels);
	    }

	    virtual void getTextureLevelParameteriv(GLenum /*target*/, GLint level, GLenum pname, GLint* params) const final {
		glGetTextureLevelParameteriv(_glTex, level, pname, params);
	    }

	    virtual void getTextureParameteriv(GLenum /*target*/, GLenum pname, GLint* params) const final
	    {
		glGetTextureParameteriv(_glTex, pname, params);
	    }


	    virtual void update() final
	    {
		// If _img is nullptr, this texture is not set from an Image
		if (_img.expired())
		    return;
		const auto img = _img.lock();

		if (img->getTimestamp() == _spec.timestamp)
		    return;

		if (_multisample > 1)
		{
		    Log::get() << Log::ERROR << "Texture_Image::" << __FUNCTION__ << " - Texture " << _name << " is multisampled, and can not be set from an image" << Log::endl;
		    return;
		}

		img->update();
		const auto imgLock = img->getReadLock();

		auto spec = img->getSpec();
		Values srgb, flip, flop;
		img->getAttribute("srgb", srgb);
		img->getAttribute("flip", flip);
		img->getAttribute("flop", flop);

		// Store the image data size
		const int imageDataSize = spec.rawSize();
		GLenum glChannelOrder = getChannelOrder(spec);

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

		// Get GL parameters
		GLenum internalFormat;
		GLenum dataFormat = GL_UNSIGNED_BYTE;
		if (!isCompressed)
		{
		    if (spec.channels == 4 && spec.type == ImageBufferSpec::Type::UINT8)
		    {
			dataFormat = GL_UNSIGNED_INT_8_8_8_8_REV;
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
			Log::get() << Log::WARNING << "Texture_Image::" << __FUNCTION__ << " - Unknown uncompressed format" << Log::endl;
			return;
		    }
		}
		else if (isCompressed)
		{
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
			return;
		    }
		}

		// Update the textures if the format changed
		if (spec != _spec || !spec.videoFrame)
		{
		    // glTexStorage2D is immutable, so we have to delete the texture first
		    glDeleteTextures(1, &_glTex);
		    glCreateTextures(_textureType, 1, &_glTex);

		    glTextureParameteri(_glTex, GL_TEXTURE_WRAP_S, _glTextureWrap);
		    glTextureParameteri(_glTex, GL_TEXTURE_WRAP_T, _glTextureWrap);

		    if (_filtering)
		    {
			if (isCompressed)
			    glTextureParameteri(_glTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			else
			    glTextureParameteri(_glTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTextureParameteri(_glTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    }
		    else
		    {
			glTextureParameteri(_glTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(_glTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		    }

		    // Create or update the texture parameters
		    if (!isCompressed)
		    {
#ifdef DEBUG
			Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Creating a new texture" << Log::endl;
#endif
			glTextureStorage2D(_glTex, _texLevels, internalFormat, spec.width, spec.height);
			glTextureSubImage2D(_glTex, 0, 0, 0, spec.width, spec.height, glChannelOrder, dataFormat, img->data());
		    }
		    else if (isCompressed)
		    {
#ifdef DEBUG
			Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Creating a new compressed texture" << Log::endl;
#endif

			glTextureStorage2D(_glTex, _texLevels, internalFormat, spec.width, spec.height);
			glCompressedTextureSubImage2D(_glTex, 0, 0, 0, spec.width, spec.height, internalFormat, imageDataSize, img->data());
		    }

		    if (!updatePbos(spec.width, spec.height, spec.pixelBytes()))
			return;

		    // Fill one of the PBOs right now
		    auto pixels = _pbosPixels[0];
		    if (pixels != nullptr)
		    {
			memcpy((void*)pixels, img->data(), imageDataSize);
		    }

		    // And copy it to the second PBO
		    glCopyNamedBufferSubData(_pbos[0], _pbos[1], 0, 0, imageDataSize);
		    _spec = spec;
		}
		// Update the content of the texture, i.e the image
		else
		{
		    // Copy the pixels from the current PBO to the texture
		    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[_pboUploadIndex]);
		    if (!isCompressed)
			glTextureSubImage2D(_glTex, 0, 0, 0, spec.width, spec.height, glChannelOrder, dataFormat, 0);
		    else
			glCompressedTextureSubImage2D(_glTex, 0, 0, 0, spec.width, spec.height, internalFormat, imageDataSize, 0);
		    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		    _pboUploadIndex = (_pboUploadIndex + 1) % 2;

		    // Fill the next PBO with the image pixels
		    auto pixels = _pbosPixels[_pboUploadIndex];
		    if (pixels != nullptr)
			memcpy(pixels, img->data(), imageDataSize);
		}

		_spec.timestamp = spec.timestamp;

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

		if (_filtering && !isCompressed)
		    generateMipmap();
	    }

	    virtual bool updatePbos(int width, int height, int bytes) final
	    {
		glDeleteBuffers(2, _pbos);

		auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		auto imageDataSize = width * height * bytes;

		glCreateBuffers(2, _pbos);
		glNamedBufferStorage(_pbos[0], imageDataSize, 0, flags);
		glNamedBufferStorage(_pbos[1], imageDataSize, 0, flags);

		_pbosPixels[0] = (GLubyte*)glMapNamedBufferRange(_pbos[0], 0, imageDataSize, flags);
		_pbosPixels[1] = (GLubyte*)glMapNamedBufferRange(_pbos[1], 0, imageDataSize, flags);

		if (!_pbosPixels[0] || !_pbosPixels[1])
		{
		    Log::get() << Log::ERROR << "Texture_Image::" << __FUNCTION__ << " - Unable to initialize upload PBOs" << Log::endl;
		    return false;
		}

		return true;
	    }

	    virtual void generateMipmap() final {
		glGenerateTextureMipmap(_glTex);
	    }

	private:
	    GLubyte* _pbosPixels[2];
    };
}

#endif
