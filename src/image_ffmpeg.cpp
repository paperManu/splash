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
    _continueReadLoop = false;
    if (_readLoopThread.joinable())
        _readLoopThread.join();
    if (_videoDisplayThread.joinable())
        _videoDisplayThread.join();
}

/*************/
bool Image_FFmpeg::read(const string& filename)
{
    // First: cleanup
    freeFFmpegObjects();

    AVFormatContext** avContext = (AVFormatContext**)&_avFormatContext;

    if (avformat_open_input(avContext, filename.c_str(), nullptr, nullptr) != 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't read file " << filename << Log::endl;
        return false;
    }

    if (avformat_find_stream_info(*avContext, NULL) < 0)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't retrieve information for file " << filename << Log::endl;
        avformat_close_input(avContext);
        return false;
    }

    Log::get() << Log::MESSAGE << "Image_FFmpeg::" << __FUNCTION__ << " - Successfully loaded file " << filename << Log::endl;
    av_dump_format(*avContext, 0, filename.c_str(), 0);
    _filepath = filename;

    // Launch the loops
    _continueReadLoop = true;
    _videoDisplayThread = thread([&]() {
        videoDisplayLoop();
    });

    _readLoopThread = thread([&]() {
        readLoop();
    });

    return true;
}

/*************/
void Image_FFmpeg::readLoop()
{
    AVFormatContext** avContext = (AVFormatContext**)&_avFormatContext;

    // Find the first video stream
    auto videoStreamIndex = -1;
    auto audioStreamIndex = -1;
    for (int i = 0; i < (*avContext)->nb_streams; ++i)
    {
        if ((*avContext)->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex < 0)
            videoStreamIndex = i;
        else if ((*avContext)->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex < 0)
            audioStreamIndex = i;
    }

    if (videoStreamIndex == -1)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - No video stream found in file " << _filepath << Log::endl;
        return;
    }

    if (audioStreamIndex == -1)
    {
        Log::get() << Log::MESSAGE << "Image_FFmpeg::" << __FUNCTION__ << " - No audio stream found in file " << _filepath << Log::endl;
    }

    // Find a video decoder
    auto videoStream = (*avContext)->streams[videoStreamIndex];
    auto videoCodecContext = (*avContext)->streams[videoStreamIndex]->codec;
    auto videoCodec = avcodec_find_decoder(videoCodecContext->codec_id);
    auto isHap = false;

    if (videoCodec == nullptr && string(videoCodecContext->codec_name).find("Hap") != string::npos)
    {
        isHap = true;
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
    AVCodec* audioCodec = nullptr;
    if (audioStreamIndex >= 0)
    {
        _audioCodecContext = (*avContext)->streams[audioStreamIndex]->codec;
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

    int numBytes = avpicture_get_size(PIX_FMT_RGB24, videoCodecContext->width, videoCodecContext->height);
    vector<unsigned char> buffer(numBytes);

    struct SwsContext* swsContext;
    if (!isHap)
    {
        swsContext = sws_getContext(videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt, videoCodecContext->width, videoCodecContext->height,
                                    PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
    
        avpicture_fill((AVPicture*)rgbFrame, buffer.data(), PIX_FMT_RGB24, videoCodecContext->width, videoCodecContext->height);
    }

    AVPacket packet;
    av_init_packet(&packet);

    double timeBase = (double)videoStream->time_base.num / (double)videoStream->time_base.den;

    // This implements looping
    while (_continueReadLoop)
    {
        _startTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        auto previousTime = 0ull;
        while (_continueReadLoop && av_read_frame(*avContext, &packet) >= 0)
        {
            // Reading the video
            if (packet.stream_index == videoStreamIndex)
            {
                oiio::ImageBuf img;
                uint64_t timing;
                bool hasFrame = false;

                //
                // If the codec is handled by FFmpeg
                if (!isHap)
                {
                    int frameFinished;
                    avcodec_decode_video2(videoCodecContext, frame, &frameFinished, &packet);

                    if (frameFinished)
                    {
                        sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize, 0, videoCodecContext->height, rgbFrame->data, rgbFrame->linesize);

                        oiio::ImageSpec spec(videoCodecContext->width, videoCodecContext->height, 3, oiio::TypeDesc::UINT8);
                        oiio::ImageBuf tmpImg(spec);
                        img.swap(tmpImg);

                        unsigned char* pixels = static_cast<unsigned char*>(img.localpixels());
                        copy(buffer.begin(), buffer.end(), pixels);

                        if (packet.pts != AV_NOPTS_VALUE)
                            timing = static_cast<uint64_t>((double)packet.pts * timeBase * 1e6);
                        else
                            timing = 0.0;
                        // This handles repeated frames
                        timing += frame->repeat_pict * timeBase * 0.5;

                        hasFrame = true;
                    }
                }
                //
                // If the codec is marked as Hap / Hap alpha / Hap Q
                else if (isHap)
                {
                    // We are using kind of a hack to store a DXT compressed image in an oiio::ImageBuf
                    // First, we check the texture format type
                    std::string textureFormat;
                    if (hapDecodeFrame(packet.data, packet.size, nullptr, 0, textureFormat))
                    {
                        // Check if we need to resize the reader buffer
                        // We set the size so as to have just enough place for the given texture format
                        oiio::ImageSpec spec;
                        if (textureFormat == "RGB_DXT1")
                            spec = oiio::ImageSpec(videoCodecContext->width, (int)(ceil((float)videoCodecContext->height / 2.f)), 1, oiio::TypeDesc::UINT8);
                        if (textureFormat == "RGBA_DXT5")
                            spec = oiio::ImageSpec(videoCodecContext->width, videoCodecContext->height, 1, oiio::TypeDesc::UINT8);
                        if (textureFormat == "YCoCg_DXT5")
                            spec = oiio::ImageSpec(videoCodecContext->width, videoCodecContext->height, 1, oiio::TypeDesc::UINT8);
                        else
                            return;

                        spec.channelnames = {textureFormat};
                        oiio::ImageBuf tmpImg(spec);
                        img.swap(tmpImg);

                        unsigned long outputBufferBytes = spec.width * spec.height * spec.nchannels;

                        if (hapDecodeFrame(packet.data, packet.size, img.localpixels(), outputBufferBytes, textureFormat))
                        {
                            if (packet.pts != AV_NOPTS_VALUE)
                                timing = static_cast<uint64_t>((double)packet.pts * timeBase * 1e6);
                            else
                                timing = 0.0;

                            hasFrame = true;
                        }
                    }
                }

                if (hasFrame)
                {
                    unique_lock<mutex> lockFrames(_videoFramesMutex);
                    _timedFrames.emplace_back();
                    _timedFrames[_timedFrames.size() - 1].frame.swap(img);
                    _timedFrames[_timedFrames.size() - 1].timing = timing;
                    _videoFramesCondition.notify_one();
                }

                av_free_packet(&packet);
            }
#if HAVE_PORTAUDIO
            // Reading the audio
            else if (packet.stream_index == audioStreamIndex)
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

            if (_seekFrame >= 0)
            {
                if (avformat_seek_file(*avContext, videoStreamIndex, 0, _seekFrame, _seekFrame, AVSEEK_FLAG_BACKWARD) < 0)
                {
                    Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not seek to timestamp " << _seekFrame << Log::endl;
                }
                else
                {
                    _startTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() - _seekFrame * timeBase * 1e6;
                    previousTime = _seekFrame * timeBase * 1e6;
                    unique_lock<mutex> lockFrames(_videoFramesMutex);
                    _timedFrames.clear();
#if HAVE_PORTAUDIO
                    _speaker->clearQueue();
#endif
                }
                _seekFrame = -1;
            }
        }

        if (av_seek_frame(*avContext, videoStreamIndex, 0, 0) < 0)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not seek in file " << _filepath << Log::endl;
            break;
        }
    }

    av_free(rgbFrame);
    av_free(frame);
    if (!isHap)
        avcodec_close(videoCodecContext);
#if HAVE_PORTAUDIO
    if (_audioCodecContext)
        avcodec_close(_audioCodecContext);
#endif
}

