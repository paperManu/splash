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
    if (_reader != nullptr)
        shmdata_any_reader_close(_reader);
    if (_writer != nullptr)
        shmdata_any_writer_close(_writer);

#ifdef DEBUG
    SLog::log << Log::DEBUGGING << "Image_Shmdata::~Image_Shmdata - Destructor" << Log::endl;
#endif
}

/*************/
bool Image_Shmdata::read(const string& filename)
{
    auto filepath = string(filename);
    if (Utils::getPathFromFilePath(filepath) == "" || filepath.find(".") == 0)
        filepath = _configFilePath + filepath;

    if (_reader != nullptr)
        shmdata_any_reader_close(_reader);

    _reader = shmdata_any_reader_init();
    shmdata_any_reader_run_gmainloop(_reader, SHMDATA_TRUE);
    shmdata_any_reader_set_on_data_handler(_reader, Image_Shmdata::onData, this);
    shmdata_any_reader_start(_reader, filepath.c_str());
    _filename = filename;

    return true;
}

/*************/
bool Image_Shmdata::write(const oiio::ImageBuf& img, const string& filename)
{
    if (img.localpixels() == NULL)
        return false;

    lock_guard<mutex> lock(_readMutex);
    oiio::ImageSpec spec = img.spec();
    if (spec.width != _writerSpec.width || spec.height != _writerSpec.height || spec.nchannels != _writerSpec.nchannels || _writer == NULL || _filename != filename)
        if (!initShmWriter(spec, filename))
            return false;

    memcpy(_writerBuffer.localpixels(), img.localpixels(), _writerInputSize);
    unsigned long long currentTime = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
    shmdata_any_writer_push_data(_writer, (void*)_writerBuffer.localpixels(), _writerInputSize, (currentTime - _writerStartTime) * 1e6, NULL, NULL);
    return true;
}

/*************/
bool Image_Shmdata::initShmWriter(const oiio::ImageSpec& spec, const string& filename)
{
    if (_writer != NULL)
        shmdata_any_writer_close(_writer);

    _writer = shmdata_any_writer_init();
    
    string dataType;
    if (spec.format == "uint8" && spec.nchannels == 4)
    {
        dataType += "video/x-raw-rgb,bpp=32,endianness=4321,depth=32,red_mask=-16777216,green_mask=16711680,blue_mask=65280,";
        _writerInputSize = 4;
    }
    else if (spec.format == "uint16" && spec.nchannels == 1)
    {
        dataType += "video/x-raw-gray,bpp=16,endianness=4321,depth=16,";
        _writerInputSize = 2;
    }
    else
    {
        _writerInputSize = 0;
        return false;
    }

    dataType += "width=" + to_string(spec.width) + ",";
    dataType += "height=" + to_string(spec.height) + ",";
    dataType += "framerate=60/1";
    _writerInputSize *= spec.width * spec.height;

    shmdata_any_writer_set_data_type(_writer, dataType.c_str());
    if (!shmdata_any_writer_set_path(_writer, filename.c_str()))
    {
        SLog::log << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - Unable to write to shared memory " << filename << Log::endl;
        _filename = "";
        return false;
    }

    _filename = filename;
    _writerSpec = spec;
    shmdata_any_writer_start(_writer);
    _writerStartTime = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();

    _writerBuffer.reset(_writerSpec);

    return true;
}

