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
    glGetTextureLevelParameteriv(_glTex, level, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(_glTex, level, GL_TEXTURE_HEIGHT, &height);
    auto size = width * height * 4;
    ResizableArray<uint8_t> buffer(size);
    glGetTextureImage(_glTex, level, GL_RGBA, GL_UNSIGNED_BYTE, buffer.size(), buffer.data());

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
    GLint width, height;
    glGetTextureLevelParameteriv(_glTex, level, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(_glTex, level, GL_TEXTURE_HEIGHT, &height);

    auto spec = _spec;
    spec.width = width;
    spec.height = height;

    auto image = ImageBuffer(spec);
    glGetTextureImage(_glTex, mipmapLevel, _texFormat, _texType, image.getSize(), image.data());
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
    glGetTextureImage(_glTex, 0, _texFormat, _texType, img->getSpec().rawSize(), (GLvoid*)img->data());
    return img;
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
void Texture_Image::init()
{
    _type = "texture_image";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;
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
