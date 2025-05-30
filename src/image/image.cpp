#include "./image/image.h"

#include <fstream>
#include <future>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include "./core/serialize/serialize_imagebuffer.h"
#include "./core/serializer.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
Image::Image(RootObject* root, const std::optional<ImageBufferSpec> spec, TreeRegisterStatus registerToTree)
    : BufferObject(root, registerToTree)
    , _image(std::make_unique<ImageBuffer>())
    , _bufferImage(std::make_unique<ImageBuffer>())
{
    _type = "image";
    registerAttributes();

    if (spec)
    {
        initFromSpec(*spec);
    }
    else
    {
        // No spec passed, create an image with the default spec.
        initFromSpec(ImageBufferSpec(128, 128, 4, 32, ImageBufferSpec::Type::UINT8));
        _renderingPriority = Priority::MEDIA;
    }

    update();
}

/*************/
Image::Image(RootObject* root, TreeRegisterStatus registerToTree)
    : Image(root, std::nullopt, registerToTree)
{
}

/*************/
Image::~Image()
{
    std::lock_guard<std::shared_mutex> readLock(_readMutex);
    std::lock_guard<Spinlock> updateLock(_updateMutex);
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Image::~Image - Destructor" << Log::endl;
#endif
}

/*************/
const void* Image::data() const
{
    if (_image)
        return _image->data();
    else
        return nullptr;
}

/*************/
ImageBuffer Image::get() const
{
    ImageBuffer img;
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    if (_image)
        img = *_image;
    return img;
}

/*************/
ImageBufferSpec Image::getSpec() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    if (_image)
        return _image->getSpec();
    else
        return ImageBufferSpec();
}

/*************/
void Image::set(const ImageBuffer& img)
{
    std::lock_guard<Spinlock> updateLock(_updateMutex);
    *_bufferImage = img;
    _bufferImageUpdated = true;
    updateTimestamp();
}

/*************/
void Image::set(unsigned int w, unsigned int h, unsigned int channels, ImageBufferSpec::Type type)
{
    ImageBufferSpec spec(w, h, channels, 8 * sizeof(channels) * (int)type, type);
    ImageBuffer img(spec);

    std::lock_guard<Spinlock> updateLock(_updateMutex);
    std::swap(*_bufferImage, img);
    _bufferImageUpdated = true;
    updateTimestamp();
}

/*************/
SerializedObject Image::serialize() const
{
    std::shared_lock<std::shared_mutex> readLock(_readMutex);

    if (Timer::get().isDebug())
        Timer::get() << "serialize " + _name;

    if (!_image)
        return {};

    _image->setName(_name);
    std::vector<uint8_t> data;
    Serial::serialize(*_image, data);
    SerializedObject obj(ResizableArray(std::move(data)));

    if (Timer::get().isDebug())
        Timer::get() >> ("serialize " + _name);

    return obj;
}

/*************/
bool Image::deserialize(SerializedObject&& obj)
{
    if (obj.size() == 0)
        return false;

    if (Timer::get().isDebug())
        Timer::get() << "deserialize " + _name;

    // After this, obj does not hold any more data
    auto serializedImage = obj.grabData();

    // We only deserialize part of the serialized ImageBuffer,
    // to prevent copying the content of the buffer another time
    auto serializedImageIt = serializedImage.cbegin();
    _name = Serial::detail::deserializer<std::string>(serializedImageIt);
    const ImageBufferSpec spec(Serial::detail::deserializer<std::string>(serializedImageIt));

    // If the specs did change, regenerate a buffer
    // Otherwise make sure the timestamp is updated
    if (spec != _bufferImage->getSpec())
        _bufferImage = std::make_unique<ImageBuffer>(spec);
    else
        _bufferImage->getSpec().timestamp = spec.timestamp;

    auto shift = std::distance(serializedImage.cbegin(), serializedImageIt);
    serializedImage.shift(shift);
    _bufferImage->setRawBuffer(std::move(serializedImage));

    _bufferImageUpdated = true;
    updateTimestamp(_bufferImage->getSpec().timestamp);

    if (Timer::get().isDebug())
        Timer::get() >> ("deserialize " + _name);

    return true;
}

/*************/
bool Image::read(const std::string& filename)
{
    const auto configurationPath = _root ? _root->getConfigurationPath() : "/";
    const auto filepath = Utils::getFullPathFromFilePath(filename, configurationPath);
    if (!_isConnectedToRemote)
        return readFile(filepath);
    else
        return true;
}

