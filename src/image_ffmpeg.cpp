#include "image_ffmpeg.h"

#include <chrono>
#include <functional>
#if HAVE_LINUX
#include <fcntl.h>
#endif
#include <hap.h>

#include "./cgUtils.h"
#include "./log.h"
#include "./osUtils.h"
#include "./threadpool.h"
#include "./timer.h"

#if HAVE_FFMPEG_3
#define PIX_FMT_RGB24 AV_PIX_FMT_RGB24
#endif

using namespace std;

namespace Splash
{

/*************/
Image_FFmpeg::Image_FFmpeg(weak_ptr<RootObject> root)
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

    // If the root object weak_ptr is expired, this means that
    // this object has been created outside of a World or Scene.
    // This is used for getting documentation "offline"
    if (_root.expired())
        return;

    av_register_all();
}

/*************/
void Image_FFmpeg::freeFFmpegObjects()
{
    _clockTime = -1;

    _continueRead = false;
    _videoQueueCondition.notify_one();

    if (_videoDisplayThread.joinable())
        _videoDisplayThread.join();
    if (_readLoopThread.joinable())
        _readLoopThread.join();

    if (_avContext)
    {
        avformat_close_input(&_avContext);
        _avContext = nullptr;
    }
}

/*************/
bool Image_FFmpeg::read(const string& filename)
{
    // First: cleanup
    freeFFmpegObjects();
    _filepath = Utils::getPathFromFilePath(filename, _configFilePath) + Utils::getFilenameFromFilePath(filename);

    if (avformat_open_input(&_avContext, _filepath.c_str(), nullptr, nullptr) != 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't read file " << _filepath << Log::endl;
        return false;
    }

    if (avformat_find_stream_info(_avContext, NULL) < 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't retrieve information for file " << _filepath << Log::endl;
        avformat_close_input(&_avContext);
        return false;
    }

    Log::get() << Log::MESSAGE << "Image_FFmpeg::" << __FUNCTION__ << " - Successfully loaded file " << _filepath << Log::endl;
    av_dump_format(_avContext, 0, _filepath.c_str(), 0);

#if HAVE_LINUX
    // Give the kernel hints about how to read the file
    auto fd = Utils::getFileDescriptorForOpenedFile(_filepath);
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

/*************/
void Image_FFmpeg::readLoop()
{
    // Find the first video stream
    for (int i = 0; i < _avContext->nb_streams; ++i)
    {
#if HAVE_FFMPEG_3
        if (_avContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && _videoStreamIndex < 0)
#else
        if (_avContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && _videoStreamIndex < 0)
#endif
            _videoStreamIndex = i;
#if HAVE_PORTAUDIO
#if HAVE_FFMPEG_3
        else if (_avContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && _audioStreamIndex < 0)
#else
        else if (_avContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && _audioStreamIndex < 0)
#endif
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
#if HAVE_FFMPEG_3
    auto videoCodecParameters = _avContext->streams[_videoStreamIndex]->codecpar;
    auto videoCodecContext = avcodec_alloc_context3(nullptr);
    if (avcodec_parameters_to_context(videoCodecContext, videoCodecParameters) < 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Unable to create a video context from the codec parameters from file " << _filepath << Log::endl;
        return;
    }
#else
    auto videoCodecContext = _avContext->streams[_videoStreamIndex]->codec;
#endif

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
    AVCodec* audioCodec{nullptr};
#if HAVE_FFMPEG_3
    auto audioCodecContext = avcodec_alloc_context3(nullptr);
#else
    AVCodecContext* audioCodecContext{nullptr};
#endif
    if (_audioStreamIndex >= 0)
    {
#if HAVE_FFMPEG_3
        auto audioCodecParameters = _avContext->streams[_audioStreamIndex]->codecpar;
        if (avcodec_parameters_to_context(audioCodecContext, audioCodecParameters) < 0)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Unable to create an audio context from the codec parameters for file " << _filepath << Log::endl;
            return;
        }
#else
        audioCodecContext = _avContext->streams[_audioStreamIndex]->codec;
#endif

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
        {
            Speaker::SampleFormat format;
            switch (audioCodecContext->sample_fmt)
            {
            default:
                Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Unsupported sample format" << Log::endl;
                return;
            case AV_SAMPLE_FMT_U8:
                format = Speaker::SAMPLE_FMT_U8;
                break;
            case AV_SAMPLE_FMT_S16:
                format = Speaker::SAMPLE_FMT_S16;
                break;
            case AV_SAMPLE_FMT_S32:
                format = Speaker::SAMPLE_FMT_S32;
                break;
            case AV_SAMPLE_FMT_FLT:
                format = Speaker::SAMPLE_FMT_FLT;
                break;
            case AV_SAMPLE_FMT_U8P:
                format = Speaker::SAMPLE_FMT_U8P;
                break;
            case AV_SAMPLE_FMT_S16P:
                format = Speaker::SAMPLE_FMT_S16P;
                break;
            case AV_SAMPLE_FMT_S32P:
                format = Speaker::SAMPLE_FMT_S32P;
                break;
            case AV_SAMPLE_FMT_FLTP:
                format = Speaker::SAMPLE_FMT_FLTP;
                break;
            }

            _speaker = unique_ptr<Speaker>(new Speaker());
            if (!_speaker)
                return;

            _speaker->setParameters(audioCodecContext->channels, audioCodecContext->sample_rate, format);
        }
    }
#endif

    // Start reading frames
    AVFrame *frame, *rgbFrame;
#if HAVE_FFMPEG_3
    frame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
#else
    frame = avcodec_alloc_frame();
    rgbFrame = avcodec_alloc_frame();
#endif

    if (!frame || !rgbFrame)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while allocating frame structures" << Log::endl;
        return;
    }

#if HAVE_FFMPEG_3
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, videoCodecContext->width, videoCodecContext->height, 1);
#else
    int numBytes = avpicture_get_size(PIX_FMT_RGB24, videoCodecContext->width, videoCodecContext->height);
