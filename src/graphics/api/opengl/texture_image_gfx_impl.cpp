#include "./graphics/api/opengl/texture_image_gfx_impl.h"

namespace Splash::gfx::opengl
{

/*************/
std::unordered_map<std::string, Texture_ImageGfxImpl::InitTuple> Texture_ImageGfxImpl::getPixelFormatToInitTable() const
{
    // Note from Emmanuel: Just FYI, `GL_UNSIGNED_INT_8_8_8_8_REV` is known to allow for faster memory transfer than mere GL_UNSIGNED_BYTE. Which is why it was used here.
    // This is in contrast to GLESTexture_Image, where `GL_UNSIGNED_INT_8_8_8_8_REV` is unavailable.
    return {
        {"RGBA", {4, 32, ImageBufferSpec::Type::UINT8, "RGBA", GL_RGBA8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV}},
        {"sRGBA", {4, 32, ImageBufferSpec::Type::UINT8, "RGBA", GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV}},
        {"RGBA16", {4, 64, ImageBufferSpec::Type::UINT16, "RGBA", GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT}},
        {"RGB", {3, 24, ImageBufferSpec::Type::UINT8, "RGB", GL_RGBA8, GL_RGB, GL_UNSIGNED_BYTE}},
        {"R16", {1, 16, ImageBufferSpec::Type::UINT16, "R", GL_R16, GL_RED, GL_UNSIGNED_SHORT}},
        {"YUYV", {3, 16, ImageBufferSpec::Type::UINT8, "YUYV", GL_RG8, GL_RG, GL_UNSIGNED_SHORT}},
        {"UYVY", {3, 16, ImageBufferSpec::Type::UINT8, "UYVY", GL_RG8, GL_RG, GL_UNSIGNED_SHORT}},
        {"D", {1, 24, ImageBufferSpec::Type::FLOAT, "R", GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT}},
    };
}

/*************/
void Texture_ImageGfxImpl::bind()
{
    glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
    _activeTexture = _activeTexture - GL_TEXTURE0;
    glBindTextureUnit(_activeTexture, _glTex);
}

/*************/
void Texture_ImageGfxImpl::unbind()
{
#ifdef DEBUG
    glBindTextureUnit(_activeTexture, 0);
#endif
}

/*************/
void Texture_ImageGfxImpl::generateMipmap() const
{
    glGenerateTextureMipmap(_glTex);
}

/*************/
std::shared_ptr<Image> Texture_ImageGfxImpl::read(int mipmapLevel, const ImageBufferSpec& spec) const
{
    glBindTexture(_textureType, _glTex);
    auto img = std::make_shared<Image>(nullptr, spec);
    getTextureImage(_glTex, _textureType, mipmapLevel, _texFormat, _texType, img->getSpec().rawSize(), const_cast<void*>(img->data()));
    return img;
}

/*************/
void Texture_ImageGfxImpl::getTextureImage(GLuint textureId, GLenum /*textureType*/, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) const
{
    glGetTextureImage(textureId, level, format, type, bufSize, pixels);
}

/*************/
ImageBufferSpec Texture_ImageGfxImpl::reset(int width, int height, std::string pixelFormat, ImageBufferSpec spec, GLint multisample, bool cubemap, bool filtering)
{
    auto newSpec = initFromPixelFormat(width, height, pixelFormat, spec);
    initOpenGLTexture(multisample, cubemap, newSpec, filtering);

    return newSpec;
}

/*************/
std::pair<bool, std::optional<ImageBufferSpec>> Texture_ImageGfxImpl::update(std::shared_ptr<Image> img, ImageBufferSpec imgSpec, const ImageBufferSpec& textureSpec, bool filtering)
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

/*************/
bool Texture_ImageGfxImpl::isCompressed(const ImageBufferSpec& spec) const
{
    return spec.format == "RGB_DXT1" || spec.format == "RGBA_DXT5" || spec.format == "YCoCg_DXT5";
}

/*************/
void Texture_ImageGfxImpl::getTextureLevelParameteriv(GLenum /*target*/, GLint level, GLenum pname, GLint* params) const
{
    glGetTextureLevelParameteriv(_glTex, level, pname, params);
}

/*************/
void Texture_ImageGfxImpl::getTextureParameteriv(GLenum /*target*/, GLenum pname, GLint* params) const
{
    glGetTextureParameteriv(_glTex, pname, params);
}

/*************/
bool Texture_ImageGfxImpl::reallocatePBOs(int width, int height, int bytes)
{
    glDeleteBuffers(2, _pbos);
    auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    auto imageDataSize = width * height * bytes;

    glCreateBuffers(2, _pbos);
    glNamedBufferStorage(_pbos[0], imageDataSize, nullptr, flags);
    glNamedBufferStorage(_pbos[1], imageDataSize, nullptr, flags);

    _pbosPixels[0] = (GLubyte*)glMapNamedBufferRange(_pbos[0], 0, imageDataSize, flags);
    _pbosPixels[1] = (GLubyte*)glMapNamedBufferRange(_pbos[1], 0, imageDataSize, flags);

    if (!_pbosPixels[0] || !_pbosPixels[1])
    {
        Log::get() << Log::ERROR << "Texture_ImageGfxImpl::" << __FUNCTION__ << " - Unable to initialize upload PBOs" << Log::endl;
        return false;
    }

    return true;
}

/*************/
std::optional<std::pair<GLenum, GLenum>> Texture_ImageGfxImpl::updateUncompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb)
{
    GLenum internalFormat;
    GLenum dataFormat = GL_UNSIGNED_BYTE;

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
        Log::get() << Log::WARNING << "Texture_ImageGfxImpl::" << __FUNCTION__ << " - Unknown uncompressed format" << Log::endl;
        return {};
    }

    return {{internalFormat, dataFormat}};
}

