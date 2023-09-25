#include "./graphics/texture_image.h"

#include <string>

#include "./image/image.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

constexpr int Texture_Image::_texLevels;

/*************/
Texture_Image::Texture_Image(RootObject* root)
    : Texture(root)
{
    init();
}

/*************/
Texture_Image::~Texture_Image()
{
    if (!_root)
        return;

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Texture_Image::~Texture_Image - Destructor" << Log::endl;
#endif

    glDeleteTextures(1, &_glTex);
    glDeleteBuffers(2, _pbos);
}

/*************/
Texture_Image& Texture_Image::operator=(const std::shared_ptr<Image>& img)
{
    _img = std::weak_ptr<Image>(img);
    return *this;
}

/*************/
RgbValue Texture_Image::getMeanValue() const
{
    int level = _texLevels - 1;
    int width, height;

    glBindTexture(_textureType, _glTex);
    getTextureLevelParameteriv(_textureType, level, GL_TEXTURE_WIDTH, &width);
    getTextureLevelParameteriv(_textureType, level, GL_TEXTURE_HEIGHT, &height);

    auto size = width * height * 4;
    ResizableArray<uint8_t> buffer(size);
    getTextureImage(_glTex, _textureType, level, GL_RGBA, GL_UNSIGNED_BYTE, buffer.size(), buffer.data());

    RgbValue meanColor;
    for (int y = 0; y < height; ++y)
    {
        RgbValue rowMeanColor;
        for (int x = 0; x < width; ++x)
        {
            auto index = (x + y * width) * 4;
            RgbValue color(buffer[index], buffer[index + 1], buffer[index + 2]);
            rowMeanColor += color;
        }
        rowMeanColor /= static_cast<float>(width);
        meanColor += rowMeanColor;
    }
    meanColor /= height;

    return meanColor;
}

/*************/
std::unordered_map<std::string, Values> Texture_Image::getShaderUniforms() const
{
    auto uniforms = _shaderUniforms;
    uniforms["size"] = {static_cast<float>(_spec.width), static_cast<float>(_spec.height)};
    return uniforms;
}

/*************/
ImageBuffer Texture_Image::grabMipmap(unsigned int level) const
{
    int mipmapLevel = std::min<int>(level, _texLevels);
    GLint width = 0, height = 0;

    glBindTexture(_textureType, _glTex);
    getTextureLevelParameteriv(_textureType, level, GL_TEXTURE_WIDTH, &width);
    getTextureLevelParameteriv(_textureType, level, GL_TEXTURE_HEIGHT, &height);

    auto spec = _spec;
    spec.width = width;
    spec.height = height;

    auto image = ImageBuffer(spec);
    getTextureImage(_glTex, _textureType, mipmapLevel, _texFormat, _texType, image.getSize(), image.data());
    return image;
}

/*************/
bool Texture_Image::linkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (std::dynamic_pointer_cast<Image>(obj))
    {
        auto img = std::dynamic_pointer_cast<Image>(obj);
        img->setDirty();
        _img = std::weak_ptr<Image>(img);
        return true;
    }

    return false;
}

/*************/
void Texture_Image::unlinkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (std::dynamic_pointer_cast<Image>(obj))
    {
        auto img = std::dynamic_pointer_cast<Image>(obj);
        if (img == _img.lock())
            _img.reset();
    }
}

/*************/
std::shared_ptr<Image> Texture_Image::read()
{
    auto img = std::make_shared<Image>(_root, _spec);
    getTextureImage(_glTex, _textureType, 0, _texFormat, _texType, img->getSpec().rawSize(), (GLvoid*)img->data());
    return img;
}

/*************/
void Texture_Image::reset(int width, int height, const std::string& pixelFormat, const GLvoid* data, int multisample, bool cubemap) {
		if (width == 0 || height == 0)
		{
#ifdef DEBUG
		    Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Texture size is null" << Log::endl;
#endif
		    return;
		}

		// Fill texture parameters
		_pixelFormat = pixelFormat.empty() ? "RGBA" : pixelFormat;
		_multisample = multisample;
		_cubemap = multisample == 0 ? cubemap : false;

		initFromPixelFormat(width, height);

		initOpenGLTexture(data);
#ifdef DEBUG
		Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Reset the texture to size " << width << "x" << height << Log::endl;
#endif
}

/*************/
void Texture_Image::resize(int width, int height)
{
    if (!_resizable)
        return;
    if (static_cast<uint32_t>(width) != _spec.width || static_cast<uint32_t>(height) != _spec.height)
        reset(width, height, _pixelFormat, 0, _multisample, _cubemap);
}

/*************/
void Texture_Image::setTextureTypeFromOptions() {
    if (_multisample > 1)
	_textureType = GL_TEXTURE_2D_MULTISAMPLE;
    else if (_cubemap)
	_textureType = GL_TEXTURE_CUBE_MAP;
    else
	_textureType = GL_TEXTURE_2D;
}

