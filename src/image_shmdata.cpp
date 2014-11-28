#include "image_shmdata.h"

#include "log.h"
#include "timer.h"
#include "threadpool.h"

#include <regex>
#include <simdpp/simd.h>
#include <hap.h>

#define SPLASH_SHMDATA_THREADS 16

using namespace std;
using namespace simdpp;

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
    if (_reader != nullptr)
        shmdata_any_reader_close(_reader);

    _reader = shmdata_any_reader_init();
    shmdata_any_reader_run_gmainloop(_reader, SHMDATA_TRUE);
    shmdata_any_reader_set_on_data_handler(_reader, Image_Shmdata::onData, this);
    shmdata_any_reader_start(_reader, filename.c_str());
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
    if (bufSpec.width != ctx->_width || bufSpec.height != ctx->_height && textureFormat != ctx->_textureFormat)
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
#if SIMDPP_NO_SSE
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
                        int yValue = (int)(Y[row * ctx->_width + col]) * 38142;
                        pixels[(row * ctx->_width + col) * 3] = (unsigned char)clamp((yValue + rPart) / 32768, 0, 255);
                        pixels[(row * ctx->_width + col) * 3 + 1] = (unsigned char)clamp((yValue + gPart) / 32768, 0, 255);
                        pixels[(row * ctx->_width + col) * 3 + 2] = (unsigned char)clamp((yValue + bPart) / 32768, 0, 255);

                        col++;
                        yValue = (int)(Y[row * ctx->_width + col]) * 38142;
                        pixels[(row * ctx->_width + col) * 3] = (unsigned char)clamp((yValue + rPart) / 32768, 0, 255);
                        pixels[(row * ctx->_width + col) * 3 + 1] = (unsigned char)clamp((yValue + gPart) / 32768, 0, 255);
                        pixels[(row * ctx->_width + col) * 3 + 2] = (unsigned char)clamp((yValue + bPart) / 32768, 0, 255);
                    }
            }));
        }
#else
        for (int block = 0; block < SPLASH_SHMDATA_THREADS; ++block)
        {
            int width = ctx->_width;
            int height = ctx->_height;

            threadIds.push_back(SThread::pool.enqueue([=, &ctx]() {
                int lastLine; // We compute the last line, to handle image size non divisible by SPLASH_SHMDATA_THREADS
                if (height - height / SPLASH_SHMDATA_THREADS * (block + 1) < height / SPLASH_SHMDATA_THREADS)
                    lastLine = height;
                else
                    lastLine = height / SPLASH_SHMDATA_THREADS * (block + 1);

                for (int y = height / SPLASH_SHMDATA_THREADS * block; y < lastLine; y += 2)
                    for (int x = 0; x + 7 < width; x += 8)
                    {
                        int16x8 yValue[2];
                        int16x8 uValue;
                        int16x8 vValue;
                        uint8x16 loadBuf;

                        load_u(loadBuf, &(U[(y / 2) * (width / 2) + x / 2]));
                        uValue = to_int16x8(loadBuf);
                        uValue = zip_lo(uValue, uValue);

                        load_u(loadBuf, &(V[(y / 2) * (width / 2) + x / 2]));
                        vValue = to_int16x8(loadBuf);
                        vValue = zip_lo(vValue, vValue);

                        uValue = sub(uValue, int16x8::make_const(128));
                        vValue = sub(vValue, int16x8::make_const(128));

                        int16x8 red, grn, blu;
                        uint8x16 uRed, uGrn, uBlu;
                        
                        for (int l = 0; l < 2; ++l)
                        {
                            load_u(loadBuf, &(Y[(y + l) * width + x]));
                            yValue[l] = to_int16x8(loadBuf);

                            red = add(mul_lo(yValue[l], int16x8::make_const(74)), mul_lo(uValue, int16x8::make_const(102)));
                            grn = add(mul_lo(yValue[l], int16x8::make_const(74)), add(mul_lo(uValue, int16x8::make_const(-25)), mul_lo(vValue, int16x8::make_const(-52))));
                            blu = add(mul_lo(yValue[l], int16x8::make_const(74)), mul_lo(vValue, int16x8::make_const(129)));

                            red = shift_r(red, 6);
                            grn = shift_r(grn, 6);
                            blu = shift_r(blu, 6);

                            red = simdpp::min(red, int16x8::make_const(255));
                            grn = simdpp::min(grn, int16x8::make_const(255));
                            blu = simdpp::min(blu, int16x8::make_const(255));

                            red = simdpp::max(red, int16x8::make_const(0));
                            grn = simdpp::max(grn, int16x8::make_const(0));
                            blu = simdpp::max(blu, int16x8::make_const(0));

                            uRed = red;
                            uRed = unzip_lo(uRed, uRed);
                            uGrn = grn;
                            uGrn = unzip_lo(uGrn, uGrn);
                            uBlu = blu;
                            uBlu = unzip_lo(uBlu, uBlu);

                            alignas(32) char dst[48];
                            store_packed3(dst, uBlu, uGrn, uRed);

                            memcpy(&(pixels[((y + l) * width + x) * 3]), dst, 24*sizeof(char));
                        }
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
                        int uValue = (int)(YUV[y * ctx->_width * 2 + x * 2]) - 128;
                        int vValue = (int)(YUV[y * ctx->_width * 2 + x * 2 + 2]) - 128;

                        int rPart = 52298 * vValue;
                        int gPart = -12846 * uValue - 36641 * vValue;
                        int bPart = 66094 * uValue;
                       
                        int yValue = (int)(YUV[y * ctx->_width * 2 + x * 2 + 1]) * 38142;
                        pixels[(y * ctx->_width + x) * 3] = (unsigned char)clamp((yValue + rPart) / 32768, 0, 255);
                        pixels[(y * ctx->_width + x) * 3 + 1] = (unsigned char)clamp((yValue + gPart) / 32768, 0, 255);
                        pixels[(y * ctx->_width + x) * 3 + 2] = (unsigned char)clamp((yValue + bPart) / 32768, 0, 255);

                        yValue = (int)(YUV[y * ctx->_width * 2 + x * 2 + 3]) * 38142;
                        pixels[(y * ctx->_width + x + 1) * 3] = (unsigned char)clamp((yValue + rPart) / 32768, 0, 255);
                        pixels[(y * ctx->_width + x + 1) * 3 + 1] = (unsigned char)clamp((yValue + gPart) / 32768, 0, 255);
                        pixels[(y * ctx->_width + x + 1) * 3 + 2] = (unsigned char)clamp((yValue + bPart) / 32768, 0, 255);
                    }
            }));
        }
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
    _attribFunctions["file"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;
        return read(args[0].asString());
    });
}

}