/*************/
void Texture_ImageGfxImpl::readFromImageIntoPBO(GLuint pboId, int imageDataSize, std::shared_ptr<Image> img) const
{

    // Given an OpenGL PBO ID, we need to figure out which pixel buffer we need to read into.
    // This is done under the assumption that we have only 2 PBOs.
    int bufferIndex = 0;
    if (pboId == _pbos[0])
    {
        bufferIndex = 0;
    }
    else
    {
        bufferIndex = 1;
    }

    // Fill the next PBO with the image pixels
    auto pixels = _pbosPixels[bufferIndex];
    if (pixels != nullptr)
        memcpy(pixels, img->data(), imageDataSize);
}

/*************/
void Texture_ImageGfxImpl::copyPixelsBetweenPBOs(int imageDataSize) const
{
    glCopyNamedBufferSubData(_pbos[0], _pbos[1], 0, 0, imageDataSize);
}

/*************/
void Texture_ImageGfxImpl::initOpenGLTexture(GLint multisample, bool cubemap, const ImageBufferSpec& spec, bool filtering)
{
    // Create and initialize the texture
    if (glIsTexture(_glTex))
        glDeleteTextures(1, &_glTex);

    setTextureTypeFromOptions(multisample, cubemap);
    glGenTextures(1, &_glTex);
    setGLTextureParameters(filtering);
    allocateGLTexture(multisample, spec);
}

/*************/
void Texture_ImageGfxImpl::setGLTextureParameters(bool filtering) const
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

/*************/
void Texture_ImageGfxImpl::allocateGLTexture(GLint multisample, const ImageBufferSpec& spec)
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

/*************/
void Texture_ImageGfxImpl::setTextureTypeFromOptions(GLint multisample, bool cubemap)
{
    if (multisample > 1)
        _textureType = GL_TEXTURE_2D_MULTISAMPLE;
    else if (cubemap)
        _textureType = GL_TEXTURE_CUBE_MAP;
    else
        _textureType = GL_TEXTURE_2D;
}

/*************/
void Texture_ImageGfxImpl::updateCompressedSpec(ImageBufferSpec& spec) const
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

/*************/
std::optional<std::pair<GLenum, GLenum>> Texture_ImageGfxImpl::updateInternalAndDataFormat(bool isCompressed, const ImageBufferSpec& spec, std::shared_ptr<Image> img)
{
    Values srgb;
    img->getAttribute("srgb", srgb);

    if (isCompressed)
        return updateCompressedInternalAndDataFormat(spec, srgb);
    else
        return updateUncompressedInternalAndDataFormat(spec, srgb);
}

/*************/
void Texture_ImageGfxImpl::updateTextureFromImage(
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

/*************/
GLenum Texture_ImageGfxImpl::getChannelOrder(const ImageBufferSpec& spec)
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

/*************/
void Texture_ImageGfxImpl::updateGLTextureParameters(bool isCompressed, bool filtering)
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

/*************/
void Texture_ImageGfxImpl::reallocateAndInitGLTexture(
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

/*************/
bool Texture_ImageGfxImpl::updatePBOs(const ImageBufferSpec& spec, int imageDataSize, std::shared_ptr<Image> img)
{
    if (!reallocatePBOs(spec.width, spec.height, spec.pixelBytes()))
        return false;

    // Fill one of the PBOs right now
    readFromImageIntoPBO(_pbos[0], imageDataSize, img);

    // And copy it to the second PBO
    copyPixelsBetweenPBOs(imageDataSize);

    return true;
}

/*************/
ImageBufferSpec Texture_ImageGfxImpl::initFromPixelFormat(int width, int height, std::string pixelFormat, ImageBufferSpec spec)
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

/*************/
std::optional<std::pair<GLenum, GLenum>> Texture_ImageGfxImpl::updateCompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) const
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

} // namespace Splash::gfx::opengl
