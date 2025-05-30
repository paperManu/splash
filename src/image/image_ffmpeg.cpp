#include "./image/image_ffmpeg.h"

#include <chrono>
#include <functional>
#include <future>
#include <numeric>
#if HAVE_LINUX
#include <fcntl.h>
#endif
#include <fstream>
#include <hap.h>

#include "./utils/cgutils.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

namespace chrono = std::chrono;

namespace Splash
{

/*************/
Image_FFmpeg::Image_FFmpeg(RootObject* root)
    : Image(root)
{
    _type = "image_ffmpeg";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;
}

/*************/
Image_FFmpeg::~Image_FFmpeg()
{
    freeFFmpegObjects();
}

/*************/
void Image_FFmpeg::freeFFmpegObjects()
{
    _clockTime = -1;

    if (_continueRead)
    {
        _continueRead = false;
        _readLoopThread.join();
        _videoDisplayThread.join();
#if HAVE_PORTAUDIO
        _audioThread.join();
        if (_speaker)
            _speaker.reset();
#endif
    }

    if (_avContext)
    {
        avformat_close_input(&_avContext);
        _avContext = nullptr;
    }
}

/*************/
float Image_FFmpeg::getMediaDuration() const
{
    if (!_avContext)
        return 0.f;
    return static_cast<float>(_avContext->duration) / static_cast<float>(AV_TIME_BASE);
}

/*************/
bool Image_FFmpeg::read(const std::string& filename)
{
    if (!_root)
        return false;

    const auto filepath = Utils::getFullPathFromFilePath(filename, _root->getConfigurationPath());

    // First: cleanup
    freeFFmpegObjects();

    if (avformat_open_input(&_avContext, filepath.c_str(), nullptr, nullptr) != 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't read file " << filepath << Log::endl;
        return false;
    }

    if (avformat_find_stream_info(_avContext, nullptr) < 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't retrieve information for file " << filepath << Log::endl;
        avformat_close_input(&_avContext);
        return false;
    }

    Log::get() << Log::MESSAGE << "Image_FFmpeg::" << __FUNCTION__ << " - Successfully loaded file " << filepath << Log::endl;
    av_dump_format(_avContext, 0, filepath.c_str(), 0);

#if HAVE_LINUX
    // Give the kernel hints about how to read the file
    auto fd = Utils::getFileDescriptorForOpenedFile(filepath);
    if (fd)
    {
        bool success = true;
        success &= posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED) == 0;
        success &= posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) == 0;
        if (!success)
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not set hints for video file reading access" << Log::endl;
    }
#endif

    // Launch the loops
    _continueRead = true;
    _videoDisplayThread = std::thread([&]() { videoDisplayLoop(); });
#if HAVE_PORTAUDIO
    _audioThread = std::thread([&]() { audioLoop(); });
#endif
    _readLoopThread = std::thread([&]() { readLoop(); });

    return true;
}

/*************/
std::string Image_FFmpeg::tagToFourCC(unsigned int tag)
{
    std::string fourcc;
    fourcc.resize(4);

    unsigned int sum = 0;
    fourcc[3] = char(tag >> 24);
    sum += (fourcc[3]) << 24;

    fourcc[2] = char((tag - sum) >> 16);
    sum += (fourcc[2]) << 16;

    fourcc[1] = char((tag - sum) >> 8);
    sum += (fourcc[1]) << 8;

    fourcc[0] = char(tag - sum);

    return fourcc;
}