/*************/
void Texture_Image::allocateGLTexture(const GLvoid* data) {
    switch(_textureType) {

	case GL_TEXTURE_2D_MULTISAMPLE: 
	    glTexStorage2DMultisample(_textureType, _multisample, _texInternalFormat, _spec.width, _spec.height, false); 
	    break;

	case GL_TEXTURE_CUBE_MAP:
	    glTexStorage2D(GL_TEXTURE_CUBE_MAP, _texLevels, _texInternalFormat, _spec.width, _spec.height); 
	    break;

	default:
	    glTexStorage2D(GL_TEXTURE_2D, _texLevels, _texInternalFormat, _spec.width, _spec.height);

	    if (data)
		glTexSubImage2D(_textureType, 0, 0, 0, _spec.width, _spec.height, _texFormat, _texType, data);

	    break;
    }
}

/*************/
void Texture_Image::initOpenGLTexture(const GLvoid* data) 
{
    // Create and initialize the texture
    if (glIsTexture(_glTex))
	glDeleteTextures(1, &_glTex);

    setTextureTypeFromOptions();

    glGenTextures(1, &_glTex);

    setGLTextureParameters();

    allocateGLTexture(data);
}

/*************/
GLenum Texture_Image::getChannelOrder(const ImageBufferSpec& spec)
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
bool Texture_Image::updateCompressedSpec(ImageBufferSpec& spec) const 
{
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

/*************/
std::optional<std::pair<GLenum, GLenum>> Texture_Image::updateCompressedInternalAndDataFormat(const ImageBufferSpec& spec, const Values& srgb) const 
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
	Log::get() << Log::WARNING << "GLESTexture_Image::" << __FUNCTION__ << " - Unknown compressed format" << Log::endl;
	return {};
    }

    return {{ internalFormat, dataFormat }};
}

/*************/
std::optional<std::pair<GLenum, GLenum>> Texture_Image::updateInternalAndDataFormat(bool isCompressed, const ImageBufferSpec& spec, std::shared_ptr<Image> img) 
{
    Values srgb;
    img->getAttribute("srgb", srgb);

    if (isCompressed)
	return updateCompressedInternalAndDataFormat(spec, srgb);
    else
	return updateUncompressedInternalAndDataFormat(spec, srgb);
}

/*************/
void Texture_Image::updateGLTextureParameters(bool isCompressed) 
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

/*************/
void Texture_Image::reallocateAndInitGLTexture(bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize) const
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

/*************/
bool Texture_Image::swapPBOs(const ImageBufferSpec& spec, int imageDataSize, std::shared_ptr<Image> img) 
{
    if (!updatePbos(spec.width, spec.height, spec.pixelBytes()))
	return false;

    // Fill one of the PBOs right now
    readFromImageIntoPBO(_pbos[0], imageDataSize, img);

    // And copy it to the second PBO
    copyPixelsBetweenPBOs(imageDataSize);

    _spec = spec;

    return true;
}

void Texture_Image::updateTextureFromPBO(bool isCompressed, GLenum internalFormat, const ImageBufferSpec& spec, GLenum glChannelOrder, GLenum dataFormat, std::shared_ptr<Image> img, int imageDataSize) 
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
    readFromImageIntoPBO(_pbos[_pboUploadIndex], imageDataSize, img);
}

/*************/
void Texture_Image::updateShaderUniforms(const ImageBufferSpec& spec, const std::shared_ptr<Image> img)
{
    // If needed, specify some uniforms for the shader which will use this texture
    _shaderUniforms.clear();

    Values flip, flop;
    img->getAttribute("flip", flip);
    img->getAttribute("flop", flop);

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

/*************/
void Texture_Image::update() 
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

    // Must be called before `updateCompressedSpec`, as it will change the height, which is involved in `rawSize`.
    // If you call `rawSize` after `updateCompressedSpec`, you might get an incorrect `imageDataSize` (depends on 
    // the format, check the function for which formats update the height), causing you to read off the buffer in 
    // `swapPBOs` and segfault.
    const int imageDataSize = spec.rawSize(); 

    const bool isCompressed = updateCompressedSpec(spec);
    const auto internalAndDataFormat = updateInternalAndDataFormat(isCompressed, spec, img);

    if(!internalAndDataFormat) 
	return;

    const auto [internalFormat, dataFormat] = internalAndDataFormat.value();

    const GLenum glChannelOrder = getChannelOrder(spec);
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

    updateShaderUniforms(spec, img);

    if (_filtering && !isCompressed)
	generateMipmap();
}

/*************/
void Texture_Image::init()
{
    _type = "texture_image";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;
}

/*************/
void Texture_Image::setGLTextureParameters() const {
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

/*************/
void Texture_Image::registerAttributes()
{
    Texture::registerAttributes();

    addAttribute(
        "filtering",
        [&](const Values& args)
        {
            _filtering = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_filtering}; },
        {'b'});
    setAttributeDescription("filtering", "Activate the mipmaps for this texture");

    addAttribute("clampToEdge",
        [&](const Values& args)
        {
            _glTextureWrap = args[0].as<bool>() ? GL_CLAMP_TO_EDGE : GL_REPEAT;
            return true;
        },
        {'b'});
    setAttributeDescription("clampToEdge", "If true, clamp the texture to the edge");

    addAttribute(
        "size",
        [&](const Values& args)
        {
            resize(args[0].as<int>(), args[1].as<int>());
            return true;
        },
        [&]() -> Values {
            return {_spec.width, _spec.height};
        },
        {'i', 'i'});
    setAttributeDescription("size", "Change the texture size");
}

} // namespace Splash
