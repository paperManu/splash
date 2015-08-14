#include "image_shmdata.h"

#include <regex>
#include <hap.h>

#ifdef HAVE_SSE2
    #define GLM_FORCE_SSE2
    #define GLM_FORCE_INLINE
    #include <glm/glm.hpp>
    #include <glm/gtx/simd_vec4.hpp>
#else
    #define GLM_FORCE_INLINE
    #include <glm/glm.hpp>
#endif

#include "cgUtils.h"
#include "log.h"
#include "osUtils.h"
#include "timer.h"
#include "threadpool.h"

#define SPLASH_SHMDATA_THREADS 16

using namespace std;

namespace Splash
{

/*************/
Image_Shmdata::Image_Shmdata()
{
    _type = "image_shmdata";

    registerAttributes();
}

/*************/
Image_Shmdata::~Image_Shmdata()
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Image_Shmdata::~Image_Shmdata - Destructor" << Log::endl;
#endif
}

/*************/
bool Image_Shmdata::read(const string& filename)
{
    auto filepath = string(filename);
    if (Utils::getPathFromFilePath(filepath) == "" || filepath.find(".") == 0)
        filepath = _configFilePath + filepath;

    _reader.reset(new shmdata::Follower(filepath,
                                        [&](void* data, size_t size) {
                                            onData(data, size, this);
                                        },
                                        [&](const string& caps) {
                                            onCaps(caps, this);
                                        },
                                        [&](){},
                                        &_logger));
    _filepath = filename;

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
void Image_Shmdata::onCaps(const string& dataType, void* user_data)
{
    Image_Shmdata* ctx = reinterpret_cast<Image_Shmdata*>(user_data);

    if (dataType != ctx->_inputDataType)
    {
        ctx->_inputDataType = dataType;

        ctx->_bpp = 0;
        ctx->_width = 0;
        ctx->_height = 0;
        ctx->_red = 0;
        ctx->_green = 0;
        ctx->_blue = 0;
        ctx->_channels = 0;
        ctx->_isHap = false;
        ctx->_isYUV = false;
        ctx->_is420 = false;
        ctx->_is422 = false;
        
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
            auto errorString = string();
            switch (e.code())
            {
                default:
                    errorString = "unknown error";
                    break;
                case regex_constants::error_collate:
                    errorString = "the expression contains an invalid collating element name";
                    break;
                case regex_constants::error_ctype:
                    errorString = "the expression contains an invalid character class name";
                    break;
                case regex_constants::error_escape:
                    errorString = "the expression contains an invalid escaped character or a trailing escape";
                    break;
                case regex_constants::error_backref:
                    errorString = "the expression contains an invalid back reference";
                    break;
                case regex_constants::error_brack:
                    errorString = "the expression contains mismatched square brackets ('[' and ']')";
                    break;
                case regex_constants::error_paren:
                    errorString = "the expression contains mismatched parentheses ('(' and ')')";
                    break;
                case regex_constants::error_brace:
                    errorString = "the expression contains mismatched curly braces ('{' and '}')";
                    break;
                case regex_constants::error_badbrace:
                    errorString = "the expression contains an invalid range in a {} expression";
                    break;
                case regex_constants::error_range:
                    errorString = "the expression contains an invalid character range (e.g. [b-a])";
                    break;
                case regex_constants::error_space:
                    errorString = "there was not enough m2emory to convert the expression into a finite state machine";
                    break;
                case regex_constants::error_badrepeat:
                    errorString = "one of *?+{ was not preceded by a valid regular expression";
                    break;
                case regex_constants::error_complexity:
                    errorString = "the complexity of an attempted match exceeded a predefined level";
                    break;
                case regex_constants::error_stack:
                    errorString = "there was not enough memory to perform a match";
                    break;
            }
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

                if ("RGB" == substr)
                {
                    ctx->_bpp = 24;
                    ctx->_channels = 3;
                    ctx->_red = 0;
                    ctx->_green = 1;
                    ctx->_blue = 2;
                }
                else if ("BGR" == substr)
                {
                    ctx->_bpp = 24;
                    ctx->_channels = 3;
                    ctx->_red = 2;
                    ctx->_green = 1;
                    ctx->_blue = 0;
                }
                else if ("RGBA" == substr)
                {
                    ctx->_bpp = 32;
                    ctx->_channels = 4;
                    ctx->_red = 2;
                    ctx->_green = 1;
                    ctx->_blue = 0;
                }
                else if ("BGRA" == substr)
                {
                    ctx->_bpp = 32;
                    ctx->_channels = 4;
                    ctx->_red = 0;
                    ctx->_green = 1;
                    ctx->_blue = 2;
                }
                else if ("I420" == substr)
                {
                    ctx->_bpp = 12;
                    ctx->_channels = 3;
                    ctx->_isYUV = true;
                    ctx->_is420 = true;
                }
                else if ("UYVY" == substr)
                {
                    ctx->_bpp = 12;
                    ctx->_channels = 3;
                    ctx->_isYUV = true;
                    ctx->_is422 = true;
                }
            }
        }
        else if (regex_match(dataType, regHap))
        {
            ctx->_isHap = true;
        }

        if (regex_match(dataType, match, regWidth))
        {
            ssub_match subMatch = match[2];
            substr = subMatch.str();
            removeExtraParenthesis(substr);
            substr = substr.substr(0, substr.find(","));
            ctx->_width = stoi(substr);
        }

        if (regex_match(dataType, match, regHeight))
        {
            ssub_match subMatch = match[2];
            substr = subMatch.str();
            removeExtraParenthesis(substr);
            substr = substr.substr(0, substr.find(","));
            ctx->_height = stoi(substr);
        }
    }
}