#if HAVE_PORTAUDIO
/*************/
bool Image_FFmpeg::setupAudioOutput(AVCodecContext* audioCodecContext)
{
    if (!audioCodecContext)
        return false;

    Sound_Engine::SampleFormat format;
    switch (audioCodecContext->sample_fmt)
    {
    default:
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Unsupported sample format" << Log::endl;
        return false;
    case AV_SAMPLE_FMT_U8:
        format = Sound_Engine::SAMPLE_FMT_U8;
        _planar = false;
        break;
    case AV_SAMPLE_FMT_S16:
        format = Sound_Engine::SAMPLE_FMT_S16;
        _planar = false;
        break;
    case AV_SAMPLE_FMT_S32:
        format = Sound_Engine::SAMPLE_FMT_S32;
        _planar = false;
        break;
    case AV_SAMPLE_FMT_FLT:
        format = Sound_Engine::SAMPLE_FMT_FLT;
        _planar = false;
        break;
    case AV_SAMPLE_FMT_U8P:
        format = Sound_Engine::SAMPLE_FMT_U8P;
        _planar = true;
        break;
    case AV_SAMPLE_FMT_S16P:
        format = Sound_Engine::SAMPLE_FMT_S16P;
        _planar = true;
        break;
    case AV_SAMPLE_FMT_S32P:
        format = Sound_Engine::SAMPLE_FMT_S32P;
        _planar = true;
        break;
    case AV_SAMPLE_FMT_FLTP:
        format = Sound_Engine::SAMPLE_FMT_FLTP;
        _planar = true;
        break;
    }

    _speaker = std::make_unique<Speaker>();
    if (!_speaker)
        return false;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 37, 100)
    _speaker->setParameters(audioCodecContext->channels, audioCodecContext->sample_rate, format, _audioDeviceOutput);
#else
    _speaker->setParameters(audioCodecContext->ch_layout.nb_channels, audioCodecContext->sample_rate, format, _audioDeviceOutput);
#endif

    return true;
}
#endif

/*************/
void Image_FFmpeg::readLoop()
{
    // Find the first video stream
    _videoStreamIndex = -1;
#if HAVE_PORTAUDIO
    _audioStreamIndex = -1;
#endif
    for (uint32_t i = 0; i < _avContext->nb_streams; ++i)
    {
        if (_avContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && _videoStreamIndex < 0)
            _videoStreamIndex = i;
#if HAVE_PORTAUDIO
        else if (_avContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && _audioStreamIndex < 0)
            _audioStreamIndex = i;
#endif
    }

    if (_videoStreamIndex == -1)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - No video stream found in file " << _filepath << Log::endl;
        return;
    }

#if HAVE_PORTAUDIO
    if (_audioStreamIndex == -1)
    {
        Log::get() << Log::MESSAGE << "Image_FFmpeg::" << __FUNCTION__ << " - No audio stream found in file " << _filepath << Log::endl;
    }
#endif

    // Find a video decoder
    auto videoStream = _avContext->streams[_videoStreamIndex];
    auto videoCodecParameters = _avContext->streams[_videoStreamIndex]->codecpar;
    auto videoCodecContext = avcodec_alloc_context3(nullptr);
    if (avcodec_parameters_to_context(videoCodecContext, videoCodecParameters) < 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Unable to create a video context from the codec parameters from file " << _filepath << Log::endl;
        return;
    }

    // Set video format info
    _videoFormat.resize(1024);
    avcodec_string(const_cast<char*>(_videoFormat.data()), _videoFormat.size(), videoCodecContext, 0);

    videoCodecContext->thread_count = std::min(Utils::getCoreCount(), 16);
    auto videoCodec = avcodec_find_decoder(videoCodecContext->codec_id);
    auto isHap = false;

    // Check whether the video codec only has intra frames
    auto desc = avcodec_descriptor_get(videoCodecContext->codec_id);
    if (desc)
        _intraOnly = !!(desc->props & AV_CODEC_PROP_INTRA_ONLY);
    else
        _intraOnly = false; // We don't know, so we consider it's not

    auto fourcc = tagToFourCC(videoCodecContext->codec_tag);
    if (fourcc.find("Hap") != std::string::npos)
    {
        isHap = true;
        _intraOnly = true; // Hap is necessarily intra only
    }
    else if (videoCodec == nullptr)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Video codec not supported for file " << _filepath << Log::endl;
        return;
    }

    if (videoCodec)
    {
        AVDictionary* optionsDict = nullptr;
        if (avcodec_open2(videoCodecContext, videoCodec, &optionsDict) < 0)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not open video codec for file " << _filepath << Log::endl;
            return;
        }
    }

