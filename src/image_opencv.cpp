#include "image_opencv.h"

#include <opencv2/opencv.hpp>

#include <chrono>
#include <hap.h>

#include "cgUtils.h"
#include "log.h"
#include "timer.h"
#include "threadpool.h"

using namespace std;

namespace Splash
{

/*************/
Image_OpenCV::Image_OpenCV()
{
    _type = "image_opencv";
    registerAttributes();

    _videoCapture = unique_ptr<cv::VideoCapture>(new cv::VideoCapture());
}

/*************/
Image_OpenCV::~Image_OpenCV()
{
    _continueReading = false;
    if (_readLoopThread.joinable())
        _readLoopThread.join();
}

/*************/
bool Image_OpenCV::read(const string& filename)
{
    try
    {
        _inputIndex = stoi(filename);
    }
    catch (...)
    {
        _inputIndex = 0;
    }

    _continueReading = true;
    _readLoopThread = thread([&]() {
        readLoop();
    });

    return true;
}

/*************/
void Image_OpenCV::readLoop()
{
    if (!_videoCapture)
    {
        Log::get() << Log::WARNING << "Image_OpenCV::" << __FUNCTION__ << " - Unable to create the VideoCapture" << Log::endl;
        return;
    }

    if (!_videoCapture->isOpened())
    {
        if (!_videoCapture->open(_inputIndex))
        {
            Log::get() << Log::WARNING << "Image_OpenCV::" << __FUNCTION__ << " - Unable to open video capture input " << _inputIndex << Log::endl;
            return;
        }
        _videoCapture->set(CV_CAP_PROP_FRAME_WIDTH, _width);
        _videoCapture->set(CV_CAP_PROP_FRAME_HEIGHT, _height);
        _videoCapture->set(CV_CAP_PROP_FPS, _framerate);

        Log::get() << Log::MESSAGE << "Image_OpenCV::" << __FUNCTION__ << " - Sucessfully initialized VideoCapture " << _inputIndex << Log::endl;
    }

    while (_continueReading)
    {
        if (Timer::get().isDebug())
            Timer::get() << "read " + _name;

        auto capture = cv::Mat();
        if (!_videoCapture->read(capture))
        {
            Log::get() << Log::WARNING << "Image_OpenCV::" << __FUNCTION__ << " - An error occurred while reading the VideoCapture" << Log::endl;
            return;
        }

        oiio::ImageSpec spec(capture.cols, capture.rows, capture.channels(), oiio::TypeDesc::UINT8);
        spec.channelnames = vector<string>({"B", "G", "R"});
        oiio::ImageBuf img(spec);
        unsigned char* pixels = static_cast<unsigned char*>(img.localpixels());

        unsigned int imageSize = capture.rows * capture.cols * capture.channels();
        copy(capture.data, capture.data + imageSize, pixels);

        unique_lock<mutex> lockWrite(_writeMutex);
        _bufferImage.swap(img);
        _imageUpdated = true;
        updateTimestamp();

        if (Timer::get().isDebug())
            Timer::get() >> "read " + _name;
    }
}

/*************/
void Image_OpenCV::registerAttributes()
{
    _attribFunctions["size"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 2)
            return false;
        _width = args[0].asInt();
        _height = args[1].asInt();
        
        _width = (_width == 0) ? 640 : _width;
        _height = (_height == 0) ? 640 : _height;

        return true;
    }, [&]() -> Values {
        return {(int)_width, (int)_height};
    });

    _attribFunctions["framerate"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        _framerate = (args[0].asFloat() == 0) ? 60 : args[0].asFloat();
        return true;
    }, [&]() -> Values {
        return {_framerate};
    });
}

} // end of namespace
