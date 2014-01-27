#include "image_shmdata.h"
#include "timer.h"

#include <regex>

using namespace std;

namespace Splash
{

/*************/
Image_Shmdata::Image_Shmdata()
{
    _type = "image_shmdata";

    registerAttributes();
    computeLUT();
}

/*************/
Image_Shmdata::~Image_Shmdata()
{
    if (_reader != nullptr)
        shmdata_any_reader_close(_reader);

    SLog::log << Log::DEBUG << "Image_Shmdata::~Image_Shmdata - Destructor" << Log::endl;
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

    return true;
}

/*************/
void Image_Shmdata::update()
{
    lock_guard<mutex> lock(_mutex);
    if (_imageUpdated)
    {
        _image.swap(_bufferImage);
        _imageUpdated = false;
        updateTimestamp();
    }
}

/*************/
void Image_Shmdata::computeLUT()
{
    // Compute YCbCr to RGB lookup table
    for (int y = 0; y < 256; ++y)
    {
        vector<vector<vector<unsigned char>>> values_1(256);
        for (int u = 0; u < 256; ++u)
        {
            vector<vector<unsigned char>> values_2(256);
            for (int v = 0; v < 256; ++v)
            {
                float yValue = (float)y;
                float uValue = (float)u - 128.f;
                float vValue = (float)v - 128.f;

                vector<unsigned char> pixel(3);
                pixel[0] = (unsigned char)max(0.f, min(255.f, (yValue * 1.164f + 1.596f * vValue)));
                pixel[1] = (unsigned char)max(0.f, min(255.f, (yValue * 1.164f - 0.392f * uValue - 0.813f * vValue)));
                pixel[2] = (unsigned char)max(0.f, min(255.f, (yValue * 1.164f + 2.017f * uValue)));

                values_2[v] = pixel;
            }
            values_1[u] = values_2;
        }
        _yCbCrLUT.push_back(values_1);
    }
}

/*************/
void Image_Shmdata::onData(shmdata_any_reader_t* reader, void* shmbuf, void* data, int data_size, unsigned long long timestamp,
    const char* type_description, void* user_data)
{
    Image_Shmdata* context = static_cast<Image_Shmdata*>(user_data);

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

        ImageSpec spec(width, height, 4, TypeDesc::UINT8);
        ImageBuf img(spec);
        if (!is420 && channels == 3)
        {
            for (ImageBuf::Iterator<unsigned char, unsigned char> p(img); !p.done(); ++p)
            {
                if (!p.exists())
                    continue;
                p[0] = ((const char*)data)[(p.y() * width + p.x()) * 3];
                p[1] = ((const char*)data)[(p.y() * width + p.x()) * 3 + 1];
                p[2] = ((const char*)data)[(p.y() * width + p.x()) * 3 + 2];
                p[4] = 255;
            }
        }
        else if (is420)
        {
            unsigned char* Y = (unsigned char*)data;
            unsigned char* U = (unsigned char*)data + width * height;
            unsigned char* V = (unsigned char*)data + width * height * 5 / 4;

            for (ImageBuf::Iterator<unsigned char, unsigned char> p(img); !p.done(); ++p)
            {
                if (!p.exists())
                    continue;

                int yValue = (int)Y[p.y() * width + p.x()];
                int uValue = (int)U[p.y() * width / 4 + p.x() / 2];
                int vValue = (int)V[p.y() * width / 4 + p.x() / 2];

                p[0] = context->_yCbCrLUT[yValue][uValue][vValue][0];
                p[1] = context->_yCbCrLUT[yValue][uValue][vValue][1];
                p[2] = context->_yCbCrLUT[yValue][uValue][vValue][2];
                p[3] = 255;
            }
        }
        else
            return;

        lock_guard<mutex> lock(context->_mutex);
        context->_bufferImage.swap(img);
        context->_imageUpdated = true;
    }

    shmdata_any_reader_free(shmbuf);
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