#if HAVE_PORTAUDIO
    // Find an audio decoder
    auto audioCodecContext = avcodec_alloc_context3(nullptr);
    if (_audioStreamIndex >= 0)
    {
        auto audioCodecParameters = _avContext->streams[_audioStreamIndex]->codecpar;
        if (avcodec_parameters_to_context(audioCodecContext, audioCodecParameters) < 0)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Unable to create an audio context from the codec parameters for file " << _filepath << Log::endl;
            return;
        }

        const auto audioCodec = avcodec_find_decoder(audioCodecContext->codec_id);

        if (audioCodec == nullptr)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Audio codec not supported for file " << _filepath << Log::endl;
            audioCodecContext = nullptr;
        }
        else
        {
            AVDictionary* audioOptionsDict = nullptr;
            if (avcodec_open2(audioCodecContext, audioCodec, &audioOptionsDict) < 0)
            {
                Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not open audio codec for file " << _filepath << Log::endl;
                audioCodecContext = nullptr;
            }
        }

        if (audioCodecContext)
            _audioDeviceOutputUpdated = true;

        auto audioStream = _avContext->streams[_audioStreamIndex];
        _audioTimeBase = (double)audioStream->time_base.num / (double)audioStream->time_base.den;
    }
#endif

    // Start reading frames
    AVFrame *frame, *rgbFrame;
    frame = av_frame_alloc();
    rgbFrame = av_frame_alloc();

    if (!frame || !rgbFrame)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while allocating frame structures" << Log::endl;
        return;
    }

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUYV422, videoCodecContext->width, videoCodecContext->height, 1);
    std::vector<unsigned char> buffer(numBytes);

    struct SwsContext* swsContext = nullptr;
    if (!isHap)
    {
        swsContext = sws_getContext(videoCodecContext->width,
            videoCodecContext->height,
            videoCodecContext->pix_fmt,
            videoCodecContext->width,
            videoCodecContext->height,
            AV_PIX_FMT_YUYV422,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr);

        av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer.data(), AV_PIX_FMT_YUYV422, videoCodecContext->width, videoCodecContext->height, 1);
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet)
    {
        Log::get() << Log::ERROR << "Image_FFmpeg::" << __FUNCTION__ << " - Unable to allocate packet for decoding frames" << Log::endl;
        return;
    }

    _videoTimeBase = static_cast<double>(videoStream->time_base.num) / static_cast<double>(videoStream->time_base.den);

    // This implements looping
    _startTime = Timer::getTime();
    while (_continueRead)
    {
        auto shouldContinueLoop = [&]() -> bool {
            std::lock_guard<std::mutex> lock(_videoSeekMutex);
            return _continueRead && av_read_frame(_avContext, packet) >= 0;
        };

        while (shouldContinueLoop())
        {
            // Reading the video
            if (packet->stream_index == _videoStreamIndex && _videoSeekMutex.try_lock())
            {
                auto img = std::unique_ptr<ImageBuffer>();
                uint64_t timing = 0;
                bool hasFrame = false;

                //
                // If the codec is handled by FFmpeg
                if (!isHap)
                {
                    auto frameFinished = false;
                    if (avcodec_send_packet(videoCodecContext, packet) < 0)
                        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while decoding a frame in file " << _filepath << Log::endl;
                    if (avcodec_receive_frame(videoCodecContext, frame) == 0)
                        frameFinished = true;

                    if (frameFinished)
                    {
                        sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize, 0, videoCodecContext->height, rgbFrame->data, rgbFrame->linesize);

                        ImageBufferSpec spec(videoCodecContext->width, videoCodecContext->height, 2, 16, ImageBufferSpec::Type::UINT8, "YUYV");
                        img = std::make_unique<ImageBuffer>(spec);

                        unsigned char* pixels = reinterpret_cast<unsigned char*>(img->data());
                        std::copy(buffer.begin(), buffer.end(), pixels);

                        if (packet->pts != AV_NOPTS_VALUE)
                            timing = static_cast<uint64_t>((double)frame->best_effort_timestamp * _videoTimeBase * 1e6);
                        else
                            timing = 0.0;
                        // This handles repeated frames
                        timing += frame->repeat_pict * _videoTimeBase * 0.5;

                        hasFrame = true;
                    }

                    av_frame_unref(frame);
                }
                //
                // If the codec is marked as Hap / Hap alpha / Hap Q
                else if (isHap)
                {
                    // We are using kind of a hack to store a DXT compressed image in an ImageBuffer
                    // First, we check the texture format type
                    std::string textureFormat;
                    if (hapDecodeFrame(packet->data, packet->size, nullptr, 0, textureFormat))
                    {
                        // Check if we need to resize the reader buffer
                        // We set the size so as to have just enough place for the given texture format
                        ImageBufferSpec spec;
                        if (textureFormat == "RGB_DXT1")
                        {
                            spec = ImageBufferSpec(videoCodecContext->width, (int)(ceil((float)videoCodecContext->height / 2.f)), 1, 8, ImageBufferSpec::Type::UINT8);
                        }
                        else if (textureFormat == "RGBA_DXT5")
                        {
                            spec = ImageBufferSpec(videoCodecContext->width, videoCodecContext->height, 1, 8, ImageBufferSpec::Type::UINT8);
                        }
                        else if (textureFormat == "YCoCg_DXT5")
                        {
                            spec = ImageBufferSpec(videoCodecContext->width, videoCodecContext->height, 1, 8, ImageBufferSpec::Type::UINT8);
                        }
                        else
                        {
                            av_packet_unref(packet);
                            return;
                        }

                        spec.format = {textureFormat};
                        img = std::make_unique<ImageBuffer>(spec);

                        unsigned long outputBufferBytes = spec.width * spec.height * spec.channels;

                        if (hapDecodeFrame(packet->data, packet->size, img->data(), outputBufferBytes, textureFormat))
                        {
                            if (packet->pts != AV_NOPTS_VALUE)
                                timing = static_cast<uint64_t>(static_cast<double>(packet->pts) * _videoTimeBase * 1e6);
                            else
                                timing = 0.0;

                            hasFrame = true;
                        }
                    }
                }

                int64_t totalBufferSize = 0;
                {
                    std::lock_guard<std::mutex> lockFrames(_videoQueueMutex);
                    if (hasFrame)
                    {
                        // Add the frame size to the history
                        _framesSize.push_back(img->getSize());

                        _timedFrames.emplace_back();
                        std::swap(_timedFrames[_timedFrames.size() - 1].frame, img);
                        _timedFrames[_timedFrames.size() - 1].timing = timing;
                    }

                    // Check the current buffer size (sum of all frames in buffer)
                    for (auto& f : _framesSize)
                        totalBufferSize += f;
                }

                int timedFramesBuffered = _timedFrames.size();

                _videoSeekMutex.unlock();
                av_packet_unref(packet);

                // Do not store more than a few frames in memory
                // _maximumBufferSize is divided by 2 as another frame queue is held by the display loop
                while (timedFramesBuffered > 0 && totalBufferSize > _maximumBufferSize / 2 && _continueRead)
                {
                    std::this_thread::sleep_for(chrono::milliseconds(5));
                    std::lock_guard<std::mutex> lockSeek(_videoSeekMutex);
                    std::lock_guard<std::mutex> lockQueue(_videoQueueMutex);
                    timedFramesBuffered = _timedFrames.size();
                }
            }
#if HAVE_PORTAUDIO
            // Reading the audio
            else if (packet->stream_index == _audioStreamIndex && audioCodecContext)
            {
                if (avcodec_send_packet(audioCodecContext, packet) < 0)
                    Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while decoding an audio frame in file " << _filepath << Log::endl;
                uint64_t timing = static_cast<double>(packet->pts) * _audioTimeBase * 1e6;
                av_packet_unref(packet);

                while (avcodec_receive_frame(audioCodecContext, frame) == 0)
                {
                    // Check whether we were asked to connect to another output
                    if (audioCodecContext && _audioDeviceOutputUpdated)
                    {
                        setupAudioOutput(audioCodecContext);
                        _audioDeviceOutputUpdated = false;
                    }

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 37, 100)
                    const auto nb_channels = audioCodecContext->channels;
#else
                    const auto nb_channels = audioCodecContext->ch_layout.nb_channels;
#endif
                    size_t dataSize = av_samples_get_buffer_size(nullptr, nb_channels, frame->nb_samples, audioCodecContext->sample_fmt, 1);
                    auto buffer = ResizableArray<uint8_t>(dataSize);
                    auto linesize = dataSize / nb_channels;
                    if (_planar)
                        for (int c = 0; c < nb_channels; ++c)
                            std::copy(frame->extended_data[c], frame->extended_data[c] + linesize, buffer.data() + c * linesize);
                    else
                        std::copy(frame->extended_data[0], frame->extended_data[0] + dataSize, buffer.data());

                    TimedAudioFrame timedFrame;
                    timedFrame.frame = std::move(buffer);
                    timedFrame.timing = timing;
                    std::lock_guard<std::mutex> lockAudio(_audioMutex);
                    _audioQueue.push_back(std::move(timedFrame));

                    av_frame_unref(frame);
                }
            }
