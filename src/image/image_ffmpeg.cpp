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
#include "./utils/osutils.h"
#include "./utils/log.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

/*************/
Image_FFmpeg::Image_FFmpeg(RootObject* root)
    : Image(root)
{
    init();
}

/*************/
Image_FFmpeg::~Image_FFmpeg()
{
    freeFFmpegObjects();
}

/*************/
void Image_FFmpeg::init()
{
    _type = "image_ffmpeg";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    av_register_all();
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
bool Image_FFmpeg::read(const string& filename)
{
    // First: cleanup
    freeFFmpegObjects();

    if (avformat_open_input(&_avContext, filename.c_str(), nullptr, nullptr) != 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't read file " << filename << Log::endl;
        return false;
    }

    if (avformat_find_stream_info(_avContext, NULL) < 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't retrieve information for file " << filename << Log::endl;
        avformat_close_input(&_avContext);
        return false;
    }

    Log::get() << Log::MESSAGE << "Image_FFmpeg::" << __FUNCTION__ << " - Successfully loaded file " << filename << Log::endl;
    av_dump_format(_avContext, 0, filename.c_str(), 0);

#if HAVE_LINUX
    // Give the kernel hints about how to read the file
    auto fd = Utils::getFileDescriptorForOpenedFile(filename);
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
    _videoDisplayThread = thread([&]() { videoDisplayLoop(); });
#if HAVE_PORTAUDIO
    _audioThread = thread([&]() { audioLoop(); });
#endif
    _readLoopThread = thread([&]() { readLoop(); });

    return true;
}

/*************/
string Image_FFmpeg::tagToFourCC(unsigned int tag)
{
    string fourcc;
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

    _speaker = unique_ptr<Speaker>(new Speaker());
    if (!_speaker)
        return false;

    _speaker->setParameters(audioCodecContext->channels, audioCodecContext->sample_rate, format, _audioDeviceOutput);
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

    videoCodecContext->thread_count = min(Utils::getCoreCount(), 16);
    auto videoCodec = avcodec_find_decoder(videoCodecContext->codec_id);
    auto isHap = false;

    // Check whether the video codec only has intra frames
    auto desc = avcodec_descriptor_get(videoCodecContext->codec_id);
    if (desc)
        _intraOnly = !!(desc->props & AV_CODEC_PROP_INTRA_ONLY);
    else
        _intraOnly = false; // We don't know, so we consider it's not

    auto fourcc = tagToFourCC(videoCodecContext->codec_tag);
    if (fourcc.find("Hap") != string::npos)
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
        AVCodec* audioCodec{nullptr};
        auto audioCodecParameters = _avContext->streams[_audioStreamIndex]->codecpar;
        if (avcodec_parameters_to_context(audioCodecContext, audioCodecParameters) < 0)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Unable to create an audio context from the codec parameters for file " << _filepath << Log::endl;
            return;
        }

        audioCodec = avcodec_find_decoder(audioCodecContext->codec_id);

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
    vector<unsigned char> buffer(numBytes);

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

    AVPacket packet;
    av_init_packet(&packet);

    _videoTimeBase = (double)videoStream->time_base.num / (double)videoStream->time_base.den;

    // This implements looping
    do
    {
        _startTime = Timer::getTime();

        auto shouldContinueLoop = [&]() -> bool {
            lock_guard<mutex> lock(_videoSeekMutex);
            return _continueRead && av_read_frame(_avContext, &packet) >= 0;
        };

        while (shouldContinueLoop())
        {
            // Reading the video
            if (packet.stream_index == _videoStreamIndex && _videoSeekMutex.try_lock())
            {
                auto img = unique_ptr<ImageBuffer>();
                uint64_t timing = 0;
                bool hasFrame = false;

                //
                // If the codec is handled by FFmpeg
                if (!isHap)
                {
                    auto frameFinished = false;
                    if (avcodec_send_packet(videoCodecContext, &packet) < 0)
                        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while decoding a frame in file " << _filepath << Log::endl;
                    if (avcodec_receive_frame(videoCodecContext, frame) == 0)
                        frameFinished = true;

                    if (frameFinished)
                    {
                        sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize, 0, videoCodecContext->height, rgbFrame->data, rgbFrame->linesize);

                        ImageBufferSpec spec(videoCodecContext->width, videoCodecContext->height, 3, 16, ImageBufferSpec::Type::UINT8, "YUYV");
                        img.reset(new ImageBuffer(spec));

                        unsigned char* pixels = reinterpret_cast<unsigned char*>(img->data());
                        copy(buffer.begin(), buffer.end(), pixels);

                        if (packet.pts != AV_NOPTS_VALUE)
                            timing = static_cast<uint64_t>((double)av_frame_get_best_effort_timestamp(frame) * _videoTimeBase * 1e6);
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
                    if (hapDecodeFrame(packet.data, packet.size, nullptr, 0, textureFormat))
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
                            av_packet_unref(&packet);
                            return;
                        }

                        spec.format = {textureFormat};
                        img.reset(new ImageBuffer(spec));

                        unsigned long outputBufferBytes = spec.width * spec.height * spec.channels;

                        if (hapDecodeFrame(packet.data, packet.size, img->data(), outputBufferBytes, textureFormat))
                        {
                            if (packet.pts != AV_NOPTS_VALUE)
                                timing = static_cast<uint64_t>((double)packet.pts * _videoTimeBase * 1e6);
                            else
                                timing = 0.0;

                            hasFrame = true;
                        }
                    }
                }

                int64_t totalBufferSize = 0;
                {
                    lock_guard<mutex> lockFrames(_videoQueueMutex);
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
                av_packet_unref(&packet);

                // Do not store more than a few frames in memory
                // _maximumBufferSize is divided by 2 as another frame queue is held by the display loop
                while (timedFramesBuffered > 0 && totalBufferSize > _maximumBufferSize / 2 && _continueRead)
                {
                    this_thread::sleep_for(chrono::milliseconds(5));
                    lock_guard<mutex> lockSeek(_videoSeekMutex);
                    lock_guard<mutex> lockQueue(_videoQueueMutex);
                    timedFramesBuffered = _timedFrames.size();
                }
            }
#if HAVE_PORTAUDIO
            // Reading the audio
            else if (packet.stream_index == _audioStreamIndex && audioCodecContext)
            {
                if (avcodec_send_packet(audioCodecContext, &packet) < 0)
                    Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while decoding an audio frame in file " << _filepath << Log::endl;
                uint64_t timing = (double)packet.pts * _audioTimeBase * 1e6;
                av_packet_unref(&packet);

                while (avcodec_receive_frame(audioCodecContext, frame) == 0)
                {
                    // Check whether we were asked to connect to another output
                    if (audioCodecContext && _audioDeviceOutputUpdated)
                    {
                        setupAudioOutput(audioCodecContext);
                        _audioDeviceOutputUpdated = false;
                    }

                    size_t dataSize = av_samples_get_buffer_size(nullptr, audioCodecContext->channels, frame->nb_samples, audioCodecContext->sample_fmt, 1);
                    auto buffer = ResizableArray<uint8_t>(dataSize);
                    auto linesize = dataSize / audioCodecContext->channels;
                    if (_planar)
                        for (int c = 0; c < audioCodecContext->channels; ++c)
                            copy(frame->extended_data[c], frame->extended_data[c] + linesize, buffer.data() + c * linesize);
                    else
                        copy(frame->extended_data[0], frame->extended_data[0] + dataSize, buffer.data());

                    TimedAudioFrame timedFrame;
                    timedFrame.frame = std::move(buffer);
                    timedFrame.timing = timing;
                    lock_guard<mutex> lockAudio(_audioMutex);
                    _audioQueue.push_back(std::move(timedFrame));

                    av_frame_unref(frame);
                }
            }
#endif
            else
            {
                av_packet_unref(&packet);
            }
        }

        // This prevents looping to happen before the queue has been consumed
        lock_guard<mutex> lockEnd(_videoEndMutex);
        // Seek to the beginning, or whatever time is set in _trimStart
        seek(_trimStart);
    } while (_loopOnVideo && _continueRead);

    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
    if (!isHap)
        sws_freeContext(swsContext);
    avcodec_close(videoCodecContext);
    _videoStreamIndex = -1;

