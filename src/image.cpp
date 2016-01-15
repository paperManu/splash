#include "image.h"

#include <fstream>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "log.h"
#include "osUtils.h"
#include "threadpool.h"
#include "timer.h"

#define SPLASH_IMAGE_COPY_THREADS 4

using namespace std;

namespace Splash {

/*************/
Image::Image()
{
    init();
}

/*************/
Image::Image(bool linked)
{
    init();
    _linkedToWorldObject = linked;
}

/*************/
Image::Image(ImageBufferSpec spec)
{
    init();
    set(spec.width, spec.height, spec.channels, spec.type);
}

/*************/
void Image::init()
{
    _type = "image";

    createDefaultImage();
    registerAttributes();
}

/*************/
Image::~Image()
{
    unique_lock<mutex> writeLock(_writeMutex);
    unique_lock<mutex> readlock(_readMutex);
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
    unique_lock<mutex> lock(_readMutex);
    if (_image)
        img = *_image;
    return img;
}

/*************/
ImageBufferSpec Image::getSpec() const
{
    unique_lock<mutex> lock(_readMutex);
    if (_image)
        return _image->getSpec();
    else
        return ImageBufferSpec();
}

/*************/
void Image::set(const ImageBuffer& img)
{
    unique_lock<mutex> lockRead(_readMutex);
    if (_image)
        *_image = img;
}

/*************/
void Image::set(unsigned int w, unsigned int h, unsigned int channels, ImageBufferSpec::Type type)
{
    ImageBufferSpec spec(w, h, channels, type);
    ImageBuffer img(spec);

    unique_lock<mutex> lock(_readMutex);
    if (!_image)
        _image = unique_ptr<ImageBuffer>(new ImageBuffer());
    std::swap(*_image, img);
    updateTimestamp();
}

/*************/
shared_ptr<SerializedObject> Image::serialize() const
{
    unique_lock<mutex> lock(_readMutex);

    if (Timer::get().isDebug())
        Timer::get() << "serialize " + _name;

    // We first get the xml version of the specs, and pack them into the obj
    if (!_image)
        return {};
    string xmlSpec = _image->getSpec().to_string();
    int nbrChar = xmlSpec.size();
    int imgSize = _image->getSpec().pixel_bytes() * _image->getSpec().width * _image->getSpec().height;
    int totalSize = sizeof(nbrChar) + nbrChar + imgSize;
    
    auto obj = make_shared<SerializedObject>(totalSize);

    auto currentObjPtr = obj->data();
    const char* ptr = reinterpret_cast<const char*>(&nbrChar);
    copy(ptr, ptr + sizeof(nbrChar), currentObjPtr);
    currentObjPtr += sizeof(nbrChar);

    const char* charPtr = reinterpret_cast<const char*>(xmlSpec.c_str());
    copy(charPtr, charPtr + nbrChar, currentObjPtr);
    currentObjPtr += nbrChar;

    // And then, the image
    const char* imgPtr = reinterpret_cast<const char*>(_image->data());
    if (imgPtr == NULL)
        return {};
    
    vector<unsigned int> threadIds;
    int stride = SPLASH_IMAGE_COPY_THREADS;
    for (int i = 0; i < stride - 1; ++i)
    {
        threadIds.push_back(SThread::pool.enqueue([=]() {
            copy(imgPtr + imgSize / stride * i, imgPtr + imgSize / stride * (i + 1), currentObjPtr + imgSize / stride * i);
        }));
    }
    copy(imgPtr + imgSize / stride * (stride - 1), imgPtr + imgSize, currentObjPtr + imgSize / stride * (stride - 1));
    SThread::pool.waitThreads(threadIds);

    if (Timer::get().isDebug())
        Timer::get() >> "serialize " + _name;

    return obj;
}

/*************/
bool Image::deserialize(shared_ptr<SerializedObject> obj)
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
        unique_lock<mutex> lockWrite(_writeMutex);

        char xmlSpecChar[nbrChar];
        ptr = reinterpret_cast<char*>(xmlSpecChar);
        copy(currentObjPtr, currentObjPtr + nbrChar, ptr);
        currentObjPtr += nbrChar;
        string xmlSpec(xmlSpecChar);

        ImageBufferSpec spec;
        spec.from_string(xmlSpec.c_str());

        ImageBufferSpec curSpec = _bufferDeserialize.getSpec();
        if (spec != curSpec)
            _bufferDeserialize = ImageBuffer(spec);

        int imgSize = _bufferDeserialize.getSpec().pixel_bytes() * _bufferDeserialize.getSpec().width * _bufferDeserialize.getSpec().height;
        ptr = reinterpret_cast<char*>(_bufferDeserialize.data());

