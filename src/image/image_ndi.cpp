#include "./image/image_ndi.h"

#include <algorithm>
#include <mutex>
#include <span>

#if HAVE_LINUX
#include <dlfcn.h>
#include <stdlib.h>
#elif HAVE_WINDOWS
#include <windows.h>
#endif

#include "utils/log.h"

namespace Splash
{

std::mutex Image_NDI::_ndiFindMutex;
Image_NDI::NDILoadStatus Image_NDI::_ndiLoaded = Image_NDI::NDILoadStatus::NotLoaded;
NDIlib_v4* Image_NDI::_ndiLib = nullptr;
NDIlib_find_instance_t Image_NDI::_ndiFind;

std::mutex Image_NDI::_ndiSourcesMutex;
std::vector<NDIlib_source_t> Image_NDI::_ndiSources;

/*************/
Image_NDI::Image_NDI(RootObject* root)
    : Image(root)
{
    _type = "image_ndi";
    registerAttributes();

    if (!loadNDILib())
        return;

    // We must run one periodic task per NDI source, as
    // each one is destroyed along the owner.
    addPeriodicTask(
        "findNDISources",
        [=]() {
            auto lock = std::unique_lock<std::mutex>(_ndiFindMutex, std::try_to_lock);
            if (!lock.owns_lock())
                return;

            // Add a try lock for the find process
            // This periodic task is ran by all instances of Image_NDI, but
            // there is only one _ndiFind
            if (!_ndiFind)
            {
                _ndiFind = _ndiLib->find_create_v2(nullptr);
                if (!_ndiFind)
                    return;
            }

            if (!_ndiLib->find_wait_for_sources(_ndiFind, 1000))
                return;

            uint32_t sourceCount;
            const NDIlib_source_t* sources = _ndiLib->find_get_current_sources(_ndiFind, &sourceCount);

            std::vector<NDIlib_source_t> ndiSources;
            for (uint32_t i = 0; i < sourceCount; ++i)
                ndiSources.push_back(sources[i]);

            auto lockSources = std::unique_lock<std::mutex>(_ndiSourcesMutex);
            _ndiSources.swap(ndiSources);
        },
        10000);

    // Create the NDI receiver
    _ndiReceiver = _ndiLib->recv_create_v3(nullptr);
}

/*************/
Image_NDI::~Image_NDI()
{
    _receive = false;
    if (_recvThread.joinable())
        _recvThread.join();

    if (_ndiLib)
        _ndiLib->recv_destroy(_ndiReceiver);
}

/*************/
bool Image_NDI::read(const std::string& sourceName)
{
    if (sourceName.empty())
        return false;
    _sourceName = sourceName;
    return connectByName(_sourceName);
}

/*************/
bool Image_NDI::loadNDILib()
{
    if (_ndiLoaded == NDILoadStatus::Loaded)
    {
        assert(_ndiLib != nullptr);
        // NDI has already been loaded
        return true;
    }
    else if (_ndiLoaded == NDILoadStatus::Failed)
    {
        // NDI has already been marked as failing to load, we
        // don't want to repeat this message so we exit immediately
        return false;
    }

    assert(_ndiLib == nullptr);
#if HAVE_LINUX
    std::string NDIPath;
    const char* NDIRuntimeFolder = getenv(NDILIB_REDIST_FOLDER);

    if (NDIRuntimeFolder)
        NDIPath = std::string(NDIRuntimeFolder).append(NDILIB_LIBRARY_NAME);
    else
        NDIPath = NDILIB_LIBRARY_NAME;

    void* ndiLibFile = dlopen(NDIPath.c_str(), RTLD_LOCAL | RTLD_LAZY);
    if (!ndiLibFile)
    {
        _ndiLoaded = NDILoadStatus::Failed;
        Log::get() << Log::WARNING << "Image_NDI::" << __FUNCTION__ << " - Error while loading the NDI library. Check that it is installed." << Log::endl;
        return false;
    }

    const NDIlib_v4* (*NDIlib_v4_load)(void) = nullptr;
    *((void**)&NDIlib_v4_load) = dlsym(ndiLibFile, "NDIlib_v4_load");
    if (!NDIlib_v4_load)
    {
        _ndiLoaded = NDILoadStatus::Failed;
        dlclose(ndiLibFile);
        Log::get() << Log::WARNING << "Image_NDI::" << __FUNCTION__ << " - Error while loading the NDI symbols. Check that the correct NDI v6 is installed" << Log::endl;
        return false;
    }
#elif HAVE_WINDOWS
    const char* NDIRuntimeFolder = getenv(NDILIB_REDIST_FOLDER);
    if (!NDIRuntimeFolder)
    {
        _ndiLoaded = NDILoadStatus::Failed;
        Log::get() << Log::WARNING << "Image_NDI::" << __FUNCTION__ << " - Error while loading the NDI library. Check that it is installed." << Log::endl;
        return false;
    }

    const std::string NDIPath = std::string(NDIRuntimeFolder) + "\\" + NDILIB_LIBRARY_NAME;
    HMODULE ndiLibFile = LoadLibraryA(NDIPath.c_str());
    if (!ndiLibFile)
    {
        _ndiLoaded = NDILoadStatus::Failed;
        Log::get() << Log::WARNING << "Image_NDI::" << __FUNCTION__ << " - Error while loading the NDI library. Check that it is installed." << Log::endl;
        return false;
    }

    const NDIlib_v4* (*NDIlib_v4_load)(void) = nullptr;
    *((FARPROC*)&NDIlib_v4_load) = GetProcAddress(ndiLibFile, "NDIlib_v4_load");

    if (!NDIlib_v4_load)
    {
        _ndiLoaded = NDILoadStatus::Failed;
        FreeLibrary(ndiLibFile);
        Log::get() << Log::WARNING << "Image_NDI::" << __FUNCTION__ << " - Error while loading the NDI symbols. Check that the correct NDI v6 is installed" << Log::endl;
        return false;
    }
#endif

    _ndiLib = const_cast<NDIlib_v4*>(NDIlib_v4_load());
    if (!_ndiLib->initialize())
    {
        _ndiLoaded = NDILoadStatus::Failed;
        Log::get() << Log::WARNING << "Image_NDI::" << __FUNCTION__ << " - Error while initializing the NDI library.." << Log::endl;
    }

    _ndiLoaded = NDILoadStatus::Loaded;
    Log::get() << Log::MESSAGE << "Image_NDI::" << __FUNCTION__ << " - NDI library loaded." << Log::endl;

    return true;
}

/*************/
bool Image_NDI::connectByName(std::string_view name)
{
    assert(_ndiReceiver != nullptr);

    // Stop a previous receiving thread
    if (_recvThread.joinable())
    {
        _receive = false;
        _recvThread.join();
    }

    // Connect to the source
    std::unique_lock<std::mutex> lock(_ndiSourcesMutex);
    const auto sourceIt = std::find_if(_ndiSources.begin(), _ndiSources.end(), [&](const auto& a) { return name == a.p_ndi_name; });
    if (sourceIt != _ndiSources.end())
    {
        // Connect to an existing source
        _ndiLib->recv_connect(_ndiReceiver, &*sourceIt);
    }
    else
    {
        // Connect to a source not yet available
        NDIlib_source_t source;
        source.p_ndi_name = name.data();
        source.p_url_address = nullptr;
        _ndiLib->recv_connect(_ndiReceiver, &source);
    }
    lock.unlock();

    // Run a thread to grab video frames
    _recvThread = std::thread([=, this]() {
        _receive = true;
        while (_receive)
        {
            NDIlib_video_frame_v2_t videoFrame;
            switch (_ndiLib->recv_capture_v3(_ndiReceiver, &videoFrame, nullptr, nullptr, 1000))
            {
            default:
                break;
            case NDIlib_frame_type_video:
                auto readBuffer = readNDIVideoFrame(videoFrame);
                _ndiLib->recv_free_video_v2(_ndiReceiver, &videoFrame);

                std::lock_guard<Spinlock> updateLock(_updateMutex);
                if (!_bufferImage)
                    _bufferImage = std::make_unique<ImageBuffer>();
                std::swap(*(_bufferImage), readBuffer);
                _bufferImageUpdated = true;
                updateTimestamp();
                break;
            }
        }
    });

    return true;
}

/*************/
ImageBuffer Image_NDI::readNDIVideoFrame(const NDIlib_video_frame_v2_t& ndiFrame)
{
    const auto width = ndiFrame.xres;
    const auto height = ndiFrame.yres;

    ImageBuffer readBuffer;
    switch (ndiFrame.FourCC)
    {
    default:
    {
        static bool warningSent = false;
        if (!warningSent)
        {
            warningSent = true;
            Log::get() << Log::WARNING << "Image_NDI::" << __FUNCTION__ << " - Video frame FourCC format not supported" << Log::endl;
        }
        break;
    }
    case NDIlib_FourCC_type_UYVY:
    {
        // YCbCr color space using 4:2:2.
        auto specs = ImageBufferSpec(width, height, 2, 16, ImageBufferSpec::Type::UINT8, "UYVY");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto input = ndiFrame.p_data;
        const auto inputStride = ndiFrame.line_stride_in_bytes;
        auto output = readBuffer.data();
        std::copy(input, input + inputStride * height, output);
        break;
    }
    case NDIlib_FourCC_type_UYVA:
    {
        // YCbCr + Alpha color space, using 4:2:2:4.
        // In memory there are two separate planes. The first is a regular
        // UYVY 4:2:2 buffer. Immediately following this in memory is a
        // alpha channel buffer.
        auto specs = ImageBufferSpec(width, height, 2, 16, ImageBufferSpec::Type::UINT8, "UYVY");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto input = ndiFrame.p_data;
        const auto inputStride = ndiFrame.line_stride_in_bytes * 3 / 4; // Drop alpha channel
        auto output = readBuffer.data();
        std::copy(input, input + inputStride * height, output);
        break;
    }
    case NDIlib_FourCC_type_P216:
    {
        // YCbCr color space using 4:2:2 in 16bpp.
        // In memory this is a semi-planar format. This is identical to a 16bpp version of the NV16 format.
        // The first buffer is a 16bpp luminance buffer.
        // Immediately after this is an interleaved buffer of 16bpp Cb, Cr pairs.
        auto specs = ImageBufferSpec(width, height, 2, 16, ImageBufferSpec::Type::UINT8, "UYVY");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto inputStride = ndiFrame.line_stride_in_bytes;
        const auto input = std::span(reinterpret_cast<uint16_t*>(ndiFrame.p_data), inputStride * height);
        auto output = std::span(readBuffer.data(), readBuffer.getSize());
        cvtP216toUYVY(input, output);
        break;
    }
    case NDIlib_FourCC_type_PA16:
    {
        // YCbCr color space with an alpha channel, using 4:2:2:4.
        // In memory this is a semi-planar format.
        // The first buffer is a 16bpp luminance buffer.
        // Immediately after this is an interleaved buffer of 16bpp Cb, Cr pairs.
        // Immediately after is a single buffer of 16bpp alpha channel.
        auto specs = ImageBufferSpec(width, height, 2, 16, ImageBufferSpec::Type::UINT8, "UYVY");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto inputStride = ndiFrame.line_stride_in_bytes * 3 / 4; // Drop alpha channel
        const auto input = std::span(reinterpret_cast<uint16_t*>(ndiFrame.p_data), inputStride * height);
        auto output = std::span(readBuffer.data(), readBuffer.getSize());
        cvtP216toUYVY(input, output);
        break;
    }
    case NDIlib_FourCC_type_YV12:
    {
        // Planar 8bit 4:2:0 video format.
        // The first buffer is an 8bpp luminance buffer.
        // Immediately following this is a 8bpp Cr buffer.
        // Immediately following this is a 8bpp Cb buffer.
        auto specs = ImageBufferSpec(width, height, 2, 16, ImageBufferSpec::Type::UINT8, "UYVY");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto inputStride = ndiFrame.line_stride_in_bytes;
        const auto input = std::span(ndiFrame.p_data, inputStride * height);
        auto output = std::span(readBuffer.data(), readBuffer.getSize());
        cvtYV12toUYVY(input, output, width, height);
        break;
    }
    case NDIlib_FourCC_type_I420:
    {
        // The first buffer is an 8bpp luminance buffer.
        // Immediately following this is a 8bpp Cb buffer.
        // Immediately following this is a 8bpp Cr buffer.
        auto specs = ImageBufferSpec(width, height, 2, 16, ImageBufferSpec::Type::UINT8, "UYVY");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto inputStride = ndiFrame.line_stride_in_bytes;
        const auto input = std::span(ndiFrame.p_data, inputStride * height);
        auto output = std::span(readBuffer.data(), readBuffer.getSize());
        cvtI420toUYVY(input, output, width, height);
        break;
    }
    case NDIlib_FourCC_type_NV12:
    {
        // Planar 8bit 4:2:0 video format.
        // The first buffer is an 8bpp luminance buffer.
        // Immediately following this is in interleaved buffer of 8bpp Cb, Cr pairs
        auto specs = ImageBufferSpec(width, height, 2, 16, ImageBufferSpec::Type::UINT8, "UYVY");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto inputStride = ndiFrame.line_stride_in_bytes;
        const auto input = std::span(ndiFrame.p_data, inputStride * height);
        auto output = std::span(readBuffer.data(), readBuffer.getSize());
        cvtNV12toUYVY(input, output, width, height);
        break;
    }
    case NDIlib_FourCC_type_BGRA:
    {
        // Planar 8bit, 4:4:4:4 video format.
        // Color ordering in memory is blue, green, red, alpha
        auto specs = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "BGRA");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto input = ndiFrame.p_data;
        const auto inputStride = ndiFrame.line_stride_in_bytes;
        auto output = readBuffer.data();
        std::copy(input, input + inputStride * height, output);
        break;
    }
    case NDIlib_FourCC_type_BGRX:
    {
        // Planar 8bit, 4:4:4 video format, packed into 32bit pixels.
        // Color ordering in memory is blue, green, red, 255
        auto specs = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "BGRA");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto input = ndiFrame.p_data;
        const auto inputStride = ndiFrame.line_stride_in_bytes;
        auto output = readBuffer.data();
        std::copy(input, input + inputStride * height, output);
        break;
    }
    case NDIlib_FourCC_type_RGBA:
    {
        // Planar 8bit, 4:4:4:4 video format.
        // Color ordering in memory is red, green, blue, alpha
        auto specs = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto input = ndiFrame.p_data;
        const auto inputStride = ndiFrame.line_stride_in_bytes;
        auto output = readBuffer.data();
        std::copy(input, input + inputStride * height, output);
        break;
    }
    case NDIlib_FourCC_type_RGBX:
    {
        // Planar 8bit, 4:4:4 video format, packed into 32bit pixels.
        // Color ordering in memory is red, green, blue, 255.
        auto specs = ImageBufferSpec(width, height, 4, 32, ImageBufferSpec::Type::UINT8, "RGBA");
        specs.timestamp = ndiFrame.timecode;
        readBuffer = ImageBuffer(specs);

        const auto input = ndiFrame.p_data;
        const auto inputStride = ndiFrame.line_stride_in_bytes;
        auto output = readBuffer.data();
        std::copy(input, input + inputStride * height, output);
        break;
    }
    }

    return readBuffer;
}

/*************/
void Image_NDI::registerAttributes()
{
    addAttribute(
        "source",
        [&](const Values& args) {
            _sourceName = args[0].as<std::string>();
            if (_ndiLib)
                return connectByName(_sourceName);
            else
                return true;
        },
        [&]() -> Values {
            Values retValues = {_sourceName};
            std::unique_lock<std::mutex> lock(_ndiSourcesMutex);
            for (const auto& source : _ndiSources)
                retValues.push_back(std::string(source.p_ndi_name));

            return retValues;
        },
        {'s'},
        true);
    setAttributeDescription("source", "NDI® source");

    addAttribute("NDI®", [&]() -> Values { return {"https://ndi.video"}; });
    setAttributeDescription("NDI®", "Link to NDI® website\nNDI® is a registered trademark of Vizrt NDI AB");
}

} // namespace Splash