/*************/
bool Image::readFile(const std::string& filename)
{
    if (!std::ifstream(filename).is_open())
    {
        Log::get() << Log::WARNING << "Image::" << __FUNCTION__ << " - Unable to load file " << filename << Log::endl;
        return false;
    }

    const bool is16bits = stbi_is_16_bit(filename.c_str());

    int w, h, c;

    // All 8bpc images are converted to RGBA, to facilitate handling later on
    // 16bpc images are left as is
    uint8_t* rawImage = is16bits ? reinterpret_cast<uint8_t*>(stbi_load_16(filename.c_str(), &w, &h, &c, 0)) : stbi_load(filename.c_str(), &w, &h, &c, 4);

    if (!rawImage)
    {
        Log::get() << Log::WARNING << "Image::" << __FUNCTION__ << " - Caught an error while opening image file " << filename << Log::endl;
        return false;
    }

    const auto channels = is16bits ? c : 4;
    const int32_t bpp = is16bits ? channels * 16 : channels * 8;
    const auto type = is16bits ? ImageBufferSpec::Type::UINT16 : ImageBufferSpec::Type::UINT8;

    auto spec = ImageBufferSpec(w, h, channels, bpp, type);
    spec.videoFrame = false;

    auto img = ImageBuffer(spec);
    memcpy(img.data(), rawImage, spec.rawSize());

    stbi_image_free(rawImage);

    {
        std::lock_guard<Spinlock> updateLock(_updateMutex);
        std::swap(*_bufferImage, img);
        _bufferImageUpdated = true;
    }

    updateTimestamp();
    return true;
}

/*************/
void Image::zero()
{
    std::lock_guard<Spinlock> updateLock(_updateMutex);
    _bufferImage->zero();
    _bufferImageUpdated = true;
    updateTimestamp();
}

/*************/
void Image::update()
{
    if (_bufferImageUpdated)
    {
        std::lock_guard<Spinlock> updateLock(_updateMutex);
        std::lock_guard<std::shared_mutex> readLock(_readMutex);
        _image.swap(_bufferImage);
        _bufferImageUpdated = false;

        if (_remoteType.empty() || _type == _remoteType)
            updateMediaInfo();
    }
    else if (_benchmark)
    {
        updateTimestamp();
    }
}

/*************/
void Image::updateTimestamp(int64_t timestamp)
{
    BufferObject::updateTimestamp(timestamp);
    if (_bufferImage)
        _bufferImage->getSpec().timestamp = _timestamp;
}

/*************/
void Image::updateMediaInfo()
{
    Values mediaInfo;
    auto spec = _image->getSpec();
    mediaInfo.push_back(Value(spec.width, "width"));
    mediaInfo.push_back(Value(spec.height, "height"));
    mediaInfo.push_back(Value(spec.bpp, "bpp"));
    mediaInfo.push_back(Value(spec.channels, "channels"));
    mediaInfo.push_back(Value(spec.format, "format"));
    mediaInfo.push_back(Value(_srgb, "srgb"));
    updateMoreMediaInfo(mediaInfo);

    std::lock_guard<std::mutex> lock(_mediaInfoMutex);
    std::swap(_mediaInfo, mediaInfo);
}

/*************/
bool Image::write(const std::string& filename)
{
    int strSize = filename.size();
    if (strSize < 5)
        return false;

    if (!_image or !_image->data())
        return false;

    auto spec = _image->getSpec();

    std::shared_lock<std::shared_mutex> readLock(_readMutex);
    if (filename.substr(strSize - 3, strSize) == "png")
    {
        auto result = stbi_write_png(filename.c_str(), spec.width, spec.height, spec.channels, _image->data(), spec.width * spec.channels);
        return (result != 0);
    }
    else if (filename.substr(strSize - 3, strSize) == "bmp")
    {
        auto result = stbi_write_bmp(filename.c_str(), spec.width, spec.height, spec.channels, _image->data());
        return (result != 0);
    }
    else if (filename.substr(strSize - 3, strSize) == "tga")
    {
        auto result = stbi_write_tga(filename.c_str(), spec.width, spec.height, spec.channels, _image->data());
        return (result != 0);
    }
    else
        return false;
}

