#include "./image/image_opencv.h"

#include <chrono>

#include <opencv2/opencv.hpp>
#include <hap.h>

#include "./utils/cgutils.h"
#include "./utils/log.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

/*************/
Image_OpenCV::Image_OpenCV(RootObject* root)
    : Image(root)
{
    init();
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
        _inputIndex = -1;
    }

    // This releases any previous input
    _continueReading = false;
    if (_readLoopThread.joinable())
        _readLoopThread.join();

    _continueReading = true;
    _readLoopThread = thread([&]() { readLoop(); });

    return true;
}

/*************/
void Image_OpenCV::init()
{
    _type = "image_opencv";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    _videoCapture = unique_ptr<cv::VideoCapture>(new cv::VideoCapture());
}

/*************/
void Image_OpenCV::readLoop()
{
    if (!_videoCapture)
    {
        Log::get() << Log::WARNING << "Image_OpenCV::" << __FUNCTION__ << " - Unable to create the VideoCapture" << Log::endl;
        return;
    }

    auto realPath = _filepath;
    if (_inputIndex != -1)
        realPath = to_string(_inputIndex);

    if (!_videoCapture->isOpened())
    {
        bool status;
        if (_inputIndex >= 0)
            status = _videoCapture->open(_inputIndex);
        else
            status = _videoCapture->open(realPath);

        if (!status)
        {
            Log::get() << Log::WARNING << "Image_OpenCV::" << __FUNCTION__ << " - Unable to open video capture input " << realPath << Log::endl;
            return;
        }

        _videoCapture->set(CV_CAP_PROP_FRAME_WIDTH, _width);
        _videoCapture->set(CV_CAP_PROP_FRAME_HEIGHT, _height);
        _videoCapture->set(CV_CAP_PROP_FPS, _framerate);

        Log::get() << Log::MESSAGE << "Image_OpenCV::" << __FUNCTION__ << " - Successfully initialized VideoCapture " << realPath << Log::endl;
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

        auto spec = _readBuffer.getSpec();
        if (static_cast<int>(spec.width) != capture.rows || static_cast<int>(spec.height) != capture.cols || static_cast<int>(spec.channels) != capture.channels())
        {
            ImageBufferSpec newSpec(capture.cols, capture.rows, capture.channels(), 8 * capture.channels(), ImageBufferSpec::Type::UINT8);
            newSpec.format = "BGR";
            _readBuffer = ImageBuffer(newSpec);
        }
        unsigned char* pixels = reinterpret_cast<unsigned char*>(_readBuffer.data());

        unsigned int imageSize = capture.rows * capture.cols * capture.channels();
        copy(capture.data, capture.data + imageSize, pixels);

        lock_guard<shared_timed_mutex> lockWrite(_writeMutex);
        if (!_bufferImage)
            _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer());
        std::swap(*_bufferImage, _readBuffer);
        _imageUpdated = true;
        updateTimestamp();

        if (Timer::get().isDebug())
            Timer::get() >> ("read " + _name);
    }
}

/*************/
void Image_OpenCV::registerAttributes()
{
    Image::registerAttributes();

    addAttribute("size",
        [&](const Values& args) {
            _width = args[0].as<int>();
            _height = args[1].as<int>();

            _width = (_width == 0) ? 640 : _width;
            _height = (_height == 0) ? 640 : _height;

            return true;
        },
        [&]() -> Values {
            return {(int)_width, (int)_height};
        },
        {'n', 'n'});
    setAttributeDescription("size", "Set the desired capture resolution");

    addAttribute("framerate",
        [&](const Values& args) {
            _framerate = (args[0].as<float>() == 0) ? 60 : args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_framerate}; },
        {'n'});
    setAttributeDescription("framerate", "Set the desired capture framerate");
}

} // end of namespace
