#include "image_ffmpeg.h"

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
Image_FFmpeg::Image_FFmpeg()
{
    _type = "image_ffmpeg";
    registerAttributes();

    av_register_all();
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
    _filepath = filename;

    // Launch the loops
    _continueRead = true;
    _videoDisplayThread = thread([&]() {
        videoDisplayLoop();
    });

    _readLoopThread = thread([&]() {
        readLoop();
    });

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
    for (int i = 0; i < (_avContext)->nb_streams; ++i)
    {
        if ((_avContext)->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && _videoStreamIndex < 0)
            _videoStreamIndex = i;
#if HAVE_PORTAUDIO
        else if ((_avContext)->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && _audioStreamIndex < 0)
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
    auto videoStream = (_avContext)->streams[_videoStreamIndex];
    auto _videoCodecContext = (_avContext)->streams[_videoStreamIndex]->codec;
    auto videoCodec = avcodec_find_decoder(_videoCodecContext->codec_id);
    auto isHap = false;

    // Check whether the video codec only has intra frames
    auto desc = avcodec_descriptor_get(_videoCodecContext->codec_id);
    if (desc)
        _intraOnly = !!(desc->props & AV_CODEC_PROP_INTRA_ONLY);
    else
        _intraOnly = false; // We don't know, so we consider it's not

    auto fourcc = tagToFourCC(_videoCodecContext->codec_tag);
    if (videoCodec == nullptr && fourcc.find("Hap") != string::npos)
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
        if (avcodec_open2(_videoCodecContext, videoCodec, &optionsDict) < 0)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not open video codec for file " << _filepath << Log::endl;
            return;
        }
    }

#if HAVE_PORTAUDIO
    // Find an audio decoder
    AVCodec* audioCodec = nullptr;
    if (_audioStreamIndex >= 0)
    {
        _audioCodecContext = (_avContext)->streams[_audioStreamIndex]->codec;
        audioCodec = avcodec_find_decoder(_audioCodecContext->codec_id);

        if (audioCodec == nullptr)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Audio codec not supported for file " << _filepath << Log::endl;
            _audioCodecContext = nullptr;
        }
        else
        {
            AVDictionary* audioOptionsDict = nullptr;
            if (avcodec_open2(_audioCodecContext, audioCodec, &audioOptionsDict) < 0)
            {
                Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not open audio codec for file " << _filepath << Log::endl;
                _audioCodecContext = nullptr;
            }
        }

        if (_audioCodecContext)
        {
            Speaker::SampleFormat format;
            switch (_audioCodecContext->sample_fmt)
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

            _speaker->setParameters(_audioCodecContext->channels, _audioCodecContext->sample_rate, format);
        }
    }