#if HAVE_PORTAUDIO
    if (audioCodecContext)
        avcodec_close(audioCodecContext);
    _audioStreamIndex = -1;
#endif
}

#if HAVE_PORTAUDIO
/*************/
void Image_FFmpeg::audioLoop()
{
    while (_continueRead)
    {
        auto localQueue = deque<TimedAudioFrame>();
        {
            lock_guard<mutex> lockAudio(_audioMutex);
            std::swap(localQueue, _audioQueue);
        }

        if (localQueue.empty())
            this_thread::sleep_for(chrono::milliseconds(10));

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
                this_thread::sleep_for(chrono::milliseconds(5));
                currentTime = Timer::getTime() - _startTime;
            }

            _speaker->addToQueue(localQueue[0].frame);

            localQueue.pop_front();
        }
    }
}
#endif

/*************/
void Image_FFmpeg::seek(float seconds)
{
    if (!_avContext)
        return;

    lock_guard<mutex> lock(_videoSeekMutex);

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
        lock_guard<mutex> lockQueue(_videoQueueMutex);
        // As seeking will no necessarily go to the desired timestamp, but to the closest i-frame,
        // we will set _startTime at the next frame in the videoDisplayLoop
        _startTime = -1;
        _timedFrames.clear();
#if HAVE_PORTAUDIO
        if (_speaker)
            _speaker->clearQueue();
#endif
    }
}