#endif
    vector<unsigned char> buffer(numBytes);

    struct SwsContext* swsContext;
    if (!isHap)
    {
#if HAVE_FFMPEG_3
        swsContext = sws_getContext(videoCodecContext->width,
            videoCodecContext->height,
            videoCodecContext->pix_fmt,
            videoCodecContext->width,
            videoCodecContext->height,
            AV_PIX_FMT_RGB24,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr);

        av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer.data(), AV_PIX_FMT_RGB24, videoCodecContext->width, videoCodecContext->height, 1);
#else
        swsContext = sws_getContext(videoCodecContext->width,
            videoCodecContext->height,
            videoCodecContext->pix_fmt,
            videoCodecContext->width,
            videoCodecContext->height,
            PIX_FMT_RGB24,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr);

        avpicture_fill((AVPicture*)rgbFrame, buffer.data(), PIX_FMT_RGB24, videoCodecContext->width, videoCodecContext->height);
#endif
    }

    AVPacket packet;
    av_init_packet(&packet);

    _timeBase = (double)videoStream->time_base.num / (double)videoStream->time_base.den;

    // This implements looping
    do
    {
        _startTime = Timer::getTime();
        auto previousTime = 0ull;

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
                uint64_t timing;
                bool hasFrame = false;

                //
                // If the codec is handled by FFmpeg
                if (!isHap)
                {
#if HAVE_FFMPEG_3
                    auto frameFinished = false;
                    if (avcodec_send_packet(videoCodecContext, &packet) < 0)
                        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while decoding a frame in file " << _filepath << Log::endl;
                    if (avcodec_receive_frame(videoCodecContext, frame) == 0)
                        frameFinished = true;
#else
                    int frameFinished;
                    avcodec_decode_video2(videoCodecContext, frame, &frameFinished, &packet);
#endif

                    if (frameFinished)
                    {
                        sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize, 0, videoCodecContext->height, rgbFrame->data, rgbFrame->linesize);

                        ImageBufferSpec spec(videoCodecContext->width, videoCodecContext->height, 3, ImageBufferSpec::Type::UINT8);
                        spec.format = {"R", "G", "B"};
                        img.reset(new ImageBuffer(spec));

                        unsigned char* pixels = reinterpret_cast<unsigned char*>(img->data());
                        copy(buffer.begin(), buffer.end(), pixels);

                        if (packet.pts != AV_NOPTS_VALUE)
                            timing = static_cast<uint64_t>((double)packet.pts * _timeBase * 1e6);
                        else
                            timing = 0.0;
                        // This handles repeated frames
                        timing += frame->repeat_pict * _timeBase * 0.5;

                        hasFrame = true;
                    }
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
                            spec = ImageBufferSpec(videoCodecContext->width, (int)(ceil((float)videoCodecContext->height / 2.f)), 1, ImageBufferSpec::Type::UINT8);
                            spec.format = {textureFormat};
                        }
                        if (textureFormat == "RGBA_DXT5")
                        {
                            spec = ImageBufferSpec(videoCodecContext->width, videoCodecContext->height, 1, ImageBufferSpec::Type::UINT8);
                            spec.format = {textureFormat};
                        }
                        if (textureFormat == "YCoCg_DXT5")
                        {
                            spec = ImageBufferSpec(videoCodecContext->width, videoCodecContext->height, 1, ImageBufferSpec::Type::UINT8);
                            spec.format = {textureFormat};
                        }
                        else
                        {
#if HAVE_FFMPEG_3
                            av_packet_unref(&packet);
#else
                            av_free_packet(&packet);
#endif
                            return;
                        }

                        spec.format = {textureFormat};
                        img.reset(new ImageBuffer(spec));

                        unsigned long outputBufferBytes = spec.width * spec.height * spec.channels;

                        if (hapDecodeFrame(packet.data, packet.size, img->data(), outputBufferBytes, textureFormat))
                        {
                            if (packet.pts != AV_NOPTS_VALUE)
                                timing = static_cast<uint64_t>((double)packet.pts * _timeBase * 1e6);
                            else
                                timing = 0.0;

                            hasFrame = true;
                        }
                    }
                }

                if (hasFrame)
                {
                    // Add the frame size to the history
                    _framesSize.push_back(img->getSize());

                    lock_guard<mutex> lockFrames(_videoQueueMutex);
                    _timedFrames.emplace_back();
                    std::swap(_timedFrames[_timedFrames.size() - 1].frame, img);
                    _timedFrames[_timedFrames.size() - 1].timing = timing;
                }

                int timedFramesBuffered = _timedFrames.size();

                _videoSeekMutex.unlock();