/*************/
void Image_Shmdata::onData(void* data, int data_size, void* user_data)
{
    Image_Shmdata* ctx = reinterpret_cast<Image_Shmdata*>(user_data);

    if (Timer::get().isDebug())
    {
        Timer::get().sinceLastSeen("image_shmdata_period " + ctx->_name);
        Timer::get() << "image_shmdata " + ctx->_name;
    }

    // Standard images, RGB or YUV
    if (ctx->_width != 0 && ctx->_height != 0 && ctx->_bpp != 0 && ctx->_channels != 0)
    {
        readUncompressedFrame(ctx, data, data_size);
    }
    // Hap compressed images
    else if (ctx->_isHap == true)
    {
        readHapFrame(ctx, data, data_size);
    }

    if (Timer::get().isDebug())
        Timer::get() >> "image_shmdata " + ctx->_name;
}

/*************/
void Image_Shmdata::readHapFrame(Image_Shmdata* ctx, void* data, int data_size)
{
    unique_lock<mutex> lock(ctx->_writeMutex);

    // We are using kind of a hack to store a DXT compressed image in an oiio::ImageBuf
    // First, we check the texture format type
    auto textureFormat = string("");
    if (!hapDecodeFrame(data, data_size, nullptr, 0, textureFormat))
        return;

    // Check if we need to resize the reader buffer
    // We set the size so as to have just enough place for the given texture format
    oiio::ImageSpec bufSpec = ctx->_readerBuffer.spec();
    if (bufSpec.width != ctx->_width || (bufSpec.height != ctx->_height && textureFormat != ctx->_textureFormat))
    {
        ctx->_textureFormat = textureFormat;

        oiio::ImageSpec spec;
        if (textureFormat == "RGB_DXT1")
            spec = oiio::ImageSpec(ctx->_width, (int)(ceil((float)ctx->_height / 2.f)), 1, oiio::TypeDesc::UINT8);
        else if (textureFormat == "RGBA_DXT5")
            spec = oiio::ImageSpec(ctx->_width, ctx->_height, 1, oiio::TypeDesc::UINT8);
        else if (textureFormat == "YCoCg_DXT5")
            spec = oiio::ImageSpec(ctx->_width, ctx->_height, 1, oiio::TypeDesc::UINT8);
        else
            return;

        spec.channelnames = {textureFormat};
        ctx->_readerBuffer.reset(spec);
    }

    unsigned long outputBufferBytes = bufSpec.width * bufSpec.height * bufSpec.nchannels;
    if (!hapDecodeFrame(data, data_size, ctx->_readerBuffer.localpixels(), outputBufferBytes, textureFormat))
        return;
    
    ctx->_bufferImage.swap(ctx->_readerBuffer);
    ctx->_imageUpdated = true;
    ctx->updateTimestamp();
}

