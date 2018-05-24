#include "./image/image_v4l2.h"

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#if HAVE_DATAPATH
#include "rgb133v4l2.h"
#endif

#define NUMERATOR 1001
#define DIVISOR 10

using namespace std;

namespace Splash
{
/*************/
Image_V4L2::Image_V4L2(RootObject* root)
    : Image(root)
{
    init();
}

/*************/
Image_V4L2::~Image_V4L2()
{
    stopCapture();
#if HAVE_DATAPATH
    closeControlDevice();
#endif
}

/*************/
void Image_V4L2::init()
{
    _type = "image_v4l2";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

#if HAVE_DATAPATH
    openControlDevice();
#endif
}

/*************/
bool Image_V4L2::doCapture()
{
    lock_guard<mutex> lockStart(_startStopMutex);

    if (_capturing)
        return true;

    if (_devicePath.empty())
    {
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - A device path has to be specified" << Log::endl;
        return false;
    }

    _capturing = openCaptureDevice(_devicePath);
    if (_capturing)
        _capturing = initializeUserPtrCapture();
    if (_capturing)
        _capturing = initializeCapture();
    if (_capturing)
    {
        _captureThreadRun = true;
        _captureFuture = async(std::launch::async, [this]() {
            captureThreadFunc();
            _capturing = false;
        });
    }

    return _capturing;
}

/*************/
void Image_V4L2::stopCapture()
{
    lock_guard<mutex> lockStop(_startStopMutex);

    if (!_capturing)
        return;

    _captureThreadRun = false;
    if (_captureFuture.valid())
        _captureFuture.wait();

    closeCaptureDevice();
}

/*************/
void Image_V4L2::captureThreadFunc()
{
    int result = 0;
    struct v4l2_buffer buffer;
    enum v4l2_buf_type bufferType;
    auto bufferSize = _outputWidth * _outputHeight * _spec.pixelBytes();

    if (!_hasStreamingIO)
    {
        unique_lock<shared_timed_mutex> lockWrite(_writeMutex, std::defer_lock);
        while (_captureThreadRun)
        {
            if (!_bufferImage || _bufferImage->getSpec() != _imageBuffers[buffer.index]->getSpec())
                _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer(_spec));

            lockWrite.lock();
            result = ::read(_deviceFd, _bufferImage->data(), bufferSize);
            lockWrite.unlock();

            if (result < 0)
            {
                Log::get() << Log::WARNING << "Texture_Datapath::" << __FUNCTION__ << " - Failed to read from capture device " << _devicePath << Log::endl;
                return;
            }

            _imageUpdated = true;
            updateTimestamp();
        }
    }
    else
    {
        // Initialize the buffers
        _imageBuffers.clear();
        for (uint32_t i = 0; i < _bufferCount; ++i)
        {
            _imageBuffers.push_back(unique_ptr<ImageBuffer>(new ImageBuffer(_spec)));

            memset(&buffer, 0, sizeof(buffer));
            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_USERPTR;
            buffer.index = i;
            buffer.m.userptr = (unsigned long)_imageBuffers[_imageBuffers.size() - 1]->data();
            buffer.length = bufferSize;

            result = ioctl(_deviceFd, VIDIOC_QBUF, &buffer);
            if (result < 0)
            {
                Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - VIDIOC_QBUF failed: " << result << Log::endl;
                return;
            }
        }

        bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        result = ioctl(_deviceFd, VIDIOC_STREAMON, &bufferType);
        if (result < 0)
        {
            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - VIDIOC_STREAMON failed: " << result << Log::endl;
            return;
        }

        // Run the capture
        unique_lock<shared_timed_mutex> lockWrite(_writeMutex, std::defer_lock);
        while (_captureThreadRun)
        {
            struct pollfd fd;
            fd.fd = _deviceFd;
            fd.events = POLLIN | POLLPRI;
            fd.revents = 0;

            if (poll(&fd, 1, 50) > 0)
            {
                if (fd.revents & (POLLIN | POLLPRI))
                {
                    memset(&buffer, 0, sizeof(buffer));
                    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    buffer.memory = V4L2_MEMORY_USERPTR;

                    result = ioctl(_deviceFd, VIDIOC_DQBUF, &buffer);
                    if (result < 0)
                    {
                        switch (errno)
                        {
                        case EAGAIN:
                            continue;
                        case EIO:
                        default:
                            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Failed to dequeue buffer " << buffer.index << Log::endl;
                            return;
                        }
                    }

                    if (buffer.index >= _bufferCount)
                    {
                        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Invalid buffer index: " << buffer.index << Log::endl;
                        return;
                    }

                    if (!_bufferImage || _bufferImage->getSpec() != _imageBuffers[buffer.index]->getSpec())
                        _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer(_spec));

                    lockWrite.lock();
                    _bufferImage.swap(_imageBuffers[buffer.index]);
                    lockWrite.unlock();

                    _imageUpdated = true;
                    updateTimestamp();

                    auto currentBuffer = buffer.index;
                    memset(&buffer, 0, sizeof(buffer));
                    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    buffer.memory = V4L2_MEMORY_USERPTR;
                    buffer.index = currentBuffer;
                    buffer.m.userptr = (unsigned long)_imageBuffers[currentBuffer]->data();
                    buffer.length = bufferSize;

                    result = ioctl(_deviceFd, VIDIOC_QBUF, &buffer);
                    if (result < 0)
                    {
                        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Failed to requeue buffer " << buffer.index << Log::endl;
                        return;
                    }
                }
            }

#if HAVE_DATAPATH
            // Get the source video format, for information
            if (_isDatapath)
            {
                memset(&_v4l2SourceFormat, 0, sizeof(_v4l2SourceFormat));
                _v4l2SourceFormat.type = V4L2_BUF_TYPE_CAPTURE_SOURCE;
                if (ioctl(_deviceFd, RGB133_VIDIOC_G_SRC_FMT, &_v4l2SourceFormat) >= 0)
                {
                    auto sourceFormatAsString = to_string(_v4l2SourceFormat.fmt.pix.width) + "x" + to_string(_v4l2SourceFormat.fmt.pix.height) + string("@") +
                                                to_string((float)_v4l2SourceFormat.fmt.pix.priv / 1000.f) + "Hz, format " +
                                                string(reinterpret_cast<char*>(&_v4l2SourceFormat.fmt.pix.pixelformat), 4);

                    bool expectedResizingValue = false;
                    if (_autosetResolution && _automaticResizing.compare_exchange_weak(expectedResizingValue, true))
                    {
                        if (_outputWidth != _v4l2SourceFormat.fmt.pix.width || _outputHeight != _v4l2SourceFormat.fmt.pix.height)
                        {
                            addTask([=]() {
                                setAttribute("captureSize", {_v4l2SourceFormat.fmt.pix.width, _v4l2SourceFormat.fmt.pix.height});
                                _automaticResizing = false;
                            });
                        }
                        else
                        {
                            _automaticResizing = false;
                        }
                    }

                    _sourceFormatAsString = sourceFormatAsString;
                }
            }
#endif
        }

        bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        result = ioctl(_deviceFd, VIDIOC_STREAMOFF, &bufferType);
        if (result < 0)
            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - VIDIOC_STREAMOFF failed: " << result << Log::endl;

        for (uint32_t i = 0; i < _bufferCount; ++i)
        {
            memset(&buffer, 0, sizeof(buffer));
            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_USERPTR;
            result = ioctl(_deviceFd, VIDIOC_DQBUF, &buffer);
            if (result < 0)
                Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - VIDIOC_DQBUF failed: " << result << Log::endl;
        }
    }

    // Reset to a default image
    _bufferImage = make_unique<ImageBuffer>(ImageBufferSpec(512, 512, 4, 32));
    _bufferImage->zero();
    _imageUpdated = true;
    updateTimestamp();
}

/*************/
bool Image_V4L2::initializeUserPtrCapture()
{
    memset(&_v4l2RequestBuffers, 0, sizeof(_v4l2RequestBuffers));
    _v4l2RequestBuffers.count = _bufferCount;
    _v4l2RequestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    _v4l2RequestBuffers.memory = V4L2_MEMORY_USERPTR;

    int result = ioctl(_deviceFd, VIDIOC_REQBUFS, &_v4l2RequestBuffers);
    if (result < 0)
    {
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Device does not support userptr IO: " << result << Log::endl;
        return false;
    }

    return true;
}

