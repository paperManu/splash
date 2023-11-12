#include "./graphics/texture_image.h"

#include <string>

#include "./image/image.h"
#include "./utils/log.h"
#include "./utils/timer.h"
#include "graphics/api/texture_image_gfx_impl.h"

namespace Splash
{

/*************/
Texture_Image::Texture_Image(RootObject* root, std::unique_ptr<gfx::Texture_ImageGfxImpl> gfxImpl)
    : Texture(root)
    , _gfxImpl(std::move(gfxImpl))
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
    int mipmapLevel = gfx::Texture_ImageGfxImpl::_texLevels - 1;
    const auto image = read(mipmapLevel);
    const auto imageSpec = image->getSpec(); // Invokes a mutex
    const auto width = imageSpec.width, height = imageSpec.height;

    RgbValue meanColor;
    for (uint y = 0; y < height; ++y)
    {
        RgbValue rowMeanColor;
        for (uint x = 0; x < width; ++x)
        {
            rowMeanColor += image->readPixel(x, y);
        }

        rowMeanColor /= static_cast<float>(width);
        meanColor += rowMeanColor;
    }
    meanColor /= static_cast<float>(height);

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
    int mipmapLevel = std::min<int>(level, gfx::Texture_ImageGfxImpl::_texLevels);
    // The copy constructor of `ImageBuffer`, which is returned by `std::shared_ptr<Image>::get()`, does a full copy of the underlying buffers.
    // So it is safe to return it from this method.
    return read(mipmapLevel)->get();
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
std::shared_ptr<Image> Texture_Image::read(int mipmapLevel) const
{
    return _gfxImpl->read(_root, mipmapLevel, _spec);
}

/*************/
void Texture_Image::reset(int width, int height, const std::string& pixelFormat, int multisample, bool cubemap)
{
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

    _spec = _gfxImpl->reset(width, height, _pixelFormat, _spec, _multisample, _cubemap, _filtering);

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
        reset(width, height, _pixelFormat, _multisample, _cubemap);
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
        Log::get() << Log::ERROR << "Texture_Image::" << __FUNCTION__ << " - Texture " << _name << " is multisampled, and can not be set from an image" << Log::endl;
        return;
    }

    img->update();
    const auto imgLock = img->getReadLock();

    auto imgSpec = img->getSpec(); // Invokes a mutex, so we should minimize the number of its calls.

    const auto [textureUpdated, updatedSpec] = _gfxImpl->update(img, imgSpec, _spec, _filtering);

    if (!textureUpdated)
        return;

    if (updatedSpec)
        _spec = *updatedSpec;

    _spec.timestamp = imgSpec.timestamp;

    updateShaderUniforms(imgSpec, img);

    if (_filtering && !_gfxImpl->isCompressed(imgSpec))
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
void Texture_Image::registerAttributes()
{
    Texture::registerAttributes();

    addAttribute("filtering",
        [&](const Values& args) {
            _filtering = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_filtering}; },
        {'b'});
    setAttributeDescription("filtering", "Activate the mipmaps for this texture");

    addAttribute("clampToEdge",
        [&](const Values& args) {
            _gfxImpl->setClampToEdge(args[0].as<bool>());
            return true;
        },
        {'b'});
    setAttributeDescription("clampToEdge", "If true, clamp the texture to the edge");

    addAttribute("size",
        [&](const Values& args) {
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