/*************/
void Image_Shmdata::readUncompressedFrame(Image_Shmdata* ctx, void* data, int data_size)
{
    unique_lock<mutex> lock(ctx->_writeMutex);

    // Check if we need to resize the reader buffer
    oiio::ImageSpec bufSpec = ctx->_readerBuffer.spec();
    if (bufSpec.width != ctx->_width || bufSpec.height != ctx->_height || bufSpec.nchannels != ctx->_channels)
    {
        oiio::ImageSpec spec(ctx->_width, ctx->_height, ctx->_channels, oiio::TypeDesc::UINT8);
        if (ctx->_green < ctx->_blue)
            spec.channelnames = {"B", "G", "R"};
        else
            spec.channelnames = {"R", "G", "B"};
        if (ctx->_channels == 4)
            spec.channelnames.push_back("A");

        ctx->_readerBuffer.reset(spec);
    }

    if (!ctx->_isYUV && (ctx->_channels == 3 || ctx->_channels == 4))
    {
        char* pixels = (char*)(ctx->_readerBuffer).localpixels();
        vector<unsigned int> threadIds;
        for (int block = 0; block < SPLASH_SHMDATA_THREADS; ++block)
        {
            int size = ctx->_width * ctx->_height * ctx->_channels * sizeof(char);
            threadIds.push_back(SThread::pool.enqueue([=, &ctx]() {
                int sizeOfBlock; // We compute the size of the block, to handle image size non divisible by SPLASH_SHMDATA_THREADS
                if (size - size / SPLASH_SHMDATA_THREADS * block < 2 * size / SPLASH_SHMDATA_THREADS)
                    sizeOfBlock = size - size / SPLASH_SHMDATA_THREADS * block;
                else
                    sizeOfBlock = size / SPLASH_SHMDATA_THREADS;
                    
                memcpy(pixels + size / SPLASH_SHMDATA_THREADS * block, (const char*)data + size / SPLASH_SHMDATA_THREADS * block, sizeOfBlock);
            }));
        }
        SThread::pool.waitThreads(threadIds);
    }
    else if (ctx->_is420)
    {
        const unsigned char* Y = (const unsigned char*)data;
        const unsigned char* U = (const unsigned char*)data + ctx->_width * ctx->_height;
        const unsigned char* V = (const unsigned char*)data + ctx->_width * ctx->_height * 5 / 4;

        char* pixels = (char*)(ctx->_readerBuffer).localpixels();
        vector<unsigned int> threadIds;
#ifdef HAVE_SSE2
        for (int block = 0; block < SPLASH_SHMDATA_THREADS; ++block)
        {
            int width = ctx->_width;
            int height = ctx->_height;

            threadIds.push_back(SThread::pool.enqueue([=, &ctx]() {
                int lastLine; // We compute the last line, to handle image size non divisible by SPLASH_SHMDATA_THREADS
                if (ctx->_height - ctx->_height / SPLASH_SHMDATA_THREADS * (block + 1) < ctx->_height / SPLASH_SHMDATA_THREADS)
                    lastLine = ctx->_height;
                else
                    lastLine = ctx->_height / SPLASH_SHMDATA_THREADS * (block + 1);

                auto uLine = vector<unsigned char>(width / 2);
                auto vLine = vector<unsigned char>(width / 2);
                auto yLine = vector<unsigned char>(width);
                auto localPixels = vector<unsigned char>(width * 3);
                for (int y = height / SPLASH_SHMDATA_THREADS * block; y < lastLine; ++y)
                {
                    memcpy(uLine.data(), &U[(y / 2) * (width / 2)], width / 2 * sizeof(unsigned char));
                    memcpy(vLine.data(), &V[(y / 2) * (width / 2)], width / 2 * sizeof(unsigned char));
                    memcpy(yLine.data(), &Y[y * width], width * sizeof(unsigned char));

                    for (int x = 0; x < width; x += 4)
                    {
                        const unsigned char* uPtr = &uLine[x / 2];
                        const unsigned char* vPtr = &vLine[x / 2];
                        const unsigned char* yPtr = &yLine[x];

                        auto uValue = glm::detail::fvec4SIMD((float)uPtr[0], (float)uPtr[0], (float)uPtr[1], (float)uPtr[1]);
                        auto vValue = glm::detail::fvec4SIMD((float)vPtr[0], (float)vPtr[0], (float)vPtr[1], (float)vPtr[1]);
                        auto yValue = glm::detail::fvec4SIMD((float)yPtr[0], (float)yPtr[1], (float)yPtr[2], (float)yPtr[3]);

                        uValue = uValue - 128.0;
                        vValue = vValue - 128.0;

                        yValue = (yValue - 16.0) * 38142.0;
                        auto rPart = glm::clamp((yValue + vValue * 52289) / 32768.0, 0.0, 255.0);
                        auto gPart = glm::clamp((yValue + uValue * -12846 - vValue * 36641) / 32768.0, 0.0, 255.0);
                        auto bPart = glm::clamp((yValue + uValue * 66094) / 32768.0, 0.0, 255.0);

                        auto rPixel = glm::vec4_cast(rPart);
                        auto gPixel = glm::vec4_cast(gPart);
                        auto bPixel = glm::vec4_cast(bPart);

                        for (int i = 0; i < 4; ++i)
                        {
                            localPixels[(x + i) * 3] = (unsigned char)rPixel[i];
                            localPixels[(x + i) * 3 + 1] = (unsigned char)gPixel[i];
                            localPixels[(x + i) * 3 + 2] = (unsigned char)bPixel[i];
                        }
                    }

                    memcpy(&pixels[y * width * 3], localPixels.data(), width * 3 * sizeof(unsigned char));
                }
            }));
        }
#else
        for (int block = 0; block < SPLASH_SHMDATA_THREADS; ++block)
        {
            threadIds.push_back(SThread::pool.enqueue([=, &ctx]() {
                int lastLine; // We compute the last line, to handle image size non divisible by SPLASH_SHMDATA_THREADS
                if (ctx->_height - ctx->_height / SPLASH_SHMDATA_THREADS * (block + 1) < ctx->_height / SPLASH_SHMDATA_THREADS)
                    lastLine = ctx->_height;
                else
                    lastLine = ctx->_height / SPLASH_SHMDATA_THREADS * (block + 1);

                for (int y = ctx->_height / SPLASH_SHMDATA_THREADS * block; y < lastLine; y++)
                    for (int x = 0; x < ctx->_width; x+=2)
                    {
                        int uValue = (int)(U[(y / 2) * (ctx->_width / 2) + x / 2]) - 128;
                        int vValue = (int)(V[(y / 2) * (ctx->_width / 2) + x / 2]) - 128;

                        int rPart = 52298 * vValue;
                        int gPart = -12846 * uValue - 36641 * vValue;
                        int bPart = 66094 * uValue;
                       
                        int col = x;
                        int row = y;
                        int yValue = (int)(Y[row * ctx->_width + col] - 16) * 38142;
                        pixels[(row * ctx->_width + col) * 3] = (unsigned char)clamp((yValue + rPart) / 32768, 0, 255);
                        pixels[(row * ctx->_width + col) * 3 + 1] = (unsigned char)clamp((yValue + gPart) / 32768, 0, 255);
                        pixels[(row * ctx->_width + col) * 3 + 2] = (unsigned char)clamp((yValue + bPart) / 32768, 0, 255);

                        col++;
                        yValue = (int)(Y[row * ctx->_width + col] - 16) * 38142;
                        pixels[(row * ctx->_width + col) * 3] = (unsigned char)clamp((yValue + rPart) / 32768, 0, 255);
                        pixels[(row * ctx->_width + col) * 3 + 1] = (unsigned char)clamp((yValue + gPart) / 32768, 0, 255);
                        pixels[(row * ctx->_width + col) * 3 + 2] = (unsigned char)clamp((yValue + bPart) / 32768, 0, 255);
                    }
            }));
        }
#endif
        SThread::pool.waitThreads(threadIds);
    }
    else if (ctx->_is422)
    {
        const unsigned char* YUV = (const unsigned char*)data;

        char* pixels = (char*)(ctx->_readerBuffer).localpixels();
        vector<unsigned int> threadIds;
#ifdef HAVE_SSE2
        for (int block = 0; block < SPLASH_SHMDATA_THREADS; ++block)
        {
            int width = ctx->_width;
            int height = ctx->_height;

            threadIds.push_back(SThread::pool.enqueue([=, &ctx]() {
                int lastLine; // We compute the last line, to handle image size non divisible by SPLASH_SHMDATA_THREADS
                if (ctx->_height - ctx->_height / SPLASH_SHMDATA_THREADS * (block + 1) < ctx->_height / SPLASH_SHMDATA_THREADS)
                    lastLine = ctx->_height;
                else
                    lastLine = ctx->_height / SPLASH_SHMDATA_THREADS * (block + 1);

                auto line = vector<unsigned char>(width * 2);
                auto localPixels = vector<unsigned char>(width * 3);

                for (int y = height / SPLASH_SHMDATA_THREADS * block; y < lastLine; ++y)
                {
                    memcpy(line.data(), &YUV[y * width * 2], width * 2 * sizeof(unsigned char));

                    for (int x = 0; x < width; x += 4)
                    {
                        const unsigned char* block = &line[x * 2];

                        auto uValue = glm::detail::fvec4SIMD((float)block[0], (float)block[0], (float)block[4], (float)block[4]);
                        auto vValue = glm::detail::fvec4SIMD((float)block[2], (float)block[2], (float)block[6], (float)block[6]);
                        auto yValue = glm::detail::fvec4SIMD((float)block[1], (float)block[3], (float)block[5], (float)block[7]);

                        uValue = uValue - 128.0;
                        vValue = vValue - 128.0;

                        yValue = (yValue - 16.0) * 38142.0;
                        auto rPart = glm::clamp((yValue + vValue * 52289) / 32768.0, 0.0, 255.0);
                        auto gPart = glm::clamp((yValue + uValue * -12846 - vValue * 36641) / 32768.0, 0.0, 255.0);
                        auto bPart = glm::clamp((yValue + uValue * 66094) / 32768.0, 0.0, 255.0);

                        auto rPixel = glm::vec4_cast(rPart);
                        auto gPixel = glm::vec4_cast(gPart);
                        auto bPixel = glm::vec4_cast(bPart);

                        for (int i = 0; i < 4; ++i)
                        {
                            localPixels[(x + i) * 3] = (unsigned char)rPixel[i];
                            localPixels[(x + i) * 3 + 1] = (unsigned char)gPixel[i];
                            localPixels[(x + i) * 3 + 2] = (unsigned char)bPixel[i];
                        }
                    }

                    memcpy(&pixels[y * width * 3], localPixels.data(), width * 3 * sizeof(unsigned char));
                }
            }));
        }
#else
        for (int block = 0; block < SPLASH_SHMDATA_THREADS; ++block)
        {
            threadIds.push_back(SThread::pool.enqueue([=, &ctx]() {
                int lastLine; // We compute the last line, to handle image size non divisible by SPLASH_SHMDATA_THREADS
                if (ctx->_height - ctx->_height / SPLASH_SHMDATA_THREADS * (block + 1) < ctx->_height / SPLASH_SHMDATA_THREADS)
                    lastLine = ctx->_height;
                else
                    lastLine = ctx->_height / SPLASH_SHMDATA_THREADS * (block + 1);

                for (int y = ctx->_height / SPLASH_SHMDATA_THREADS * block; y < lastLine; y++)
                    for (int x = 0; x < ctx->_width; x+=2)
                    {
                        unsigned char block[4];
                        memcpy(block, &YUV[y * ctx->_width * 2 + x * 2], 4 * sizeof(unsigned char));

                        int uValue = (int)(block[0]) - 128;
                        int vValue = (int)(block[2]) - 128;

                        int rPart = 52298 * vValue;
                        int gPart = -12846 * uValue - 36641 * vValue;
                        int bPart = 66094 * uValue;
                       
                        int yValue = (int)(block[1] - 16) * 38142;
                        pixels[(y * ctx->_width + x) * 3] = (unsigned char)clamp((yValue + rPart) / 32768, 0, 255);
                        pixels[(y * ctx->_width + x) * 3 + 1] = (unsigned char)clamp((yValue + gPart) / 32768, 0, 255);
                        pixels[(y * ctx->_width + x) * 3 + 2] = (unsigned char)clamp((yValue + bPart) / 32768, 0, 255);

                        yValue = (int)(block[3] - 16) * 38142;
                        pixels[(y * ctx->_width + x + 1) * 3] = (unsigned char)clamp((yValue + rPart) / 32768, 0, 255);
                        pixels[(y * ctx->_width + x + 1) * 3 + 1] = (unsigned char)clamp((yValue + gPart) / 32768, 0, 255);
                        pixels[(y * ctx->_width + x + 1) * 3 + 2] = (unsigned char)clamp((yValue + bPart) / 32768, 0, 255);
                    }
            }));
        }
#endif
        SThread::pool.waitThreads(threadIds);
    }
    else
        return;

    ctx->_bufferImage.swap(ctx->_readerBuffer);
    ctx->_imageUpdated = true;
    ctx->updateTimestamp();

}

/*************/
void Image_Shmdata::registerAttributes()
{
}

}
