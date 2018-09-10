#include "./image/image_shmdata.h"

#include <hap.h>
#include <regex>

// All existing 64bits x86 CPUs have SSE2
#if __x86_64__
#define GLM_FORCE_SSE2
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#else
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#endif

#include "./utils/cgutils.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

#define SPLASH_SHMDATA_THREADS 2
#define SPLASH_SHMDATA_WITH_POOL 0 // FIXME: there is an issue with the threadpool in the shmdata callback

using namespace std;

namespace Splash
{

/*************/
Image_Shmdata::Image_Shmdata(RootObject* root)
    : Image(root)
{
    init();
}

/*************/
Image_Shmdata::~Image_Shmdata()
{
    _reader.reset();
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Image_Shmdata::~Image_Shmdata - Destructor" << Log::endl;
#endif
}

/*************/
bool Image_Shmdata::read(const string& filename)
{
    _reader = make_unique<shmdata::Follower>(filename, [&](void* data, size_t size) { onData(data, size); }, [&](const string& caps) { onCaps(caps); }, [&]() {}, &_logger);

    return true;
}

/*************/
// Small function to work around a bug in GCC's libstdc++
void removeExtraParenthesis(string& str)
{
    if (str.find(")") == 0)
        str = str.substr(1);
}

/*************/
void Image_Shmdata::init()
{
    _type = "image_shmdata";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;
}

/*************/
void Image_Shmdata::onCaps(const string& dataType)
{
    Log::get() << Log::MESSAGE << "Image_Shmdata::" << __FUNCTION__ << " - Trying to connect with the following caps: " << dataType << Log::endl;

    if (dataType != _inputDataType)
    {
        _inputDataType = dataType;

        _bpp = 0;
        _width = 0;
        _height = 0;
        _red = 0;
        _green = 0;
        _blue = 0;
        _channels = 0;
        _isHap = false;
        _isYUV = false;
        _is420 = false;
        _is422 = false;

        regex regHap, regWidth, regHeight;
        regex regVideo, regFormat;
        try
        {
            regVideo = regex("(.*video/x-raw)(.*)", regex_constants::extended);
            regHap = regex("(.*video/x-gst-fourcc-HapY)(.*)", regex_constants::extended);
            regFormat = regex("(.*format=\\(string\\))(.*)", regex_constants::extended);
            regWidth = regex("(.*width=\\(int\\))(.*)", regex_constants::extended);
            regHeight = regex("(.*height=\\(int\\))(.*)", regex_constants::extended);
        }
        catch (const regex_error& e)
        {
            auto errorString = e.what(); // string();
            Log::get() << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - Regex error: " << errorString << Log::endl;
            return;
        }

        smatch match;
        string substr, format;

        if (regex_match(dataType, regVideo))
        {

            if (regex_match(dataType, match, regFormat))
            {
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                removeExtraParenthesis(substr);
                substr = substr.substr(0, substr.find(","));

                if ("BGR" == substr)
                {
                    _bpp = 24;
                    _channels = 3;
                    _red = 2;
                    _green = 1;
                    _blue = 0;
                }
                else if ("RGB" == substr)
                {
                    _bpp = 24;
                    _channels = 3;
                    _red = 0;
                    _green = 1;
                    _blue = 2;
                }
                else if ("RGBA" == substr)
                {
                    _bpp = 32;
                    _channels = 4;
                    _red = 2;
                    _green = 1;
                    _blue = 0;
                }
                else if ("BGRA" == substr)
                {
                    _bpp = 32;
                    _channels = 4;
                    _red = 0;
                    _green = 1;
                    _blue = 2;
                }
                else if ("I420" == substr)
                {
                    _bpp = 12;
                    _channels = 3;
                    _isYUV = true;
                    _is420 = true;
                }
                else if ("UYVY" == substr)
                {
                    _bpp = 12;
                    _channels = 3;
                    _isYUV = true;
                    _is422 = true;
                }
            }
        }
        else if (regex_match(dataType, regHap))
        {
            _isHap = true;
        }

        if (regex_match(dataType, match, regWidth))
        {
            ssub_match subMatch = match[2];
            substr = subMatch.str();
            removeExtraParenthesis(substr);
            substr = substr.substr(0, substr.find(","));
            _width = stoi(substr);
        }

        if (regex_match(dataType, match, regHeight))
        {
            ssub_match subMatch = match[2];
            substr = subMatch.str();
            removeExtraParenthesis(substr);
            substr = substr.substr(0, substr.find(","));
            _height = stoi(substr);
        }

        Log::get() << Log::MESSAGE << "Image_Shmdata::" << __FUNCTION__ << " - Connection successful" << Log::endl;
    }
}

/*************/
void Image_Shmdata::onData(void* data, int data_size)
{
    if (Timer::get().isDebug())
    {
        Timer::get() << "image_shmdata " + _name;
    }

    // Standard images, RGB or YUV
    if (_width != 0 && _height != 0 && _bpp != 0 && _channels != 0)
    {
        readUncompressedFrame(data, data_size);
    }
    // Hap compressed images
    else if (_isHap == true)
    {
        readHapFrame(data, data_size);
    }

    if (Timer::get().isDebug())
        Timer::get() >> ("image_shmdata " + _name);
}

/*************/
void Image_Shmdata::readHapFrame(void* data, int data_size)
{
    lock_guard<shared_timed_mutex> lock(_writeMutex);

    // We are using kind of a hack to store a DXT compressed image in an ImageBuffer
    // First, we check the texture format type
    auto textureFormat = string("");
    if (!hapDecodeFrame(data, data_size, nullptr, 0, textureFormat))
        return;

    // Check if we need to resize the reader buffer
    // We set the size so as to have just enough place for the given texture format
    auto bufSpec = _readerBuffer.getSpec();
    if (bufSpec.width != _width || (bufSpec.height != _height && textureFormat != _textureFormat))
    {
        _textureFormat = textureFormat;

        ImageBufferSpec spec;
        if (textureFormat == "RGB_DXT1")
            spec = ImageBufferSpec(_width, (int)(ceil((float)_height / 2.f)), 1, 8, ImageBufferSpec::Type::UINT8);
        else if (textureFormat == "RGBA_DXT5")
            spec = ImageBufferSpec(_width, _height, 1, 8, ImageBufferSpec::Type::UINT8);
        else if (textureFormat == "YCoCg_DXT5")
            spec = ImageBufferSpec(_width, _height, 1, 8, ImageBufferSpec::Type::UINT8);
        else
            return;

        spec.format = textureFormat;
        _readerBuffer = ImageBuffer(spec);
    }

    unsigned long outputBufferBytes = bufSpec.width * bufSpec.height * bufSpec.channels;
    if (!hapDecodeFrame(data, data_size, _readerBuffer.data(), outputBufferBytes, textureFormat))
        return;

    if (!_bufferImage)
        _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer());
    std::swap(*(_bufferImage), _readerBuffer);
    _imageUpdated = true;
    updateTimestamp();
}

