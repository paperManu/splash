#include "image_shmdata.h"
#include "timer.h"
#include "threadpool.h"

#include <regex>

#define SPLASH_SHMDATA_THREADS 16

using namespace std;
using namespace OIIO_NAMESPACE;

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

    SLog::log << Log::DEBUGGING << "Image_Shmdata::~Image_Shmdata - Destructor" << Log::endl;
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
bool Image_Shmdata::write(const ImageBuf& img, const string& filename)
{
    if (img.localpixels() == NULL)
        return false;

    lock_guard<mutex> lock(_mutex);
    ImageSpec spec = img.spec();
    if (spec.width != _writerSpec.width || spec.height != _writerSpec.height || spec.nchannels != _writerSpec.nchannels || _writer == NULL || _filename != filename)
        if (!initShmWriter(spec, filename))
            return false;

    memcpy(_writerBuffer.localpixels(), img.localpixels(), _writerInputSize);
    unsigned long long currentTime = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
    shmdata_any_writer_push_data(_writer, (void*)_writerBuffer.localpixels(), _writerInputSize, (currentTime - _writerStartTime) * 1e6, NULL, NULL);
    return true;
}

/*************/
bool Image_Shmdata::initShmWriter(const ImageSpec& spec, const string& filename)
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
    Image_Shmdata* context = static_cast<Image_Shmdata*>(user_data);

    STimer::timer << "image_shmdata " + context->_name;

    string dataType(type_description);
    regex regRgb, regGray, regYUV, regBpp, regWidth, regHeight, regRed, regBlue, regFormatYUV;
    try
    {
        // TODO: replace these regex with some GCC 4.7 compatible ones
        // GCC 4.6 does not support the full regular expression. Some work around is needed,
        // this is why this may seem complicated for nothing ...
        regRgb = regex("(video/x-raw-rgb)(.*)", regex_constants::extended);
        regYUV = regex("(video/x-raw-yuv)(.*)", regex_constants::extended);
        regFormatYUV = regex("(.*format=\\(fourcc\\))(.*)", regex_constants::extended);
        regBpp = regex("(.*bpp=\\(int\\))(.*)", regex_constants::extended);
        regWidth = regex("(.*width=\\(int\\))(.*)", regex_constants::extended);
        regHeight = regex("(.*height=\\(int\\))(.*)", regex_constants::extended);
        regRed = regex("(.*red_mask=\\(int\\))(.*)", regex_constants::extended);
        regBlue = regex("(.*blue_mask=\\(int\\))(.*)", regex_constants::extended);
    }
    catch (const regex_error& e)
    {
        SLog::log << Log::WARNING << "Image_Shmdata::" << __FUNCTION__ << " - Regex error code: " << e.code() << Log::endl;
        return;
    }

    if (regex_match(dataType, regRgb) || regex_match(dataType, regYUV))
    {
        int bpp, width, height, red, green, blue, channels;
        bool isYUV {false};
        bool is420 {false};
        smatch match;
        string substr, format;

        if (regex_match(dataType, match, regBpp))
        {
            ssub_match subMatch = match[2];
            substr = subMatch.str();
            sscanf(substr.c_str(), ")%i", &bpp);
        }
        if (regex_match(dataType, match, regWidth))
        {
            ssub_match subMatch = match[2];
            substr = subMatch.str();
            sscanf(substr.c_str(), ")%i", &width);
        }
        if (regex_match(dataType, match, regHeight))
        {
            ssub_match subMatch = match[2];
            substr = subMatch.str();
            sscanf(substr.c_str(), ")%i", &height);
        }
        if (regex_match(dataType, match, regRed))
        {
            ssub_match subMatch = match[2];
            substr = subMatch.str();
            sscanf(substr.c_str(), ")%i", &red);
        }
        else if (regex_match(dataType, regYUV))
        {
            isYUV = true;
        }
        if (regex_match(dataType, match, regBlue))
        {
            ssub_match subMatch = match[2];
            substr = subMatch.str();
            sscanf(substr.c_str(), ")%i", &blue);
        }

        if (bpp == 24)
            channels = 3;
        else if (isYUV)
        {
            bpp = 12;
            channels = 3;

            if (regex_match(dataType, match, regFormatYUV))
            {
                char format[16];
                ssub_match subMatch = match[2];
                substr = subMatch.str();
                sscanf(substr.c_str(), ")%s", format);
                if (strstr(format, (char*)"I420") != nullptr)
                    is420 = true;
                else
                    return; // No other YUV format is supported yet
            }
        }
        else
            return;

        if (width == 0 || height == 0 || bpp == 0)
            return;

        oiio::ImageSpec spec(width, height, 4, oiio::TypeDesc::UINT8);
        oiio::ImageBuf img(spec);
        if (!is420 && channels == 3)
        {
            char* pixels = (char*)img.localpixels();
            vector<unsigned int> threadIds;
            for (int block = 0; block < SPLASH_SHMDATA_THREADS; ++block)
            {
                threadIds.push_back(SThread::pool.enqueue([=]() {
                    for (int p = width * height / SPLASH_SHMDATA_THREADS * block; p < width * height / SPLASH_SHMDATA_THREADS * (block + 1); ++p)
                    {
                        int pixel = *((int*)&((const char*)data)[p * 3]);
                        memcpy(&(pixels[p * 4]), &pixel, 3 * sizeof(char));
                        pixels[p * 4 + 3] = 255;
                    }
                }));
            }
            SThread::pool.waitThreads(threadIds);
        }
        else if (is420)
        {
            unsigned char* Y = (unsigned char*)data;
            unsigned char* U = (unsigned char*)data + width * height;
            unsigned char* V = (unsigned char*)data + width * height * 5 / 4;

            char* pixels = (char*)img.localpixels();
            vector<unsigned int> threadIds;
            for (int block = 0; block < SPLASH_SHMDATA_THREADS; ++block)
            {
                threadIds.push_back(SThread::pool.enqueue([=, &context]() {
                    for (int y = height / SPLASH_SHMDATA_THREADS * block; y < height / SPLASH_SHMDATA_THREADS * (block + 1); ++y)
                        for (int x = 0; x < width; ++x)
                        {
                            int yValue = (int)Y[y * width + x];
                            int uValue = (int)U[(y / 2) * (width / 2) + x / 2] - 128;
                            int vValue = (int)V[(y / 2) * (width / 2) + x / 2] - 128;
                            
                            pixels[(y * width + x) * 4] = (unsigned char)max(0, min(255, (yValue * 1164 + 1596 * vValue) / 1000));
                            pixels[(y * width + x) * 4 + 1] = (unsigned char)max(0, min(255, (yValue * 1164 - 392 * uValue - 813 * vValue) / 1000));
                            pixels[(y * width + x) * 4 + 2] = (unsigned char)max(0, min(255, (yValue * 1164 + 2017 * uValue) / 1000));
                            pixels[(y * width + x) * 4 + 3] = 255;
                        }
                }));
            }
            SThread::pool.waitThreads(threadIds);
        }
        else
            return;

        lock_guard<mutex> lock(context->_mutex);
        context->_bufferImage.swap(img);
        context->_imageUpdated = true;
        context->updateTimestamp();
    }

    shmdata_any_reader_free(shmbuf);

    STimer::timer >> "image_shmdata " + context->_name;
}

/*************/
void Image_Shmdata::registerAttributes()
{
    _attribFunctions["file"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        return read(args[0].asString());
    });
}

}
