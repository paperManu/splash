#include "texture_image.h"

#include "image.h"
#include "log.h"
#include "threadpool.h"
#include "timer.h"

#include <string>

#define SPLASH_TEXTURE_COPY_THREADS 2

using namespace std;

namespace Splash
{

/*************/
Texture_Image::Texture_Image(RootObject* root)
    : Texture(root)
{
    init();
}

/*************/
Texture_Image::Texture_Image(RootObject* root, GLsizei width, GLsizei height, const string& pixelFormat, const GLvoid* data)
    : Texture(root)
{
    init();
    reset(width, height, pixelFormat, data);
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
Texture_Image& Texture_Image::operator=(const shared_ptr<Image>& img)
{
    _img = weak_ptr<Image>(img);
    return *this;
}

/*************/
void Texture_Image::bind()
{
    glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
    glBindTexture(GL_TEXTURE_2D, _glTex);
}

/*************/
void Texture_Image::generateMipmap() const
{
    glBindTexture(GL_TEXTURE_2D, _glTex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/*************/
RgbValue Texture_Image::getMeanValue() const
{
    glBindTexture(GL_TEXTURE_2D, _glTex);
    int level = _texLevels - 1;
    int width, height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &height);
    auto size = width * height * 4;
    ResizableArray<uint8_t> buffer(size);
    glGetTexImage(GL_TEXTURE_2D, level, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data());
    glBindTexture(GL_TEXTURE_2D, 0);

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
bool Texture_Image::linkTo(const std::shared_ptr<BaseObject>& obj)
{
    // Mandatory before trying to link
    if (!Texture::linkTo(obj))
        return false;

    if (dynamic_pointer_cast<Image>(obj).get() != nullptr)
    {
        auto img = dynamic_pointer_cast<Image>(obj);
        _img = weak_ptr<Image>(img);
        return true;
    }

    return false;
}

/*************/
shared_ptr<Image> Texture_Image::read()
{
    auto img = make_shared<Image>(_root, _spec);
    glBindTexture(GL_TEXTURE_2D, _glTex);
    glGetTexImage(GL_TEXTURE_2D, 0, _texFormat, _texType, (GLvoid*)img->data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return img;
}

/*************/
void Texture_Image::reset(int width, int height, const string& pixelFormat, const GLvoid* data)
{
    if (width == 0 || height == 0)
    {
        Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Texture size is null" << Log::endl;
        return;
    }

    // Fill texture parameters
    _spec.width = width;
    _spec.height = height;
    auto realPixelFormat = pixelFormat;
    if (realPixelFormat.empty())
        realPixelFormat = "RGBA";
    _pixelFormat = realPixelFormat;

    if (realPixelFormat == "RGBA")
    {
        _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
        _texInternalFormat = GL_RGBA;
        _texFormat = GL_RGBA;
        _texType = GL_UNSIGNED_INT_8_8_8_8_REV;
    }
    else if (realPixelFormat == "sRGBA")
    {
        _spec = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
        _texInternalFormat = GL_SRGB8_ALPHA8;
        _texFormat = GL_RGBA;
        _texType = GL_UNSIGNED_INT_8_8_8_8_REV;
    }
    else if (realPixelFormat == "RGBA16")
    {
        _spec = ImageBufferSpec(width, height, 4, 64, ImageBufferSpec::Type::UINT8, "RGBA");
        _texInternalFormat = GL_RGBA16;
        _texFormat = GL_RGBA;
        _texType = GL_UNSIGNED_INT_8_8_8_8_REV;
    }
    else if (realPixelFormat == "RGB")
    {
        _spec = ImageBufferSpec(width, height, 3, 24, ImageBufferSpec::Type::UINT8, "RGB");
        _texInternalFormat = GL_RGBA;
        _texFormat = GL_RGB;
        _texType = GL_UNSIGNED_BYTE;
    }
    else if (realPixelFormat == "R16")
    {
        _spec = ImageBufferSpec(width, height, 1, 16, ImageBufferSpec::Type::UINT16, "R");
        _texInternalFormat = GL_R16;
        _texFormat = GL_RED;
        _texType = GL_UNSIGNED_SHORT;
    }
    else if (realPixelFormat == "YUYV" || realPixelFormat == "UYVY")
    {
        _spec = ImageBufferSpec(width, height, 3, 16, ImageBufferSpec::Type::UINT8, realPixelFormat);
        _texInternalFormat = GL_RGBA;
        _texFormat = GL_RGBA;
        _texType = GL_UNSIGNED_INT_8_8_8_8_REV;
    }
    else if (realPixelFormat == "D")
    {
        _spec = ImageBufferSpec(width, height, 1, 16, ImageBufferSpec::Type::UINT16, "R");
        _texInternalFormat = GL_DEPTH_COMPONENT;
        _texFormat = GL_DEPTH_COMPONENT;
        _texType = GL_FLOAT;
    }

    // Create and initialize the texture
    if (_glTex == 0)
    {
        glGenTextures(1, &_glTex);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _glTex);

        if (_texInternalFormat == GL_DEPTH_COMPONENT)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _glTextureWrap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _glTextureWrap);

            if (_filtering)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, _texInternalFormat, width, height, 0, _texFormat, _texType, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _glTex);
        glTexImage2D(GL_TEXTURE_2D, 0, _texInternalFormat, width, height, 0, _texFormat, _texType, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Reset the texture to size " << width << "x" << height << Log::endl;
#endif
}

/*************/
void Texture_Image::resize(int width, int height)
{
    if (!_resizable)
        return;
    if (width != _spec.width || height != _spec.height)
        reset(width, height, _pixelFormat, 0);
}

/*************/
void Texture_Image::unbind()
{
#ifdef DEBUG
    glActiveTexture((GLenum)_activeTexture);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

/*************/
GLenum Texture_Image::getChannelOrder(const ImageBufferSpec& spec)
{
    GLenum glChannelOrder = GL_RGB;

    if (spec.format == "BGR")
        glChannelOrder = GL_BGR;
    else if (spec.format == "RGB")
        glChannelOrder = GL_RGB;
    else if (spec.format == "BGRA")
        glChannelOrder = GL_BGRA;
    else if (spec.format == "RGBA")
        glChannelOrder = GL_RGBA;
    else if (spec.format == "RGB" || spec.format == "RGB_DXT1")
        glChannelOrder = GL_RGB;
    else if (spec.format == "RGBA" || spec.format == "RGBA_DXT5")
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
void Texture_Image::update()
{
    lock_guard<mutex> lock(_mutex);

    // If _img is nullptr, this texture is not set from an Image
    if (_img.expired())
        return;
    auto img = _img.lock();

    if (img->getTimestamp() == _timestamp)
        return;
    img->update();

    auto spec = img->getSpec();
    Values srgb, flip, flop;
    img->getAttribute("srgb", srgb);
    img->getAttribute("flip", flip);
    img->getAttribute("flop", flop);

    if (!(bool)glIsTexture(_glTex))
        glGenTextures(1, &_glTex);

    // Store the image data size
    int imageDataSize = spec.rawSize();
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
    GLenum dataFormat;
    if (!isCompressed)
    {
        if (spec.channels == 4 && spec.type == ImageBufferSpec::Type::UINT8)
        {
            dataFormat = GL_UNSIGNED_INT_8_8_8_8_REV;
            if (srgb[0].as<int>() > 0)
                internalFormat = GL_SRGB8_ALPHA8;
            else
                internalFormat = GL_RGBA;
        }
        else if (spec.channels == 3 && spec.type == ImageBufferSpec::Type::UINT8)
        {
            dataFormat = GL_UNSIGNED_BYTE;
            if (srgb[0].as<int>() > 0)
                internalFormat = GL_SRGB8_ALPHA8;
            else
                internalFormat = GL_RGBA;
        }
        else if (spec.channels == 1 && spec.type == ImageBufferSpec::Type::UINT16)
        {
            dataFormat = GL_UNSIGNED_SHORT;
            internalFormat = GL_R16;
        }
        else if (spec.channels == 2 && spec.type == ImageBufferSpec::Type::UINT8)
        {
            dataFormat = GL_UNSIGNED_SHORT;
            internalFormat = GL_RG;
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
            if (srgb[0].as<int>() > 0)
                internalFormat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
            else
                internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        }
        else if (spec.format == "RGBA_DXT5")
        {
            if (srgb[0].as<int>() > 0)
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
        glGenTextures(1, &_glTex);

        glBindTexture(GL_TEXTURE_2D, _glTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _glTextureWrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _glTextureWrap);

        if (_filtering)
        {
            if (isCompressed)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            else
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        // Create or update the texture parameters
        if (!isCompressed)
        {
#ifdef DEBUG
            Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Creating a new texture" << Log::endl;
#endif
            img->lock();
            glTexStorage2D(GL_TEXTURE_2D, _texLevels, internalFormat, spec.width, spec.height);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spec.width, spec.height, glChannelOrder, dataFormat, img->data());
            img->unlock();
        }
        else if (isCompressed)
        {
#ifdef DEBUG
            Log::get() << Log::DEBUGGING << "Texture_Image::" << __FUNCTION__ << " - Creating a new compressed texture" << Log::endl;
#endif

            img->lock();
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalFormat, spec.width, spec.height, 0, imageDataSize, img->data());
            img->unlock();
        }
        updatePbos(spec.width, spec.height, spec.pixelBytes());

        // Fill one of the PBOs right now
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[0]);
        GLubyte* pixels = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, imageDataSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
        if (pixels != NULL)
        {
            img->lock();
            memcpy((void*)pixels, img->data(), imageDataSize);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            img->unlock();
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        // And copy it to the second PBO
        glBindBuffer(GL_COPY_READ_BUFFER, _pbos[0]);
        glBindBuffer(GL_COPY_WRITE_BUFFER, _pbos[1]);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, imageDataSize);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

#ifdef DEBUG
        glBindTexture(GL_TEXTURE_2D, 0);
#endif

        _spec = spec;
    }
    // Update the content of the texture, i.e the image
    else
    {
        glBindTexture(GL_TEXTURE_2D, _glTex);

        // Copy the pixels from the current PBO to the texture
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[_pboReadIndex]);
        if (!isCompressed)
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spec.width, spec.height, glChannelOrder, dataFormat, 0);
        else
            glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spec.width, spec.height, internalFormat, imageDataSize, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#ifdef DEBUG
        glBindTexture(GL_TEXTURE_2D, 0);
#endif

        _pboReadIndex = (_pboReadIndex + 1) % 2;

        // Fill the next PBO with the image pixels
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[_pboReadIndex]);
        GLubyte* pixels = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, imageDataSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
        if (pixels != NULL)
        {
            img->lock();

            _pboCopyThreadIds.clear();
            int stride = SPLASH_TEXTURE_COPY_THREADS;
            int size = imageDataSize;
            for (int i = 0; i < stride - 1; ++i)
            {
                _pboCopyThreadIds.push_back(SThread::pool.enqueue(
                    [=]() { copy((char*)img->data() + size / stride * i, (char*)img->data() + size / stride * (i + 1), (char*)pixels + size / stride * i); }));
            }
            _pboCopyThreadIds.push_back(
                SThread::pool.enqueue([=]() { copy((char*)img->data() + size / stride * (stride - 1), (char*)img->data() + size, (char*)pixels + size / stride * (stride - 1)); }));
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    // If needed, specify some uniforms for the shader which will use this texture
    _shaderUniforms.clear();
    if (spec.format == "YCoCg_DXT5")
        _shaderUniforms["YCoCg"] = {1};
    else
        _shaderUniforms["YCoCg"] = {0};

    if (spec.format == "UYVY")
        _shaderUniforms["YUV"] = {1};
    else if (spec.format == "YUYV")
        _shaderUniforms["YUV"] = {2};
    else
        _shaderUniforms["YUV"] = {0};

    _shaderUniforms["flip"] = flip;
    _shaderUniforms["flop"] = flop;
    _shaderUniforms["size"] = {(float)_spec.width, (float)_spec.height};

    _timestamp = img->getTimestamp();

    if (_filtering && !isCompressed)
        generateMipmap();
}

/*************/
void Texture_Image::flushPbo()
{
    if (_pboCopyThreadIds.size() != 0)
    {
        SThread::pool.waitThreads(_pboCopyThreadIds);
        _pboCopyThreadIds.clear();

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[_pboReadIndex]);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

        if (!_img.expired())
            _img.lock()->unlock();
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
}

/*************/
void Texture_Image::init()
{
    _type = "texture_image";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    _timestamp = Timer::getTime();

    glGenBuffers(2, _pbos);
}

/*************/
void Texture_Image::updatePbos(int width, int height, int bytes)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * bytes, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * bytes, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

/*************/
void Texture_Image::registerAttributes()
{
    Texture::registerAttributes();

    addAttribute("filtering",
        [&](const Values& args) {
            _filtering = args[0].as<int>() > 0 ? true : false;
            return true;
        },
        [&]() -> Values { return {_filtering}; },
        {'n'});
    setAttributeDescription("filtering", "Activate the mipmaps for this texture");

    addAttribute("clampToEdge",
        [&](const Values& args) {
            _glTextureWrap = args[0].as<int>() ? GL_CLAMP_TO_EDGE : GL_REPEAT;
            return true;
        },
        {'n'});
    setAttributeDescription("clampToEdge", "If set to 1, clamp the texture to the edge");

    addAttribute("size",
        [&](const Values& args) {
            resize(args[0].as<int>(), args[1].as<int>());
            return true;
        },
        {'n', 'n'});
    setAttributeDescription("size", "Change the texture size");
}

} // end of namespace