#endif
            else
            {
                av_packet_unref(packet);
            }
        }

        // This prevents looping to happen before the queue has been consumed
        std::lock_guard<std::mutex> lockEnd(_videoEndMutex);

        // If we loop, seek to the beginning, or whatever time is set in _trimStart
        if (_loopOnVideo)
            seek(static_cast<float>(_trimStart) / 1e6, false);
        else
            std::this_thread::sleep_for(chrono::milliseconds(50));
    }

    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
    if (!isHap)
        sws_freeContext(swsContext);
    avcodec_free_context(&videoCodecContext);
    _videoStreamIndex = -1;

#if HAVE_PORTAUDIO
    if (audioCodecContext)
        avcodec_free_context(&audioCodecContext);
    _audioStreamIndex = -1;
#endif
}

#if HAVE_PORTAUDIO
/*************/
void Image_FFmpeg::audioLoop()
{
    while (_continueRead)
    {
        auto localQueue = std::deque<TimedAudioFrame>();
        {
            std::lock_guard<std::mutex> lockAudio(_audioMutex);
            std::swap(localQueue, _audioQueue);
        }

        if (localQueue.empty())
            std::this_thread::sleep_for(chrono::milliseconds(10));

        while (!localQueue.empty() && _continueRead && _speaker)
        {
            auto currentTime = Timer::getTime() - _startTime;

            if (localQueue[0].timing - currentTime < 0)
            {
                localQueue.pop_front();
                continue;
            }

            while (localQueue[0].timing - currentTime > 100000)
            {
                std::this_thread::sleep_for(chrono::milliseconds(5));
                currentTime = Timer::getTime() - _startTime;
            }

            _speaker->addToQueue(localQueue[0].frame);

            localQueue.pop_front();
        }
    }
}
#endif