        vector<unsigned int> threadIds;
        int stride = SPLASH_IMAGE_COPY_THREADS;
        for (int i = 0; i < stride - 1; ++i)
        {
            threadIds.push_back(SThread::pool.enqueue([=]() {
                copy(currentObjPtr + imgSize / stride * i, currentObjPtr + imgSize / stride * (i + 1), ptr + imgSize / stride * i);
            }));
        }
        copy(currentObjPtr + imgSize / stride * (stride - 1), currentObjPtr + imgSize, ptr + imgSize / stride * (stride - 1));
        SThread::pool.waitThreads(threadIds);

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
    _filepath = filename;
    if (!_linkedToWorldObject)
        return readFile(filename);
    else
        return true;
}

/*************/
bool Image::readFile(const string& filename)
{
    auto filepath = string(filename);
    if (Utils::getPathFromFilePath(filepath) == "" || filepath.find(".") == 0)
        filepath = _configFilePath + filepath;

    _filepath = filepath;

    if (!ifstream(_filepath).is_open())
    {
        Log::get() << Log::WARNING << "Image::" << __FUNCTION__ << " - Unable to load file " << filename << Log::endl;
        return false;
    }

    int w, h, c;
    uint8_t* rawImage = stbi_load(filepath.c_str(), &w, &h, &c, 3);

    if (!rawImage)
    {
        Log::get() << Log::WARNING << "Image::" << __FUNCTION__ << " - Caught an error while opening image file " << filepath << Log::endl;
        return false;
    }

    auto spec = ImageBufferSpec(w, h, c, ImageBufferSpec::Type::UINT8);
    spec.format = {"R", "G", "B"};
    auto img = ImageBuffer(spec);
    memcpy(img.data(), rawImage, w * h * c);
    stbi_image_free(rawImage);

    unique_lock<mutex> lock(_writeMutex);
    if (!_bufferImage)
        _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer());
    std::swap(*_bufferImage, img);
    _imageUpdated =  true;

    updateTimestamp();

    return true;
}

/*************/
void Image::setTo(float value)
{
    unique_lock<mutex> lock(_readMutex);
    if (!_image)
        return;

    _image->fill(value);
}

/*************/
void Image::update()
{
    unique_lock<mutex> lockRead(_readMutex);
    unique_lock<mutex> lockWrite(_writeMutex);
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
// TODO: reimplement image write with std_image_write
//    oiio::ImageOutput* out = oiio::ImageOutput::create(filename);
//    if (!out)
//        return false;
//
//    unique_lock<mutex> lock(_readMutex);
//    if (!_image)
//        return false;
//    out->open(filename, _image->spec());
//    out->write_image(_image->spec().format, _image->localpixels());
//    out->close();
//    delete out;
//
    return true;
}

/*************/
void Image::createDefaultImage()
{
    ImageBufferSpec spec(512, 512, 4, ImageBufferSpec::Type::UINT8);
    ImageBuffer img(spec);

    uint8_t* p = reinterpret_cast<uint8_t*>(img.data());

    for (int y = 0; y < 512; ++y)
        for (int x = 0; x < 512; ++x)
        {
            if (x % 16 > 7 && y % 64 > 31)
                for (int c = 0; c < 4; ++c)
                    p[(x + y * 512) * 3 + c] = 255;
            else
                for (int c = 0; c < 4; ++c)
                    p[(x + y * 512) * 3 + c] = 0;
        }

    unique_lock<mutex> lock(_readMutex);
    if (!_image)
        _image = unique_ptr<ImageBuffer>(new ImageBuffer());
    std::swap(*_image, img);
    updateTimestamp();
}

/*************/
void Image::registerAttributes()
{
    _attribFunctions["flip"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        _flip = (args[0].asInt() > 0) ? true : false;
        return true;
    }, [&]() -> Values {
        return {_flip};
    });

    _attribFunctions["flop"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        _flop = (args[0].asInt() > 0) ? true : false;
        return true;
    }, [&]() -> Values {
        return {_flop};
    });

    _attribFunctions["file"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        return read(args[0].asString());
    }, [&]() -> Values {
        return {_filepath};
    });

    _attribFunctions["srgb"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        _srgb = (args[0].asInt() > 0) ? true : false;     
        return true;
    }, [&]() -> Values {
        return {_srgb};
    });

    _attribFunctions["benchmark"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        if (args[0].asInt() > 0)
            _benchmark = true;
        else
            _benchmark = false;
        return true;
    });
}

} // end of namespace
