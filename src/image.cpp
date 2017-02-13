#include "image.h"

#include <fstream>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include "log.h"
#include "osUtils.h"
#include "threadpool.h"
#include "timer.h"

#define SPLASH_IMAGE_COPY_THREADS 2
#define SPLASH_IMAGE_SERIALIZED_HEADER_SIZE 4096

using namespace std;

namespace Splash
{

/*************/
Image::Image(weak_ptr<RootObject> root)
    : BufferObject(root)
{
    init();

    if (!root.expired() && root.lock()->getType() == "world")
        _worldObject = true;
}

/*************/
Image::Image(weak_ptr<RootObject> root, ImageBufferSpec spec)
    : BufferObject(root)
{
    init();
    set(spec.width, spec.height, spec.channels, spec.type);
}

/*************/
void Image::init()
{
    _type = "image";
    registerAttributes();

    // If the root object weak_ptr is expired, this means that
    // this object has been created outside of a World or Scene.
    // This is used for getting documentation "offline"
    if (_root.expired())
        return;

    createDefaultImage();
}

/*************/
Image::~Image()
{
    lock_guard<Spinlock> writeLock(_writeMutex);
    lock_guard<Spinlock> readlock(_readMutex);
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
    lock_guard<Spinlock> lock(_readMutex);
    if (_image)
        img = *_image;
    return img;
}

/*************/
ImageBufferSpec Image::getSpec() const
{
    lock_guard<Spinlock> lock(_readMutex);
    if (_image)
        return _image->getSpec();
    else
        return ImageBufferSpec();
}

/*************/
void Image::set(const ImageBuffer& img)
{
    lock_guard<Spinlock> lockRead(_readMutex);
    if (_image)
        *_image = img;
}

/*************/
void Image::set(unsigned int w, unsigned int h, unsigned int channels, ImageBufferSpec::Type type)
{
    ImageBufferSpec spec(w, h, channels, 8 * sizeof(channels) * (int)type, type);
    ImageBuffer img(spec);

    lock_guard<Spinlock> lock(_readMutex);
    if (!_image)
        _image = unique_ptr<ImageBuffer>(new ImageBuffer());
    std::swap(*_image, img);
    updateTimestamp();
}

/*************/
shared_ptr<SerializedObject> Image::serialize() const
{
    lock_guard<Spinlock> lock(_readMutex);

    if (Timer::get().isDebug())
        Timer::get() << "serialize " + _name;

    // We first get the xml version of the specs, and pack them into the obj
    if (!_image)
        return {};
    string xmlSpec = _image->getSpec().to_string();
    int nbrChar = xmlSpec.size();
    int imgSize = _image->getSpec().rawSize();
    int totalSize = SPLASH_IMAGE_SERIALIZED_HEADER_SIZE + imgSize;

    auto obj = make_shared<SerializedObject>(totalSize);

    auto currentObjPtr = obj->data();
    const char* ptr = reinterpret_cast<const char*>(&nbrChar);
    copy(ptr, ptr + sizeof(nbrChar), currentObjPtr);
    currentObjPtr += sizeof(nbrChar);

    const char* charPtr = reinterpret_cast<const char*>(xmlSpec.c_str());
    copy(charPtr, charPtr + nbrChar, currentObjPtr);
    currentObjPtr = obj->data() + SPLASH_IMAGE_SERIALIZED_HEADER_SIZE;

    // And then, the image
    const char* imgPtr = reinterpret_cast<const char*>(_image->data());
    if (imgPtr == NULL)
        return {};

    vector<unsigned int> threadIds;
    int stride = SPLASH_IMAGE_COPY_THREADS;
    for (int i = 0; i < stride - 1; ++i)
    {
        threadIds.push_back(SThread::pool.enqueue([=]() { copy(imgPtr + imgSize / stride * i, imgPtr + imgSize / stride * (i + 1), currentObjPtr + imgSize / stride * i); }));
    }
    copy(imgPtr + imgSize / stride * (stride - 1), imgPtr + imgSize, currentObjPtr + imgSize / stride * (stride - 1));
    SThread::pool.waitThreads(threadIds);

    if (Timer::get().isDebug())
        Timer::get() >> "serialize " + _name;

    return obj;
}

/*************/
bool Image::deserialize(const shared_ptr<SerializedObject>& obj)
{
    if (obj.get() == nullptr || obj->size() == 0)
        return false;

    if (Timer::get().isDebug())
        Timer::get() << "deserialize " + _name;

    // First, we get the size of the metadata
    int nbrChar;
    char* ptr = reinterpret_cast<char*>(&nbrChar);

    auto currentObjPtr = obj->data();
    copy(currentObjPtr, currentObjPtr + sizeof(nbrChar), ptr);
    currentObjPtr += sizeof(nbrChar);

    try
    {
        char xmlSpecChar[nbrChar];
        ptr = reinterpret_cast<char*>(xmlSpecChar);
        copy(currentObjPtr, currentObjPtr + nbrChar, ptr);
        currentObjPtr = obj->data() + SPLASH_IMAGE_SERIALIZED_HEADER_SIZE;
        string xmlSpec(xmlSpecChar);

        ImageBufferSpec spec;
        spec.from_string(xmlSpec.c_str());

        ImageBufferSpec curSpec = _bufferDeserialize.getSpec();
        if (spec != curSpec)
            _bufferDeserialize = ImageBuffer(spec);

        auto rawBuffer = obj->grabData();
        rawBuffer.shift(SPLASH_IMAGE_SERIALIZED_HEADER_SIZE);
        _bufferDeserialize.setRawBuffer(std::move(rawBuffer));

        if (!_bufferImage)
            _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer());
        std::swap(*_bufferImage, _bufferDeserialize);
        _imageUpdated = true;

        updateTimestamp();
    }
    catch (...)
    {
        Log::get() << Log::ERROR << "Image::" << __FUNCTION__ << " - Unable to deserialize the given object" << Log::endl;
        return false;
    }