#if HAVE_FFMPEG_3
                av_packet_unref(&packet);
#else
                av_free_packet(&packet);
#endif

                // Check the current buffer size (sum of all frames in buffer)
                int64_t totalBufferSize = accumulate(_framesSize.begin(), _framesSize.end(), (int64_t)0);

                // Do not store more than a few frames in memory
                // _maximumBufferSize is divided by 2 as another frame queue is held by the display loop
                while (timedFramesBuffered > 0 && totalBufferSize > _maximumBufferSize / 2 && _continueRead)
                {
                    this_thread::sleep_for(chrono::milliseconds(5));
                    lock_guard<mutex> lockSeek(_videoSeekMutex);
                    lock_guard<mutex> lockQueue(_videoQueueMutex);
                    timedFramesBuffered = _timedFrames.size();
                }

                // Clear the frame size history if the buffer has been swapped
                if (timedFramesBuffered == 0)
                    _framesSize.clear();
            }
#if HAVE_PORTAUDIO
            // Reading the audio
            else if (packet.stream_index == _audioStreamIndex && audioCodecContext)
            {
#if HAVE_FFMPEG_3
                auto hasFrame = false;
                if (avcodec_send_packet(audioCodecContext, &packet) < 0)
                    Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while decoding an audio frame in file " << _filepath << Log::endl;
                if (avcodec_receive_frame(audioCodecContext, frame) == 0)
                    hasFrame = true;
#else
                int hasFrame = 0;
                int length = avcodec_decode_audio4(audioCodecContext, frame, &hasFrame, &packet);
#endif

                if (hasFrame)
                {
                    size_t dataSize = av_samples_get_buffer_size(nullptr, audioCodecContext->channels, frame->nb_samples, audioCodecContext->sample_fmt, 1);
                    auto buffer = ResizableArray<uint8_t>((uint8_t*)frame->data[0], (uint8_t*)frame->data[0] + dataSize);
                    _speaker->addToQueue(buffer);
                }

#if HAVE_FFMPEG_3
                av_packet_unref(&packet);
#else
                av_free_packet(&packet);
#endif
            }
#endif
            else
            {
#if HAVE_FFMPEG_3
                av_packet_unref(&packet);
#else
                av_free_packet(&packet);
#endif
            }
        }

        seek(0); // Go back to the beginning of the file

    } while (_loopOnVideo && _continueRead);