/*************/
RgbValue Image::readPixel(uint32_t x, uint32_t y) const
{
    const auto* imageBytes = static_cast<const uint8_t*>(data());
    const auto spec = getSpec();

    assert(_image && "Image is uninitialized!"); // Make sure the image is already initialized
    assert(spec.type == ImageBufferSpec::Type::UINT8 && "Only UINT8 textures support reading pixels.");
    assert(spec.format == "RGBA" && "Only RGBA textures support reading pixels.");
    assert(spec.bpp == 32 && spec.channels == 4 && "Only 8 bits per channel textures support reading pixels.");

    // Note that since x and  y are already unsigned, they should always be > 0.
    // However, if a negative value is passed, it should result in a very large unsigned value.
    assert(x < spec.width && "Pixel x is outside of image");
    assert(y < spec.height && "Pixel y is outside of image");

    assert(imageBytes != nullptr && "Image doesn't contain any data");

    auto index = (x + y * spec.width) * 4;
    return RgbValue(imageBytes[index], imageBytes[index + 1], imageBytes[index + 2]);
}

/*************/
void Image::initFromSpec(const ImageBufferSpec& spec)
{
    ImageBuffer img(spec);
    img.zero();

    std::lock_guard<Spinlock> updateLock(_updateMutex);
    std::swap(*_bufferImage, img);
    _bufferImageUpdated = true;
    updateTimestamp();
}

/*************/
void Image::createPattern()
{
    ImageBufferSpec spec(512, 512, 4, 8 * 4, ImageBufferSpec::Type::UINT8);
    ImageBuffer img(spec);

    uint8_t* p = reinterpret_cast<uint8_t*>(img.data());

    for (int y = 0; y < 512; ++y)
        for (int x = 0; x < 512; ++x)
        {
            if (x % 16 > 7 && y % 64 > 31)
                for (int c = 0; c < 4; ++c)
                    p[(x + y * 512) * 4 + c] = 255;
            else
                for (int c = 0; c < 4; ++c)
                    p[(x + y * 512) * 4 + c] = 0;
        }

    std::lock_guard<Spinlock> updateLock(_updateMutex);
    std::swap(*_image, img);
    updateTimestamp();
}

/*************/
void Image::registerAttributes()
{
    BufferObject::registerAttributes();

    addAttribute(
        "flip",
        [&](const Values& args) {
            _flip = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_flip}; },
        {'b'});
    setAttributeDescription("flip", "Mirrors the image on the Y axis");

    addAttribute(
        "flop",
        [&](const Values& args) {
            _flop = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_flop}; },
        {'b'});
    setAttributeDescription("flop", "Mirrors the image on the X axis");

    addAttribute(
        "file",
        [&](const Values& args) {
            _filepath = args[0].as<std::string>();
            if (_filepath.empty())
                return true;
            return read(_filepath);
        },
        [&]() -> Values { return {_filepath}; },
        {'s'});
    setAttributeDescription("file", "Image file to load");

    addAttribute(
        "reload",
        [&](const Values&) {
            read(_filepath);
            return true;
        },
        [&]() -> Values { return {false}; },
        {});
    setAttributeDescription("reload", "Reload the file");

    addAttribute(
        "srgb",
        [&](const Values& args) {
            _srgb = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_srgb}; },
        {'b'});
    setAttributeDescription("srgb", "Set to true if the image file is stored as sRGB");

    addAttribute("benchmark",
        [&](const Values& args) {
            _benchmark = args[0].as<bool>();
            return true;
        },
        {'b'});
    setAttributeDescription("benchmark", "Set to true to resend the image even when not updated");

    addAttribute(
        "pattern",
        [&](const Values& args) {
            if (_showPattern != args[0].as<bool>())
            {
                _showPattern = args[0].as<bool>();
                if (_showPattern)
                    createPattern();
                else
                    read(_filepath);
            }
            return true;
        },
        [&]() -> Values { return {_showPattern}; },
        {'b'});
    setAttributeDescription("pattern", "Set to true to replace the image with a pattern");

    addAttribute(
        "mediaInfo",
        [&](const Values& args) {
            std::lock_guard<std::mutex> lock(_mediaInfoMutex);
            _mediaInfo = args;
            return true;
        },
        [&]() -> Values {
            std::lock_guard<std::mutex> lock(_mediaInfoMutex);
            return _mediaInfo;
        },
        {});
    setAttributeDescription("mediaInfo", "Media information (size, duration, etc.)");
}

} // namespace Splash
