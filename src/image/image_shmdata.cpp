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
bool Image_Shmdata::read(const std::string& filename)
{
    _reader = std::make_unique<shmdata::Follower>(
        filename, [&](void* data, size_t size) { onData(data, size); }, [&](const std::string& caps) { onCaps(caps); }, [&]() {}, &_logger);

    return true;
}

/*************/
// Small function to work around a bug in GCC's libstdc++
void removeExtraParenthesis(std::string& str)
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
void Image_Shmdata::onCaps(const std::string& dataType)
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
        _isVideo = false;
        _isDepth = false;
        _isHap = false;
        _isYUV = false;
        _is420 = false;
        _is422 = false;

        std::regex regHap, regWidth, regHeight;
        std::regex regVideo, regFormat;
        try
        {
            regVideo = std::regex("(.*video/x-raw)(.*)", std::regex_constants::extended);
            regHap = std::regex("(.*video/x-gst-fourcc-HapY)(.*)", std::regex_constants::extended);
            regFormat = std::regex("(.*format=)(\\(string\\))?(.*)", std::regex_constants::extended);
            regWidth = std::regex("(.*width=)(\\(int\\))?(.*)", std::regex_constants::extended);
            regHeight = std::regex("(.*height=)(\\(int\\))?(.*)", std::regex_constants::extended);
        }
        catch (const std::regex_error& e)
        {
            auto errorString = e.what(); // string();
            Log::get() << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - Regex error: " << errorString << Log::endl;
            return;
        }

        std::smatch match;
        std::string substr, format;

        if (std::regex_match(dataType, regVideo))
        {

            if (std::regex_match(dataType, match, regFormat))
            {
                std::ssub_match subMatch = match[3];
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
                else if ("D" == substr)
                {
                    _bpp = 16;
                    _channels = 1;
                    _isDepth = true;
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
        else if (std::regex_match(dataType, regHap))
        {
            _isHap = true;
        }
        else
        {
            Log::get() << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - Incoming shmdata seems not to be of a supported video format" << Log::endl;
            return;
        }

        if (std::regex_match(dataType, match, regWidth))
        {
            std::ssub_match subMatch = match[3];
            substr = subMatch.str();
            removeExtraParenthesis(substr);
            substr = substr.substr(0, substr.find(","));
            _width = stoi(substr);
        }
        else
        {
            Log::get() << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - Incoming shmdata width needs to be specified" << Log::endl;
            return;
        }

        if (std::regex_match(dataType, match, regHeight))
        {
            std::ssub_match subMatch = match[3];
            substr = subMatch.str();
            removeExtraParenthesis(substr);
            substr = substr.substr(0, substr.find(","));
            _height = stoi(substr);
        }
        else
        {
            Log::get() << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - Incoming shmdata height needs to be specified" << Log::endl;
            return;
        }

        _isVideo = true;
        Log::get() << Log::MESSAGE << "Image_Shmdata::" << __FUNCTION__ << " - Connection successful" << Log::endl;
    }
}

/*************/
void Image_Shmdata::onData(void* data, int data_size)
{
    if (Timer::get().isDebug())
        Timer::get() << "image_shmdata " + _name;

    if (!_isVideo)
        return;

    // Standard images, RGB or YUV
    if (_width != 0 && _height != 0 && _bpp != 0 && _channels != 0)
        readUncompressedFrame(data, data_size);
    // Hap compressed images
    else if (_isHap == true)
        readHapFrame(data, data_size);

    if (Timer::get().isDebug())
        Timer::get() >> ("image_shmdata " + _name);
}

/*************/
void Image_Shmdata::readHapFrame(void* data, int data_size)
{
    // We are using kind of a hack to store a DXT compressed image in an ImageBuffer
    // First, we check the texture format type
    auto textureFormat = std::string("");
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

    {
        std::lock_guard<Spinlock> updateLock(_updateMutex);
        if (!_bufferImage)
            _bufferImage = std::make_unique<ImageBuffer>();
        std::swap(*(_bufferImage), _readerBuffer);
        _bufferImageUpdated = true;
    }
    updateTimestamp();
}

/*************/
void Image_Shmdata::readUncompressedFrame(void* data, int /*data_size*/)
{
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

        if (_isDepth)
        {
            spec.format = "R";
            spec.bpp = 16;
            spec.channels = 1;
        }
        else if (_is420 || _is422)
        {
            spec.format = "UYVY";
            spec.bpp = 16;
            spec.channels = 2;
        }

        _readerBuffer = ImageBuffer(spec);
    }

    if (!_isYUV && (_channels == 3 || _channels == 4))
    {
        char* pixels = (char*)(_readerBuffer).data();
        std::vector<std::future<void>> threads;
        for (uint32_t block = 0; block < _shmdataCopyThreads; ++block)
        {
            int size = _width * _height * _channels * sizeof(char);
            threads.push_back(std::async(std::launch::async, [=]() {
                int sizeOfBlock; // We compute the size of the block, to handle image size non divisible by _shmdataCopyThreads
                if (size - size / _shmdataCopyThreads * block < 2 * size / _shmdataCopyThreads)
                    sizeOfBlock = size - size / _shmdataCopyThreads * block;
                else
                    sizeOfBlock = size / _shmdataCopyThreads;

                memcpy(pixels + size / _shmdataCopyThreads * block, (const char*)data + size / _shmdataCopyThreads * block, sizeOfBlock);
            }));
        }
    }
    else if (_isDepth)
    {
        auto pixels = reinterpret_cast<char*>(_readerBuffer.data());
        memcpy(pixels, data, _readerBuffer.getSpec().rawSize());
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
        std::copy(YUV, YUV + _width * _height * 2, pixels);
    }
    else
        return;

    {
        std::lock_guard<Spinlock> updateLock(_updateMutex);
        if (!_bufferImage)
            _bufferImage = std::make_unique<ImageBuffer>();
        std::swap(*(_bufferImage), _readerBuffer);
        _bufferImageUpdated = true;
    }
    updateTimestamp();
}

/*************/
void Image_Shmdata::registerAttributes()
{
    Image::registerAttributes();
}
} // namespace Splash