/*************/
void Image_Shmdata::readUncompressedFrame(void* data, int /*data_size*/)
{
    lock_guard<shared_timed_mutex> lock(_writeMutex);

    // Check if we need to resize the reader buffer
    auto bufSpec = _readerBuffer.getSpec();
    if (bufSpec.width != _width || bufSpec.height != _height || bufSpec.channels != _channels)
    {
        ImageBufferSpec spec(_width, _height, _channels, 8 * _channels, ImageBufferSpec::Type::UINT8);
        if (_green < _blue)
            spec.format = "BGR";
        else
            spec.format = "RGB";
        if (_channels == 4)
            spec.format.push_back('A');

        if (_is420 || _is422)
        {
            spec.format = "UYVY";
            spec.bpp = 16;
        }

        _readerBuffer = ImageBuffer(spec);
    }

    if (!_isYUV && (_channels == 3 || _channels == 4))
    {
        char* pixels = (char*)(_readerBuffer).data();
        vector<future<void>> threads;
        for (int block = 0; block < SPLASH_SHMDATA_THREADS; ++block)
        {
            int size = _width * _height * _channels * sizeof(char);
            threads.push_back(async(launch::async, [=]() {
                int sizeOfBlock; // We compute the size of the block, to handle image size non divisible by SPLASH_SHMDATA_THREADS
                if (size - size / SPLASH_SHMDATA_THREADS * block < 2 * size / SPLASH_SHMDATA_THREADS)
                    sizeOfBlock = size - size / SPLASH_SHMDATA_THREADS * block;
                else
                    sizeOfBlock = size / SPLASH_SHMDATA_THREADS;

                memcpy(pixels + size / SPLASH_SHMDATA_THREADS * block, (const char*)data + size / SPLASH_SHMDATA_THREADS * block, sizeOfBlock);
            }));
        }
    }
    else if (_is420)
    {
        const unsigned char* Y = static_cast<const unsigned char*>(data);
        const unsigned char* U = static_cast<const unsigned char*>(data) + _width * _height;
        const unsigned char* V = static_cast<const unsigned char*>(data) + _width * _height * 5 / 4;
        char* pixels = (char*)(_readerBuffer).data();

        for (uint32_t y = 0; y < _height; ++y)
        {
            for (uint32_t x = 0; x < _width; x += 2)
            {
                pixels[(x + y * _width) * 2 + 0] = U[(x / 2) + (y / 2) * (_width / 2)];
                pixels[(x + y * _width) * 2 + 1] = Y[x + y * _width];
                pixels[(x + y * _width) * 2 + 2] = V[(x / 2) + (y / 2) * (_width / 2)];
                pixels[(x + y * _width) * 2 + 3] = Y[x + y * _width + 1];
            }
        }
    }
    else if (_is422)
    {
        const unsigned char* YUV = static_cast<const unsigned char*>(data);
        char* pixels = (char*)(_readerBuffer).data();
        copy(YUV, YUV + _width * _height * 2, pixels);
    }
    else
        return;

    if (!_bufferImage)
        _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer());
    std::swap(*(_bufferImage), _readerBuffer);
    _imageUpdated = true;
    updateTimestamp();
}

/*************/
void Image_Shmdata::registerAttributes()
{
    Image::registerAttributes();
}
}
