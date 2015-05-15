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

    return true;
}

/*************/
void Image_OpenCV::update()
{
    if (!_videoCapture)
        return;

    if (!_videoCapture->isOpened())
    {
        if (!_videoCapture->open(_inputIndex))
            return;
        _videoCapture->set(CV_CAP_PROP_FRAME_WIDTH, _width);
        _videoCapture->set(CV_CAP_PROP_FRAME_HEIGHT, _height);
    }

    auto capture = cv::Mat();
    if (!_videoCapture->read(capture))
        return;

    oiio::ImageSpec spec(capture.cols, capture.rows, capture.channels(), oiio::TypeDesc::UINT8);
    oiio::ImageBuf img(spec);
    unsigned char* pixels = static_cast<unsigned char*>(img.localpixels());

    unsigned int imageSize = capture.rows * capture.cols * capture.channels();
    copy(capture.data, capture.data + imageSize, pixels);

    unique_lock<mutex> lockRead(_readMutex);
    unique_lock<mutex> lockWrite(_writeMutex);
    _image.swap(img);
    updateTimestamp();
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
    });
}

} // end of namespace