/*************/
void Image_Shmdata::onData(shmdata_any_reader_t* reader, void* shmbuf, void* data, int data_size, unsigned long long timestamp,
    const char* type_description, void* user_data)
{
    Image_Shmdata* ctx = reinterpret_cast<Image_Shmdata*>(user_data);

    STimer::timer.sinceLastSeen("image_shmdata_period " + ctx->_name);
    STimer::timer << "image_shmdata " + ctx->_name;

    string dataType(type_description);
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
        
        regex regRgb, regGray, regYUV, regHap, regBpp, regWidth, regHeight, regRed, regGreen, regBlue, regFormatYUV;
        try
        {
            // TODO: replace these regex with some GCC 4.7 compatible ones
            // GCC 4.6 does not support the full regular expression. Some work around is needed,
            // this is why this may seem complicated for nothing ...
            regRgb = regex("(video/x-raw-rgb)(.*)", regex_constants::extended);
            regYUV = regex("(video/x-raw-yuv)(.*)", regex_constants::extended);
            regHap = regex("(video/x-gst-fourcc-Hap)(.*)", regex_constants::extended);
            regFormatYUV = regex("(.*format=\\(fourcc\\))(.*)", regex_constants::extended);
            regBpp = regex("(.*bpp=\\(int\\))(.*)", regex_constants::extended);
            regWidth = regex("(.*width=\\(int\\))(.*)", regex_constants::extended);
            regHeight = regex("(.*height=\\(int\\))(.*)", regex_constants::extended);
            regRed = regex("(.*red_mask=\\(int\\))(.*)", regex_constants::extended);
            regGreen = regex("(.*green_mask=\\(int\\))(.*)", regex_constants::extended);
            regBlue = regex("(.*blue_mask=\\(int\\))(.*)", regex_constants::extended);
        }
        catch (const regex_error& e)
        {
            SLog::log << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - Regex error code: " << e.code() << Log::endl;
            shmdata_any_reader_free(shmbuf);
            return;
        }

        if (regex_match(dataType, regRgb) || regex_match(dataType, regYUV))
        {

            smatch match;
            string substr, format;

            if (regex_match(dataType, match, regBpp))
            {
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                sscanf(substr.c_str(), ")%i", &ctx->_bpp);
            }
            if (regex_match(dataType, match, regWidth))
            {
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                sscanf(substr.c_str(), ")%i", &ctx->_width);
            }
            if (regex_match(dataType, match, regHeight))
            {
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                sscanf(substr.c_str(), ")%i", &ctx->_height);
            }
            if (regex_match(dataType, match, regRed))
            {
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                sscanf(substr.c_str(), ")%i", &ctx->_red);
            }
            else if (regex_match(dataType, regYUV))
            {
                ctx->_isYUV = true;
            }
            if (regex_match(dataType, match, regGreen))
            {
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                sscanf(substr.c_str(), ")%i", &ctx->_green);
            }
            if (regex_match(dataType, match, regBlue))
            {
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                sscanf(substr.c_str(), ")%i", &ctx->_blue);
            }

            if (ctx->_bpp == 24)
                ctx->_channels = 3;
            else if (ctx->_bpp == 32)
                ctx->_channels = 4;
            else if (ctx->_isYUV)
            {
                ctx->_bpp = 12;
                ctx->_channels = 3;

                if (regex_match(dataType, match, regFormatYUV))
                {
                    char format[16];
                    ssub_match subMatch = match[2];
                    substr = subMatch.str();
                    sscanf(substr.c_str(), ")%s", format);
                    if (strstr(format, (char*)"I420") != nullptr)
                        ctx->_is420 = true;
                    else if (strstr(format, (char*)"UYVY") != nullptr)
                        ctx->_is422 = true;
                }
            }
        }
        else if (regex_match(dataType, regHap))
        {
            ctx->_isHap = true;

            smatch match;
            string substr, format;
            if (regex_match(dataType, match, regWidth))
            {
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                sscanf(substr.c_str(), ")%i", &ctx->_width);
            }
            if (regex_match(dataType, match, regHeight))
            {
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                sscanf(substr.c_str(), ")%i", &ctx->_height);
            }
        }
    }

    /*********/
    // Standard images, RGB or YUV
    if (ctx->_width != 0 && ctx->_height != 0 && ctx->_bpp != 0 && ctx->_channels != 0)
    {
        readUncompressedFrame(ctx, shmbuf, data, data_size);
    }
    /*********/
    // Hap compressed images
    else if (ctx->_isHap == true)
    {
        readHapFrame(ctx, shmbuf, data, data_size);
    }

    shmdata_any_reader_free(shmbuf);

    STimer::timer >> "image_shmdata " + ctx->_name;
}

/*************/
void Image_Shmdata::readHapFrame(Image_Shmdata* ctx, void* shmbuf, void* data, int data_size)
{
    lock_guard<mutex> lock(ctx->_writeMutex);

    // We are using kind of a hack to store a DXT compressed image in an oiio::ImageBuf
    // First, we check the texture format type
    unsigned int textureFormat = 0;
    if (HapGetFrameTextureFormat(data, data_size, &textureFormat) != HapResult_No_Error)
    {
        SLog::log << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - Unknown texture format. Frame discarded" << Log::endl;
        return;
    }

    // Check if we need to resize the reader buffer
    // We set the size so as to have just enough place for the given texture format
    oiio::ImageSpec bufSpec = ctx->_readerBuffer.spec();
    if (bufSpec.width != ctx->_width || (bufSpec.height != ctx->_height && textureFormat != ctx->_textureFormat))
    {
        ctx->_textureFormat = textureFormat;

        oiio::ImageSpec spec;
        if (textureFormat == HapTextureFormat_RGB_DXT1)
        {
            spec = oiio::ImageSpec(ctx->_width, (int)(ceil((float)ctx->_height / 2.f)), 1, oiio::TypeDesc::UINT8);
            spec.channelnames = {"RGB_DXT1"};
        }
        else if (textureFormat == HapTextureFormat_RGBA_DXT5)
        {
            spec = oiio::ImageSpec(ctx->_width, ctx->_height, 1, oiio::TypeDesc::UINT8);
            spec.channelnames = {"RGBA_DXT5"};
        }
        else if (textureFormat == HapTextureFormat_YCoCg_DXT5)
        {
            spec = oiio::ImageSpec(ctx->_width, ctx->_height, 1, oiio::TypeDesc::UINT8);
            spec.channelnames = {"YCoCg_DXT5"};
        }
        else
            return;

        ctx->_readerBuffer.reset(spec);
    }

    unsigned long outputBufferBytes = bufSpec.width * bufSpec.height * bufSpec.nchannels;
    unsigned long bytesUsed = 0;
    if (HapDecode(data, data_size, NULL, NULL, ctx->_readerBuffer.localpixels(), outputBufferBytes, &bytesUsed, &textureFormat) != HapResult_No_Error)
    {
        SLog::log << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - An error occured while decoding frame" << Log::endl;
        return;
    }
    
    ctx->_bufferImage.swap(ctx->_readerBuffer);
    ctx->_imageUpdated = true;
    ctx->updateTimestamp();
}

/*************/
void Image_Shmdata::readUncompressedFrame(Image_Shmdata* ctx, void* shmbuf, void* data, int data_size)
{
    lock_guard<mutex> lock(ctx->_writeMutex);

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
    _attribFunctions["file"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        return read(args[0].asString());
    });
}

}