/*************/
void Image_FFmpeg::seek_async(float seconds)
{
    _seekFuture = async(launch::async, [=]() {
        seek(seconds);
        _timeJump = false;
    });
}

/*************/
void Image_FFmpeg::videoDisplayLoop()
{
    while (_continueRead)
    {
        auto localQueue = deque<TimedFrame>();
        if (!_timedFrames.empty())
        {
            lock_guard<mutex> lockFrames(_videoQueueMutex);
            std::swap(localQueue, _timedFrames);
            _framesSize.clear();
        }

        // This sets the start time after a seek
        if (!localQueue.empty() && _startTime == -1)
            _startTime = Timer::getTime() - localQueue[0].timing;

        lock_guard<mutex> lockEnd(_videoEndMutex);
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
            float seekTiming = _intraOnly ? 1.f : 3.f; // Maximum diff for seek to happen when synced to a master clock

            int64_t clockAsMs;
            bool clockIsPaused{false};
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
                    this_thread::sleep_for(chrono::milliseconds(2));
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
                if (_trimEnd > _trimStart && (timedFrame.timing / 1e6 > _trimEnd || timedFrame.timing / 1e6 < _trimStart))
                {
                    if (!_timeJump)
                    {
                        _timeJump = true;
                        localQueue.clear();
                        seek_async(_trimStart);
                    }
                    continue;
                }

                // Compute the difference between next frame and the current clock
                int64_t waitTime = timedFrame.timing - _currentTime;

                // If the gap is too big, we seek through the video
                if (abs(waitTime / 1e6) > seekTiming)
                {
                    if (!_timeJump) // We do not want more than one jump at a time...
                    {
                        _timeJump = true;
                        _elapsedTime = _currentTime / 1e6;
                        localQueue.clear();
                        seek_async(_elapsedTime);
                    }
                    continue;
                }

                // Otherwise, wait for the right time to display the frame
                if (waitTime > 2e3) // we don't wait if the frame is due for the next few ms
                    this_thread::sleep_for(chrono::microseconds(waitTime));

                _elapsedTime = timedFrame.timing;

                lock_guard<shared_timed_mutex> lock(_writeMutex);
                if (!_bufferImage)
                    _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer());
                std::swap(_bufferImage, timedFrame.frame);
                _imageUpdated = true;
                updateTimestamp();
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

    addAttribute("bufferSize",
        [&](const Values& args) {
            int64_t sizeMB = max(16, args[0].as<int>());
            _maximumBufferSize = sizeMB * (int64_t)1048576;
            return true;
        },
        [&]() -> Values { return {_maximumBufferSize / (int64_t)1048576}; },
        {'n'});
    setAttributeParameter("bufferSize", true, true);
    setAttributeDescription("bufferSize", "Set the maximum buffer size for the video (in MB)");

    addAttribute("duration",
        [&](const Values&) { return false; },
        [&]() -> Values {
            if (_avContext == nullptr)
                return {0.f};

            return {getMediaDuration()};
        });
    setAttributeParameter("duration", false, true);

#if HAVE_PORTAUDIO
    addAttribute("audioDeviceOutput",
        [&](const Values& args) {
            _audioDeviceOutput = args[0].as<string>();
            _audioDeviceOutputUpdated = true;
            return true;
        },
        [&]() -> Values { return {_audioDeviceOutput}; },
        {'s'});
    setAttributeParameter("audioDeviceOutput", true, true);
    setAttributeDescription("audioDeviceOutput", "Name of the audio device to send the audio to (i.e. Jack writable client)");
#endif

    addAttribute("loop",
        [&](const Values& args) {
            _loopOnVideo = (bool)args[0].as<int>();
            return true;
        },
        [&]() -> Values {
            int loop = _loopOnVideo;
            return {loop};
        },
        {'n'});
    setAttributeParameter("loop", true, true);

    addAttribute("remaining",
        [&](const Values&) { return false; },
        [&]() -> Values {
            if (_avContext == nullptr)
                return {0.f};

            float duration = std::max(0.f, getMediaDuration() - static_cast<float>(_elapsedTime) / 1e6f);
            return {duration};
        });
    setAttributeParameter("remaining", false, true);

    addAttribute("pause",
        [&](const Values& args) {
            _paused = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_paused}; },
        {'n'});
    setAttributeParameter("pause", false, true);

    addAttribute("seek",
        [&](const Values& args) {
            float seconds = args[0].as<float>();
            seek_async(seconds);
            _seekTime = seconds;
            return true;
        },
        [&]() -> Values { return {_seekTime}; },
        {'n'});
    setAttributeParameter("seek", false, true);
    setAttributeDescription("seek", "Change the read position in the video file");

    addAttribute("trim",
        [&](const Values& args) {
            float start = args[0].as<float>();
            float end = args[1].as<float>();

            if (start > end)
                return false;

            if (start < 0.f)
                start = 0.f;

            auto mediaDuration = getMediaDuration();
            if (end > mediaDuration)
                end = mediaDuration;

            _trimStart = start;
            _trimEnd = end;

            return true;
        },
        [&]() -> Values {
            return {_trimStart, _trimEnd};
        },
        {'n', 'n'});
    setAttributeParameter("trim", true, true);
    setAttributeDescription("trim", "Trim the video by setting the start and end times");

    addAttribute("useClock",
        [&](const Values& args) {
            _useClock = args[0].as<int>();
            if (!_useClock)
                _clockTime = -1;
            else
                _clockTime = 0;

            return true;
        },
        [&]() -> Values { return {(int)_useClock}; },
        {'n'});
    setAttributeParameter("useClock", true, true);

    addAttribute("videoFormat",
        [&](const Values&) {
            // Video format string cannot be set from outside this class
            return true;
        },
        [&]() -> Values { return {_videoFormat}; },
        {'s'});
    setAttributeParameter("videoFormat", false, true);

    addAttribute("timeShift",
        [&](const Values& args) {
            _shiftTime = args[0].as<float>();

            return true;
        },
        {'n'});
}

} // end of namespace