/*************/
void Image_FFmpeg::videoDisplayLoop()
{
    auto previousTime = 0;

    while(_continueReadLoop)
    {
        unique_lock<mutex> lockFrames(_videoFramesMutex);
        _videoFramesCondition.wait(lockFrames);

        while (_timedFrames.size() > 0)
        {
            TimedFrame& timedFrame = _timedFrames[0];
            if (timedFrame.timing != 0ull)
            {
                uint64_t waitTime = timedFrame.timing - previousTime;
                this_thread::sleep_for(chrono::microseconds(waitTime));
                previousTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() - _startTime;
                _elapsedTime = timedFrame.timing;

                unique_lock<mutex> lock(_writeMutex);
                _bufferImage.swap(timedFrame.frame);
                _imageUpdated = true;
                updateTimestamp();
            }
            _timedFrames.pop_front();
        }
    }
}

/*************/
void Image_FFmpeg::registerAttributes()
{
    _attribFunctions["duration"] = AttributeFunctor([&](const Values& args) {
        return false;
    }, [&]() -> Values {
        if (_avFormatContext == nullptr)
            return {0.f};

        AVFormatContext** avContext = (AVFormatContext**)&_avFormatContext;
        float duration = (*avContext)->duration / AV_TIME_BASE;
        return {duration};
    });

    _attribFunctions["remaining"] = AttributeFunctor([&](const Values& args) {
        return false;
    }, [&]() -> Values {
        if (_avFormatContext == nullptr)
            return {0.f};

        AVFormatContext** avContext = (AVFormatContext**)&_avFormatContext;
        float duration = (double)(*avContext)->duration / (double)AV_TIME_BASE - (double)_elapsedTime  / 1e6;
        return {duration};
    });

    _attribFunctions["seek"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;
        if (args[0].asInt() >= 0)
            _seekFrame = args[0].asInt();
        return true;
    }, [&]() -> Values {
        return {(int)_seekFrame};
    });
}

} // end of namespace