#endif

    // Start reading frames
    AVFrame* frame;
    frame = avcodec_alloc_frame();

    AVFrame* rgbFrame;
    rgbFrame = avcodec_alloc_frame();

    if (!frame || !rgbFrame)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while allocating frame structures" << Log::endl;
        return;
    }

    int numBytes = avpicture_get_size(PIX_FMT_RGB24, _videoCodecContext->width, _videoCodecContext->height);
    vector<unsigned char> buffer(numBytes);

    struct SwsContext* swsContext;
    if (!isHap)
    {
        swsContext = sws_getContext(_videoCodecContext->width, _videoCodecContext->height, _videoCodecContext->pix_fmt, _videoCodecContext->width, _videoCodecContext->height,
                                    PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
    
        avpicture_fill((AVPicture*)rgbFrame, buffer.data(), PIX_FMT_RGB24, _videoCodecContext->width, _videoCodecContext->height);
    }

    AVPacket packet;
    av_init_packet(&packet);

    _timeBase = (double)videoStream->time_base.num / (double)videoStream->time_base.den;

    // This implements looping
    do
    {
        _startTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        auto previousTime = 0ull;
        
        auto shouldContinueLoop = [&]() -> bool {
            unique_lock<mutex> lock(_videoSeekMutex);
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
                    int frameFinished;
                    avcodec_decode_video2(_videoCodecContext, frame, &frameFinished, &packet);

                    if (frameFinished)
                    {
                        sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize, 0, _videoCodecContext->height, rgbFrame->data, rgbFrame->linesize);

                        ImageBufferSpec spec(_videoCodecContext->width, _videoCodecContext->height, 3, ImageBufferSpec::Type::UINT8);
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
                            spec = ImageBufferSpec(_videoCodecContext->width, (int)(ceil((float)_videoCodecContext->height / 2.f)), 1, ImageBufferSpec::Type::UINT8);
                            spec.format = {textureFormat};
                        }
                        if (textureFormat == "RGBA_DXT5")
                        {
                            spec = ImageBufferSpec(_videoCodecContext->width, _videoCodecContext->height, 1, ImageBufferSpec::Type::UINT8);
                            spec.format = {textureFormat};
                        }
                        if (textureFormat == "YCoCg_DXT5")
                        {
                            spec = ImageBufferSpec(_videoCodecContext->width, _videoCodecContext->height, 1, ImageBufferSpec::Type::UINT8);
                            spec.format = {textureFormat};
                        }
                        else
                        {
                            av_free_packet(&packet);
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
                    unique_lock<mutex> lockFrames(_videoQueueMutex);
                    _timedFrames.emplace_back();
                    std::swap(_timedFrames[_timedFrames.size() - 1].frame, img);
                    _timedFrames[_timedFrames.size() - 1].timing = timing;
                }

               _videoSeekMutex.unlock();
                av_free_packet(&packet);

                // Do not store more than a few frames in memory
                while (_timedFrames.size() > 30 && _continueRead)
                    this_thread::sleep_for(chrono::milliseconds(2));
            }
#if HAVE_PORTAUDIO
            // Reading the audio
            else if (packet.stream_index == _audioStreamIndex && _audioCodecContext)
            {
                auto frame = unique_ptr<AVFrame>(new AVFrame());
                int gotFrame = 0;
                int length = avcodec_decode_audio4(_audioCodecContext, frame.get(), &gotFrame, &packet);
                if (length < 0)
                    Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while decoding audio frame, skipping" << Log::endl;

                if (gotFrame)
                {
                    size_t dataSize = av_samples_get_buffer_size(nullptr, _audioCodecContext->channels, frame->nb_samples, _audioCodecContext->sample_fmt, 1);
                    vector<uint8_t> buffer((uint8_t*)frame->data[0], (uint8_t*)frame->data[0] + dataSize);
                    _speaker->addToQueue(buffer);
                }

                av_free_packet(&packet);
            }
#endif
            else
            {
                av_free_packet(&packet);
            }
        }

        // Set elapsed time to infinity (video finished)
        _elapsedTime = numeric_limits<float>::max();

        if (av_seek_frame(_avContext, _videoStreamIndex, 0, 0) < 0)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not seek in file " << _filepath << Log::endl;
            break;
        }
        else
        {
            unique_lock<mutex> lock(_videoQueueMutex);
            _timedFrames.clear();
#if HAVE_PORTAUDIO
            if (_speaker)
                _speaker->clearQueue();
#endif
        }
    } while (_loopOnVideo && _continueRead);

    av_free(rgbFrame);
    av_free(frame);
    if (!isHap)
        avcodec_close(_videoCodecContext);
    _videoStreamIndex = -1;

#if HAVE_PORTAUDIO
    if (_audioCodecContext)
    {
        avcodec_close(_audioCodecContext);
        _speaker.reset();
    }
    _audioStreamIndex = -1;
#endif
}