/*************/
bool Image_V4L2::initializeCapture()
{
#if HAVE_DATAPATH
    if (_isDatapath)
    {
        struct v4l2_queryctrl queryCtrl;
        struct v4l2_control control;

        memset(&queryCtrl, 0, sizeof(queryCtrl));
        queryCtrl.id = RGB133_V4L2_CID_LIVESTREAM;

        int result = ioctl(_deviceFd, VIDIOC_QUERYCTRL, &queryCtrl);
        if (result == -1)
        {
            if (errno != EINVAL)
            {
                Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - VIDIOC_QUERYCTRL failed for RGB133_V4L2_CID_LIVESTREAM" << Log::endl;
                return false;
            }
            else
            {
                Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - RGB133_V4L2_CID_LIVESTREAM is not supported" << Log::endl;
            }
        }
        else if (queryCtrl.flags & V4L2_CTRL_FLAG_DISABLED)
        {
            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - RGB133_V4L2_CID_LIVESTREAM is not supported" << Log::endl;
        }
        else
        {
            memset(&control, 0, sizeof(control));
            control.id = RGB133_V4L2_CID_LIVESTREAM;
            control.value = 1; // enable livestream
            result = ioctl(_deviceFd, VIDIOC_S_CTRL, &control);
            if (result == -1)
            {
                Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - VIDIOC_S_CTRL failed for RGB133_V4L2_CID_LIVESTREAM" << Log::endl;
                return false;
            }
        }

        memset(&control, 0, sizeof(control));
        control.id = RGB133_V4L2_CID_LIVESTREAM;
        result = ioctl(_deviceFd, VIDIOC_G_CTRL, &control);
        if (result == -1)
        {
            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - VIDIOC_G_CTRL failed for RGB133_V4L2_CID_LIVESTREAM" << Log::endl;
            return false;
        }

        Log::get() << Log::MESSAGE << "Image_V4L2::" << __FUNCTION__ << " - RGB133_V4L2_CID_LIVESTREAM current value: " << control.value << Log::endl;
    }
#endif

    return true;
}

/*************/
#if HAVE_DATAPATH
bool Image_V4L2::openControlDevice()
{
    if ((_controlFd = open(_controlDevicePath.c_str(), O_RDWR)) < 0)
    {
        _isDatapath = false;
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Unable to open control device." << Log::endl;
        return false;
    }

    _isDatapath = true;
    return true;
}

/*************/
void Image_V4L2::closeControlDevice()
{
    if (_controlFd >= 0)
    {
        close(_controlFd);
        _controlFd = -1;
    }
}
#endif