/*************/
void Image_FFmpeg::seek(float seconds, bool clearQueues)
{
    if (!_avContext)
        return;

    std::lock_guard<std::mutex> lock(_videoSeekMutex);

    int seekFlag = 0;
    if (_elapsedTime > seconds)
        seekFlag = AVSEEK_FLAG_BACKWARD;

    // Prevent seeking outside of the file
    float duration = getMediaDuration();
    if (seconds < 0)
        seconds = 0;
    else if (seconds > duration)
        seconds = duration;

    int frame = static_cast<int>(floor(seconds / _videoTimeBase));
    if (avformat_seek_file(_avContext, _videoStreamIndex, 0, frame, frame, seekFlag) < 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not seek to timestamp " << seconds << Log::endl;
    }
    else
    {
        std::lock_guard<std::mutex> lockQueue(_videoQueueMutex);
        // As seeking will no necessarily go to the desired timestamp, but to the closest i-frame,
        // we will set _startTime at the next frame in the videoDisplayLoop
        _startTime = -1;

        if (clearQueues)
        {
            _timedFrames.clear();
#if HAVE_PORTAUDIO
            if (_speaker)
                _speaker->clearQueue();
#endif
        }
    }
}

/*************/
void Image_FFmpeg::seek_async(float seconds, bool clearQueues)
{
    _seekFuture = std::async(std::launch::async, [=, this]() {
        seek(seconds, clearQueues);
        _timeJump = false;
    });
}