#if HAVE_FFMPEG_3
    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
#else
    av_free(rgbFrame);
    av_free(frame);
#endif

    if (!isHap)
        avcodec_close(videoCodecContext);
    _videoStreamIndex = -1;

#if HAVE_PORTAUDIO
    if (audioCodecContext)
    {
        avcodec_close(audioCodecContext);
        _speaker.reset();
    }
    _audioStreamIndex = -1;
#endif
}

/*************/
void Image_FFmpeg::seek(float seconds)
{
    lock_guard<mutex> lock(_videoSeekMutex);

    int seekFlag = 0;
    if (_elapsedTime > seconds)
        seekFlag = AVSEEK_FLAG_BACKWARD;

    // Prevent seeking outside of the file
    float duration = (float)_avContext->duration / (float)AV_TIME_BASE;
    if (seconds < 0)
        seconds = 0;
    else if (seconds > duration)
        seconds = duration;

    int frame = static_cast<int>(floor(seconds / _timeBase));
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
void Image_FFmpeg::videoDisplayLoop()
{
    auto previousTime = 0;

    while (_continueRead)
    {
        auto localQueue = deque<TimedFrame>();
        {
            lock_guard<mutex> lockFrames(_videoQueueMutex);
            std::swap(localQueue, _timedFrames);
        }

        // This sets the start time after a seek
        if (localQueue.size() > 0 && _startTime == -1)
            _startTime = Timer::getTime() - localQueue[0].timing;

        while (localQueue.size() > 0 && _continueRead)
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
            _currentTime = Timer::getTime() - _startTime;

            float seekTiming = _intraOnly ? 1.f : 3.f; // Maximum diff for seek to happen when synced to a master clock

            int64_t clockAsMs;
            bool clockIsPaused{false};
            if (_useClock && Timer::get().getMasterClock<chrono::milliseconds>(clockAsMs, clockIsPaused))
            {
                float seconds = (float)clockAsMs / 1e3f + _shiftTime;
                _clockTime = seconds * 1e6;
            }

            //
            // Show the frame at the right timing, according to clocks
            //
            TimedFrame& timedFrame = localQueue[0];
            if (timedFrame.timing != 0ull)
            {
                if (_paused || (clockIsPaused && _useClock))
                {
                    _startTime = Timer::getTime() - _currentTime;
                    this_thread::sleep_for(chrono::milliseconds(2));
                    continue;
                }
                else if (_useClock && _clockTime != -1l)
                {
                    auto delta = abs(_currentTime - _clockTime);
                    // If the difference between master clock and local clock is greater than 1.5 frames @30Hz, we adjust local clock
                    if (delta > 50000)
                    {
                        _startTime = Timer::getTime() - _clockTime;
                        _currentTime = _clockTime;
                    }
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
                        SThread::pool.enqueueWithoutId([=]() {
                            seek(_elapsedTime);
                            _timeJump = false;
                        });
                    }
                    continue;
                }

                // Otherwise, wait for the right time to display the frame
                if (waitTime > 2e3) // we don't wait if the frame is due for the next few ms
                    this_thread::sleep_for(chrono::microseconds(waitTime));

                _elapsedTime = timedFrame.timing;

                lock_guard<mutex> lock(_writeMutex);
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
void Image_FFmpeg::registerAttributes()
{
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
        [&](const Values& args) { return false; },
        [&]() -> Values {
            if (_avContext == nullptr)
                return {0.f};

            float duration = (float)_avContext->duration / (float)AV_TIME_BASE;
            return {duration};
        });
    setAttributeParameter("duration", false, true);

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
        [&](const Values& args) { return false; },
        [&]() -> Values {
            if (_avContext == nullptr)
                return {0.f};

            float duration = std::max(0.0, (double)_avContext->duration / (double)AV_TIME_BASE - (double)_elapsedTime / 1e6);
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
            SThread::pool.enqueueWithoutId([=]() { seek(seconds); });

            _seekTime = seconds;
            return true;
        },
        [&]() -> Values { return {_seekTime}; },
        {'n'});
    setAttributeParameter("seek", false, true);
    setAttributeDescription("seek", "Change the read position in the video file");

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

    addAttribute("timeShift",
        [&](const Values& args) {
            _shiftTime = args[0].as<float>();

            return true;
        },
        {'n'});
}

} // end of namespace
