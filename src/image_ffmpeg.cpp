#include "image_ffmpeg.h"

#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

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

    if (_avFormatContext)
        avformat_close_input((AVFormatContext**)&_avFormatContext);
}

/*************/
bool Image_FFmpeg::read(const string& filename)
{
    // First: cleanup
    freeFFmpegObjects();

    AVFormatContext** avContext = (AVFormatContext**)&_avFormatContext;

    if (avformat_open_input(avContext, filename.c_str(), nullptr, nullptr) != 0)
    {
        SLog::log << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't read file " << filename << Log::endl;
        return false;
    }

    if (avformat_find_stream_info(*avContext, NULL) < 0)
    {
        SLog::log << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Couldn't retrieve information for file " << filename << Log::endl;
        avformat_close_input(avContext);
        return false;
    }

    SLog::log << Log::MESSAGE << "Image_FFmpeg::" << __FUNCTION__ << " - Successfully loaded file " << filename << Log::endl;
    av_dump_format(*avContext, 0, filename.c_str(), 0);
    _filename = filename;

    _continueReadLoop = true;
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
    auto videoStream = -1;
    for (int i = 0; i < (*avContext)->nb_streams; ++i)
    {
        if ((*avContext)->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1)
    {
        SLog::log << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - No video stream found in file " << _filename << Log::endl;
        return;
    }

    // Find a decoder
    auto stream = (*avContext)->streams[videoStream];
    auto codecContext = (*avContext)->streams[videoStream]->codec;
    auto codec = avcodec_find_decoder(codecContext->codec_id);
    if (codec == nullptr)
    {
        SLog::log << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Codec not supported for file " << _filename << Log::endl;
        return;
    }

    AVDictionary* optionsDict = nullptr;
    if (avcodec_open2(codecContext, codec, &optionsDict) < 0)
    {
        SLog::log << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not open codec for file " << _filename << Log::endl;
        return;
    }

    AVFrame* frame;
    frame = avcodec_alloc_frame();

    AVFrame* rgbFrame;
    rgbFrame = avcodec_alloc_frame();

    if (!frame || !rgbFrame)
    {
        SLog::log << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while allocating frame structures" << Log::endl;
        return;
    }

    int numBytes = avpicture_get_size(PIX_FMT_RGB24, codecContext->width, codecContext->height);
    vector<unsigned char> buffer(numBytes);

    struct SwsContext* swsContext = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height,
                                                   PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
    
    avpicture_fill((AVPicture*)rgbFrame, buffer.data(), PIX_FMT_RGB24, codecContext->width, codecContext->height);

    AVPacket packet;
    av_init_packet(&packet);

    double timeBase = (double)stream->time_base.num / (double)stream->time_base.den;

    // This implements looping
    while (_continueReadLoop)
    {
        auto startTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        auto previousTime = 0;
        // Reading the video
        while (_continueReadLoop && av_read_frame(*avContext, &packet) >= 0)
        {
            if (packet.stream_index == videoStream)
            {
                int frameFinished;
                avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);

                if (frameFinished)
                {
                    sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize, 0, codecContext->height, rgbFrame->data, rgbFrame->linesize);

                    oiio::ImageSpec spec(codecContext->width, codecContext->height, 3, oiio::TypeDesc::UINT8);
                    oiio::ImageBuf img(spec);
                    unsigned char* pixels = static_cast<unsigned char*>(img.localpixels());
                    copy(buffer.begin(), buffer.end(), pixels);

                    // We wait until we get the green light for displaying this frame
                    unsigned long long waitTime = 0;
                    if (packet.pts != AV_NOPTS_VALUE)
                        waitTime = static_cast<unsigned long long>((double)packet.pts * timeBase * 1e6) - previousTime;
                    this_thread::sleep_for(chrono::microseconds(waitTime));
                    previousTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() - startTime;

                    unique_lock<mutex> lock(_writeMutex);
                    _bufferImage.swap(img);
                    _imageUpdated = true;
                    updateTimestamp();
                }
            }

            av_free_packet(&packet);
        }

        if (av_seek_frame(*avContext, videoStream, 0, AVSEEK_FLAG_BACKWARD) < 0)
        {
            SLog::log << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not seek in file " << _filename << Log::endl;
            break;
        }
    }

    av_free(rgbFrame);
    av_free(frame);
    avcodec_close(codecContext);
}

/*************/
void Image_FFmpeg::registerAttributes()
{
}

} // end of namespace