/*************/
void Image_FFmpeg::videoDisplayLoop()
{
    while (_continueRead)
    {
        auto localQueue = std::deque<TimedFrame>();
        if (!_timedFrames.empty())
        {
            std::lock_guard<std::mutex> lockFrames(_videoQueueMutex);
            std::swap(localQueue, _timedFrames);
            _framesSize.clear();
        }
        else
        {
            std::this_thread::sleep_for(chrono::milliseconds(1));
            continue;
        }

        // This sets the start time after a seek
        if (!localQueue.empty() && _startTime == -1)
            _startTime = Timer::getTime() - localQueue[0].timing;

        std::lock_guard<std::mutex> lockEnd(_videoEndMutex);
        while (!localQueue.empty() && _continueRead)
        {
            // If seek, clear the local queue as the frames should not be shown
            if (_startTime == -1)
            {
                localQueue.clear();
                continue;
            }

            //
            // Get the current master and local clocks
            //
            int64_t clockAsMs = 0;
            bool clockIsPaused = false;
            bool useClock = _useClock && Timer::get().getMasterClock<chrono::milliseconds>(clockAsMs, clockIsPaused);
            if (useClock)
            {
                float seconds = (float)clockAsMs / 1e3f + _shiftTime + _trimStart;
                _clockTime = seconds * 1e6;
            }

            //
            // Show the frame at the right timing, according to clocks
            //
            TimedFrame& timedFrame = localQueue[0];
            if (timedFrame.timing != 0ull)
            {
                if (_paused || (clockIsPaused && useClock))
                {
                    _startTime = Timer::getTime() - _currentTime;
                    std::this_thread::sleep_for(chrono::milliseconds(2));
                    continue;
                }
                else if (useClock && _clockTime != -1l)
                {
                    _currentTime = Timer::getTime() - _startTime;
                    auto delta = abs(_currentTime - _clockTime);
                    // If the difference between master clock and local clock is greater than 1.5 frames @30Hz, we adjust local clock
                    if (delta > 50000)
                    {
                        _startTime = Timer::getTime() - _clockTime;
                        _currentTime = _clockTime;
                    }
                }
                else
                {
                    _currentTime = Timer::getTime() - _startTime;
                }

                // If the frame is beyond the trimming end, seek to the trimming start
                if (_trimEnd > _trimStart)
                {
                    if (timedFrame.timing < _trimStart)
                    {
                        auto expectedValue = false;
                        localQueue.clear();
                        if (_timeJump.compare_exchange_strong(expectedValue, true, std::memory_order_acquire))
                            seek_async(static_cast<float>(_trimStart) / 1e6);
                        continue;
                    }
                    else if (timedFrame.timing > _trimEnd)
                    {
                        auto expectedValue = false;
                        localQueue.clear();
                        if (_timeJump.compare_exchange_strong(expectedValue, true, std::memory_order_acquire))
                            seek_async(getMediaDuration());
                        continue;
                    }
                }

                // Compute the difference between next frame and the current clock
                int64_t waitTime = timedFrame.timing - _currentTime;

                // If the gap is too big, we seek through the video
                if (abs(waitTime / 1e6) > (_intraOnly ? 1.f : 3.f)) // Maximum gap duration depending on encoding type (arbitrary values)
                {
                    auto expectedValue = false;
                    if (_timeJump.compare_exchange_strong(expectedValue, true, std::memory_order_acquire))
                    {
                        _elapsedTime = _currentTime / 1e6;
                        localQueue.clear();
                        seek_async(_elapsedTime);
                    }

                    continue;
                }

                // Wait for the right time to display the frame
                if (waitTime > 0)
                    std::this_thread::sleep_for(chrono::microseconds(waitTime));

                _elapsedTime = timedFrame.timing;

                {
                    std::lock_guard<Spinlock> updateLock(_updateMutex);
                    if (!_bufferImage)
                        _bufferImage = std::make_unique<ImageBuffer>();
                    std::swap(_bufferImage, timedFrame.frame);
                    _bufferImageUpdated = true;
                }

                updateTimestamp(_bufferImage->getSpec().timestamp);
            }

            localQueue.pop_front();
        }
    }
}