/*************/
bool Image_V4L2::openCaptureDevice(const std::string& devicePath)
{
    if (devicePath.empty())
    {
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - A device path has to be specified" << Log::endl;
        return false;
    }

    if ((_deviceFd = open(devicePath.c_str(), O_RDWR)) < 0)
    {
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Unable to open capture device" << Log::endl;
        return false;
    }

    if (ioctl(_deviceFd, VIDIOC_QUERYCAP, &_v4l2Capability) < 0)
    {
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Unable to query device capabilities" << Log::endl;
        return false;
    }

    if (!_capabilitiesEnumerated)
    {
        if (_v4l2Capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        {
            if (!enumerateCaptureDeviceInputs())
                Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Failed to enumerate capture inputs" << Log::endl;

            if (!enumerateVideoStandards())
                Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Failed to enumerate video standards" << Log::endl;

            if (!enumerateCaptureFormats())
                Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Failed to enumerate capture formats" << Log::endl;

            _capabilitiesEnumerated = true;
        }
        else
        {
            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Only video captures are supported" << Log::endl;
            closeCaptureDevice();
            return false;
        }

        if (_v4l2Capability.capabilities & V4L2_CAP_STREAMING)
        {
            _hasStreamingIO = true;
            Log::get() << Log::MESSAGE << "Image_V4L2::" << __FUNCTION__ << " - Capture device supports streaming I/O" << Log::endl;
        }
        else
        {
            _hasStreamingIO = false;
            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Capture device does not support streaming I/O" << Log::endl;
        }
    }

    // Choose the input
    v4l2_input v4l2Input;
    memset(&v4l2Input, 0, sizeof(v4l2Input));
    v4l2Input.index = _v4l2Index;
    if (ioctl(_deviceFd, VIDIOC_S_INPUT, &v4l2Input) < 0)
    {
        Log::get() << Log::WARNING << "Texture_Datapath::" << __FUNCTION__ << " - Unable to select the input " << _v4l2Index << Log::endl;
        closeCaptureDevice();
        return false;
    }

    // Try setting the video format
    memset(&_v4l2Format, 0, sizeof(_v4l2Format));
    _v4l2Format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    _v4l2Format.fmt.pix.width = _outputWidth;
    _v4l2Format.fmt.pix.height = _outputHeight;
    _v4l2Format.fmt.pix.field = V4L2_FIELD_NONE;
    _v4l2Format.fmt.pix.pixelformat = _outputPixelFormat;

    if (ioctl(_deviceFd, VIDIOC_S_FMT, &_v4l2Format) < 0)
    {
        Log::get() << Log::WARNING << "Texture_Datapath::" << __FUNCTION__ << " - Unable to set the desired video format" << Log::endl;
        closeCaptureDevice();
        return false;
    }

    Log::get() << Log::MESSAGE << "Image_V4L2::" << __FUNCTION__ << " - Trying to set capture format to: " << _outputWidth << "x" << _outputHeight << ", format "
               << string(reinterpret_cast<char*>(&_outputPixelFormat), 4) << Log::endl;

    // Get the real video format
    if (ioctl(_deviceFd, VIDIOC_G_FMT, &_v4l2Format) < 0)
    {
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Failed to get current output buffer width and height" << Log::endl;
        closeCaptureDevice();
        return false;
    }

    _outputWidth = _v4l2Format.fmt.pix.width;
    _outputHeight = _v4l2Format.fmt.pix.height;

    Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Capture format set to: " << _outputWidth << "x" << _outputHeight << " for format "
               << string(reinterpret_cast<char*>(&_outputPixelFormat), 4) << Log::endl;

    switch (_outputPixelFormat)
    {
    default:
    case V4L2_PIX_FMT_RGB24:
        _spec = ImageBufferSpec(_outputWidth, _outputHeight, 3, 24, ImageBufferSpec::Type::UINT8, "RGB");
        break;
    case V4L2_PIX_FMT_YUYV:
        _spec = ImageBufferSpec(_outputWidth, _outputHeight, 3, 16, ImageBufferSpec::Type::UINT8, "YUYV");
        break;
    }

    return true;
}

/*************/
void Image_V4L2::closeCaptureDevice()
{
    _v4l2Inputs.clear();
    _v4l2Standards.clear();
    _v4l2Formats.clear();

    if (_deviceFd >= 0)
    {
        close(_deviceFd);
        _deviceFd = -1;
    }
}

/*************/
bool Image_V4L2::enumerateCaptureDeviceInputs()
{
    if (_deviceFd < 0)
    {
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Invalid device file descriptor" << Log::endl;
        return false;
    }

    _v4l2InputCount = 0;
    struct v4l2_input input;

    memset(&input, 0, sizeof(input));
    while (ioctl(_deviceFd, VIDIOC_ENUMINPUT, &input) >= 0)
    {
        ++_v4l2InputCount;
        input.index = _v4l2InputCount;
    }

    _v4l2Inputs.resize(_v4l2InputCount);
    for (int i = 0; i < _v4l2InputCount; ++i)
    {
        _v4l2Inputs[i].index = i;
        if (ioctl(_deviceFd, VIDIOC_ENUMINPUT, &_v4l2Inputs[i]))
        {
            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Failed to enumerate input " << i << Log::endl;
            return false;
        }
        Log::get() << Log::MESSAGE << "Image_V4L2::" << __FUNCTION__ << " - Detected input "
                   << string(reinterpret_cast<char*>(&_v4l2Inputs[i].name[0]), sizeof(_v4l2Inputs[i].name)) << " of type " << _v4l2Inputs[i].type << Log::endl;
    }

    return true;
}

/*************/
bool Image_V4L2::enumerateCaptureFormats()
{
    if (_deviceFd < 0)
    {
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Invalid device file descriptor" << Log::endl;
        return false;
    }

    struct v4l2_fmtdesc format;

    memset(&format, 0, sizeof(format));
    format.index = 0;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    _v4l2FormatCount = 0;
    while (ioctl(_deviceFd, VIDIOC_ENUM_FMT, &format) >= 0)
    {
        ++_v4l2FormatCount;
        format.index = _v4l2FormatCount;
    }

    _v4l2Formats.resize(_v4l2FormatCount);
    for (int i = 0; i < _v4l2FormatCount; ++i)
    {
        _v4l2Formats[i].index = i;
        _v4l2Formats[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (ioctl(_deviceFd, VIDIOC_ENUM_FMT, &_v4l2Formats[i]) < 0)
        {
            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Failed to get format " << i << " description" << Log::endl;
            return false;
        }
        else
        {
            Log::get() << Log::MESSAGE << "Image_V4L2::" << __FUNCTION__ << " - Detected format "
                       << string(reinterpret_cast<char*>(&_v4l2Formats[i].description), sizeof(_v4l2Formats[i].description)) << Log::endl;
        }
    }

    return true;
}

/*************/
bool Image_V4L2::enumerateVideoStandards()
{
    if (_deviceFd < 0)
    {
        Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Invalid device file descriptor" << Log::endl;
        return false;
    }

    _v4l2StandardCount = 0;
    struct v4l2_standard standards;
    standards.index = 0;

    while (ioctl(_deviceFd, VIDIOC_ENUMSTD, &standards) >= 0)
    {
        ++_v4l2StandardCount;
        standards.index = _v4l2StandardCount;
    }

    _v4l2Standards.resize(_v4l2StandardCount);
    for (int i = 0; i < _v4l2StandardCount; ++i)
    {
        _v4l2Standards[i].index = i;
        if (ioctl(_deviceFd, VIDIOC_ENUMSTD, &_v4l2Standards[i]))
        {
            Log::get() << Log::WARNING << "Image_V4L2::" << __FUNCTION__ << " - Failed to enumerate video standard " << i << Log::endl;
            return false;
        }
        Log::get() << Log::MESSAGE << "Image_V4L2::" << __FUNCTION__
                   << " - Detected video standard: " << string(reinterpret_cast<char*>(&_v4l2Standards[i].name[0]), sizeof(_v4l2Standards[i].name)) << Log::endl;
    }

    return true;
}

/*************/
void Image_V4L2::updateMoreMediaInfo(Values& mediaInfo)
{
    mediaInfo.push_back(Value(_devicePath, "devicePath"));
    mediaInfo.push_back(Value(_v4l2Index, "v4l2Index"));
}

/*************/
void Image_V4L2::registerAttributes()
{
    Image::registerAttributes();

    addAttribute("autosetResolution",
        [&](const Values& args) {
            _autosetResolution = static_cast<bool>(args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {(int)_autosetResolution}; },
        {'n'});
    setAttributeParameter("autosetResolution", true, true);

    addAttribute("doCapture",
        [&](const Values& args) {
            if (args[0].as<int>() == 1)
                return doCapture();
            else
                stopCapture();

            return true;
        },
        [&]() -> Values { return {(int)_capturing}; },
        {'n'});
    setAttributeParameter("doCapture", true, true);

    addAttribute("captureSize",
        [&](const Values& args) {
            auto isCapturing = _capturing;
            if (isCapturing)
                stopCapture();

            _outputWidth = std::max(320, args[0].as<int>());
            _outputHeight = std::max(240, args[1].as<int>());

            if (isCapturing)
                doCapture();

            return true;
        },
        [&]() -> Values {
            return {_outputWidth, _outputHeight};
        },
        {'n', 'n'});
    setAttributeParameter("captureSize", true, true);

    addAttribute("device",
        [&](const Values& args) {
            auto path = args[0].as<string>();
            auto index = -1;

            try
            {
                index = stoi(path);
            }
            catch (...)
            {
            }

            auto isCapturing = _capturing;
            if (isCapturing)
                stopCapture();

            if (index != -1) // Only the index is specified
                _devicePath = "/dev/video" + to_string(index);
            else if (path.empty() || path[0] != '/') // No path specified, or an invalid one
                _devicePath = "";
            else
                _devicePath = path;

            _capabilitiesEnumerated = false;

            if (isCapturing)
                doCapture();

            return true;
        },
        [&]() -> Values { return {_devicePath}; },
        {'s'});
    setAttributeParameter("device", true, true);

    addAttribute("index",
        [&](const Values& args) {
            _v4l2Index = std::max(args[0].as<int>(), 0);
            return true;
        },
        [&]() -> Values { return {_v4l2Index}; },
        {'n'});
    setAttributeParameter("index", true, true);
    setAttributeDescription("index", "Set the input index for the selected V4L2 capture device");

    addAttribute("sourceFormat", [&](const Values&) { return true; }, [&]() -> Values { return {_sourceFormatAsString}; }, {});
    setAttributeParameter("sourceFormat", true, true);

    addAttribute("pixelFormat",
        [&](const Values& args) {
            auto isCapturing = _capturing;
            if (isCapturing)
                stopCapture();

            auto format = args[0].as<string>();
            if (format == "RGB")
                _outputPixelFormat = V4L2_PIX_FMT_RGB24;
            else if (format == "YUYV")
                _outputPixelFormat = V4L2_PIX_FMT_YUYV;
            else
                _outputPixelFormat = V4L2_PIX_FMT_RGB24;

            if (isCapturing)
                doCapture();

            return true;
        },
        [&]() -> Values {
            string format;
            switch (_outputPixelFormat)
            {
            default:
                format = "RGB";
                break;
            case V4L2_PIX_FMT_RGB24:
                format = "RGB";
                break;
            case V4L2_PIX_FMT_YUYV:
                format = "YUYV";
                break;
            }
            return {format};
        },
        {'s'});
    setAttributeParameter("pixelFormat", true, true);
    setAttributeDescription("pixelFormat", "Set the desired output format, either RGB or YUYV");
}

} // namespace Splash