/*************/
void Image_FFmpeg::seek(float seconds)
{
    unique_lock<mutex> lock(_videoSeekMutex);

    int seekFlag = 0;
    if (_elapsedTime > seconds)
        seekFlag = AVSEEK_FLAG_BACKWARD;

    int frame = static_cast<int>(floor(seconds / _timeBase));
    if (avformat_seek_file(_avContext, _videoStreamIndex, 0, frame, frame, seekFlag) < 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not seek to timestamp " << seconds << Log::endl;
    }
    else
    {
        unique_lock<mutex> lockQueue(_videoQueueMutex);
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

    while(_continueRead)
    {
        auto localQueue = deque<TimedFrame>();
        {
            unique_lock<mutex> lockFrames(_videoQueueMutex);
            std::swap(localQueue, _timedFrames);
        }

        // This sets the start time after a seek
        if (localQueue.size() > 0 && _startTime == -1)
            _startTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() - localQueue[0].timing;

        while (localQueue.size() > 0 && _continueRead)
        {
            // If seek, clear the local queue as the frames should not be shown
            if (_startTime == -1)
            {
                localQueue.clear();
                continue;
            }

            int64_t clockAsMs;
            bool clockIsPaused {false};

            //
            // Get the current master and local clocks
            //
            _currentTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() - _startTime;

            float seekTiming = _intraOnly ? 0.33f : 3.f; // Maximum diff for seek to happen when synced to a master clock
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
                    auto actualTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() - _startTime;
                    _startTime = _startTime + (actualTime - _currentTime);
                    continue;
                }
                else if (_useClock && _clockTime != -1l)
                {
                    auto delta = abs(_currentTime - _clockTime);
                    // If the difference between master clock and local clock is greater than 1.5 frames @30Hz, we adjust local clock
                    if (delta > 50000)
                    {
                        _startTime = _startTime + _currentTime - _clockTime;
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
                        _elapsedTime = _currentTime / 1e6 + _shiftTime;
                        localQueue.clear();
                        auto seekThread = thread([=]() {
                            seek(_elapsedTime);
                            _timeJump = false;
                        });
                        seekThread.detach();
                    }
                    continue;
                }

                // Otherwise, wait for the right time to display the frame
                if (waitTime > 1e3) // we don't wait if the frame is due for the next ms
                    this_thread::sleep_for(chrono::microseconds(waitTime));

                _elapsedTime = timedFrame.timing;

                unique_lock<mutex> lock(_writeMutex);
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
    _attribFunctions["duration"] = AttributeFunctor([&](const Values& args) {
        return false;
    }, [&]() -> Values {
        if (_avContext == nullptr)
            return {0.f};

        float duration = _avContext->duration / AV_TIME_BASE;
        return {duration};
    });
    _attribFunctions["duration"].doUpdateDistant(true);
    _attribFunctions["duration"].savable(false);

    _attribFunctions["loop"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        _loopOnVideo = (bool)args[0].asInt();
        return true;
    }, [&]() -> Values {
        int loop = _loopOnVideo;
        return {loop};
    });
    _attribFunctions["loop"].doUpdateDistant(true);

    _attribFunctions["remaining"] = AttributeFunctor([&](const Values& args) {
        return false;
    }, [&]() -> Values {
        if (_avContext == nullptr)
            return {0.f};

        float duration = std::max(0.0, (double)_avContext->duration / (double)AV_TIME_BASE - (double)_elapsedTime  / 1e6);
        return {duration};
    });
    _attribFunctions["remaining"].doUpdateDistant(true);
    _attribFunctions["remaining"].savable(false);

    _attribFunctions["pause"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        _paused = args[0].asInt();
        return true;
    }, [&]() -> Values {
        return {_paused};
    });
    _attribFunctions["pause"].doUpdateDistant(true);
    _attribFunctions["pause"].savable(false);

    _attribFunctions["seek"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        float seconds = args[0].asFloat();
        SThread::pool.enqueueWithoutId([=]() {
            seek(seconds);
        });

        _seekTime = seconds;
        return true;
    }, [&]() -> Values {
        return {_seekTime};
    });
    _attribFunctions["seek"].doUpdateDistant(true);
    _attribFunctions["seek"].savable(false);

    _attribFunctions["useClock"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        _useClock = args[0].asInt();
        if (!_useClock)
            _clockTime = -1;
        else
            _clockTime = 0;

        return true;
    }, [&]() -> Values {
        return {(int)_useClock};
    });
    _attribFunctions["useClock"].doUpdateDistant(true);

    _attribFunctions["timeShift"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        _shiftTime = args[0].asFloat();

        return true;
    });
}

} // end of namespace