    if (Timer::get().isDebug())
        Timer::get() >> "deserialize " + _name;

    return true;
}

/*************/
bool Image::read(const string& filename)
{
    _filepath = Utils::getFullPathFromFilePath(filename, _root.lock()->getConfigurationPath());
    if (!_isConnectedToRemote)
        return readFile(_filepath);
    else
        return true;
}

/*************/
bool Image::readFile(const string& filename)
{
    if (!ifstream(filename).is_open())
    {
        Log::get() << Log::WARNING << "Image::" << __FUNCTION__ << " - Unable to load file " << filename << Log::endl;
        return false;
    }

    int w, h, c;
    uint8_t* rawImage = stbi_load(filename.c_str(), &w, &h, &c, 0);

    if (!rawImage)
    {
        Log::get() << Log::WARNING << "Image::" << __FUNCTION__ << " - Caught an error while opening image file " << filename << Log::endl;
        return false;
    }

    auto spec = ImageBufferSpec(w, h, c, 8 * c, ImageBufferSpec::Type::UINT8);
    if (c == 1)
        spec.format = "R";
    else if (c == 3)
        spec.format = "RGB";
    else if (c == 4)
        spec.format = "RGBA";
    else
        return false;
    auto img = ImageBuffer(spec);
    memcpy(img.data(), rawImage, w * h * c);
    stbi_image_free(rawImage);

    lock_guard<Spinlock> lock(_writeMutex);
    if (!_bufferImage)
        _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer());
    std::swap(*_bufferImage, img);
    _imageUpdated = true;

    updateTimestamp();

    return true;
}

/*************/
void Image::zero()
{
    lock_guard<Spinlock> lock(_readMutex);
    if (!_image)
        return;

    _image->zero();
}

/*************/
void Image::update()
{
    lock_guard<Spinlock> lockRead(_readMutex);
    lock_guard<Spinlock> lockWrite(_writeMutex);
    if (_imageUpdated)
    {
        _image.swap(_bufferImage);
        _imageUpdated = false;
    }
    else if (_benchmark)
        updateTimestamp();
}

/*************/
bool Image::write(const std::string& filename)
{
    int strSize = filename.size();
    if (strSize < 5)
        return false;

    if (!_image)
        return false;

    auto spec = _image->getSpec();

    lock_guard<Spinlock> lock(_readMutex);
    if (filename.substr(strSize - 3, strSize) == "png")
    {
        auto result = stbi_write_png(filename.c_str(), spec.width, spec.height, spec.channels, _image->data(), spec.rawSize());
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
void Image::createDefaultImage()
{
    ImageBufferSpec spec(128, 128, 4, 32, ImageBufferSpec::Type::UINT8);
    ImageBuffer img(spec);
    img.zero();

    lock_guard<Spinlock> lock(_readMutex);
    if (!_image)
        _image = unique_ptr<ImageBuffer>(new ImageBuffer());
    std::swap(*_image, img);
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

    lock_guard<Spinlock> lock(_readMutex);
    if (!_image)
        _image = unique_ptr<ImageBuffer>(new ImageBuffer());
    std::swap(*_image, img);
    updateTimestamp();
}

/*************/
void Image::registerAttributes()
{
    BufferObject::registerAttributes();

    addAttribute("flip",
        [&](const Values& args) {
            _flip = (args[0].as<int>() > 0) ? true : false;
            return true;
        },
        [&]() -> Values { return {_flip}; },
        {'n'});
    setAttributeDescription("flip", "Mirrors the image on the Y axis");

    addAttribute("flop",
        [&](const Values& args) {
            _flop = (args[0].as<int>() > 0) ? true : false;
            return true;
        },
        [&]() -> Values { return {_flop}; },
        {'n'});
    setAttributeDescription("flop", "Mirrors the image on the X axis");

    addAttribute("file", [&](const Values& args) { return read(args[0].as<string>()); }, [&]() -> Values { return {_filepath}; }, {'s'});
    setAttributeDescription("file", "Image file to load");

    addAttribute("srgb",
        [&](const Values& args) {
            _srgb = (args[0].as<int>() > 0) ? true : false;
            return true;
        },
        [&]() -> Values { return {_srgb}; },
        {'n'});
    setAttributeDescription("srgb", "Set to 1 if the image file is stored as sRGB");

    addAttribute("benchmark",
        [&](const Values& args) {
            if (args[0].as<int>() > 0)
                _benchmark = true;
            else
                _benchmark = false;
            return true;
        },
        {'n'});
    setAttributeDescription("benchmark", "Set to 1 to resend the image even when not updated");

    addAttribute("pattern",
        [&](const Values& args) {
            if (args[0].as<int>() == 1)
                createPattern();
            return true;
        },
        [&]() -> Values { return {false}; },
        {'n'});
    setAttributeDescription("pattern", "Set to 1 to replace the image with a pattern");
}

} // end of namespace