/*************/
void Image_FFmpeg::updateMoreMediaInfo(Values& mediaInfo)
{
    auto spec = _image->getSpec();
    mediaInfo.push_back(Value(getMediaDuration(), "duration"));
}

/*************/
void Image_FFmpeg::registerAttributes()
{
    Image::registerAttributes();

    addAttribute(
        "bufferSize",
        [&](const Values& args) {
            int64_t sizeMB = std::max(16, args[0].as<int>());
            _maximumBufferSize = sizeMB * (int64_t)1048576;
            return true;
        },
        [&]() -> Values { return {_maximumBufferSize / (int64_t)1048576}; },
        {'i'});
    setAttributeDescription("bufferSize", "Set the maximum buffer size for the video (in MB)");

    addAttribute("duration", [&]() -> Values {
        if (_avContext == nullptr)
            return {0.f};

        return {getMediaDuration()};
    });
    setAttributeDescription("duration", "Duration of the video file");

#if HAVE_PORTAUDIO
    addAttribute(
        "audioDeviceOutput",
        [&](const Values& args) {
            _audioDeviceOutput = args[0].as<std::string>();
            _audioDeviceOutputUpdated = true;
            return true;
        },
        [&]() -> Values { return {_audioDeviceOutput}; },
        {'s'});
    setAttributeDescription("audioDeviceOutput", "Name of the audio device to send the audio to (i.e. Jack writable client)");
#endif

    addAttribute(
        "loop",
        [&](const Values& args) {
            _loopOnVideo = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {static_cast<bool>(_loopOnVideo)}; },
        {'b'});

    addAttribute("elapsed", [&]() -> Values {
        if (_avContext == nullptr)
            return {0.f};

        float duration = std::max(0.f, static_cast<float>(_elapsedTime) / 1e6f);
        return {duration};
    });
    setAttributeDescription("elapsed", "Time elapsed since the beginning of the video");

    addAttribute(
        "pause",
        [&](const Values& args) {
            _paused = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_paused}; },
        {'b'});

    addAttribute(
        "seek",
        [&](const Values& args) {
            float seconds = args[0].as<float>();
            seek_async(seconds);
            _seekTime = seconds;
            return true;
        },
        [&]() -> Values { return {_seekTime}; },
        {'r'});
    setAttributeDescription("seek", "Change the read position in the video file");

    addAttribute(
        "trim",
        [&](const Values& args) {
            auto start = args[0].as<double>();
            auto end = args[1].as<double>();

            if (start > end)
                return false;

            if (start < 0.f)
                start = 0.f;

            auto mediaDuration = getMediaDuration();
            if (end > mediaDuration)
                end = mediaDuration;

            _trimStart = static_cast<int64_t>(start * 1e6);
            _trimEnd = static_cast<int64_t>(end * 1e6);

            return true;
        },
        [&]() -> Values {
            return {static_cast<double>(_trimStart) / 1e6, static_cast<double>(_trimEnd / 1e6)};
        },
        {'r', 'r'});
    setAttributeDescription("trim", "Trim the video by setting the start and end times");

    addAttribute(
        "useClock",
        [&](const Values& args) {
            _useClock = args[0].as<bool>();
            if (!_useClock)
                _clockTime = -1;
            else
                _clockTime = 0;

            return true;
        },
        [&]() -> Values { return {_useClock}; },
        {'b'});

    addAttribute(
        "videoFormat",
        [&](const Values&) {
            // Video format string cannot be set from outside this class
            return true;
        },
        [&]() -> Values { return {_videoFormat}; },
        {'s'});

    addAttribute("timeShift",
        [&](const Values& args) {
            _shiftTime = args[0].as<float>();

            return true;
        },
        {'r'});
}

} // namespace Splash
