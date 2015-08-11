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

    if (_avFormatContext)
        avformat_close_input((AVFormatContext**)&_avFormatContext);
}

#if HAVE_PORTAUDIO
/*************/
bool Image_FFmpeg::initPortAudio()
{
    auto error = Pa_Initialize();
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not initialized PortAudio: " << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return false;
    }

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not find default audio device" << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return false;
    }

    outputParams.channelCount = 2;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    error = Pa_OpenStream(&_portAudioStream, nullptr, &outputParams, 44100, 64, paClipOff, Image_FFmpeg::portAudioCallback, this);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not open PortAudio stream" << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return false;
    }

    error = Pa_StartStream(_portAudioStream);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not start PortAudio stream" << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return false;
    }

    return true;
}

/*************/
int Image_FFmpeg::portAudioCallback(const void* in, void* out, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
    auto that = (Image_FFmpeg*)userData;
    float* output = (float*)out;
    int decodedDataSize = 0;

    unique_lock<mutex> lock(that->_portAudioMutex);

    for (unsigned int i = 0; i < framesPerBuffer;)
    {

        if (that->_portAudioQueue.size() == 0)
        {
            lock.unlock();
            this_thread::sleep_for(chrono::microseconds(10));
            lock.lock();
            continue;
        }

        AVFrame frame;
        int gotFrame = 0;
        int length = avcodec_decode_audio4(that->_audioCodecContext, &frame, &gotFrame, that->_portAudioQueue[0]);
        if (length < 0)
        {
            Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while decoding audio frame, skipping" << Log::endl;
            continue;
        }
        else
        {
            av_free_packet(that->_portAudioQueue[0]);
            that->_portAudioQueue.pop_front();
        }

        int dataSize = 0;
        if (gotFrame)
        {
            dataSize = av_samples_get_buffer_size(nullptr, that->_audioCodecContext->channels, frame.nb_samples, that->_audioCodecContext->sample_fmt, 1);
            memcpy(output, frame.data[0], dataSize);
            output += dataSize;
            ++i;
        }
        if (dataSize <= 0)
            continue;

        decodedDataSize += dataSize;
    }

    return decodedDataSize;
}
#endif

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
        return;
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
        auto startTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        auto previousTime = 0;
        while (_continueReadLoop && av_read_frame(*avContext, &packet) >= 0)
        {
            // Reading the video
            if (packet.stream_index == videoStreamIndex)
            {
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
                        oiio::ImageBuf img(spec);

                        unsigned long outputBufferBytes = spec.width * spec.height * spec.nchannels;

                        if (hapDecodeFrame(packet.data, packet.size, img.localpixels(), outputBufferBytes, textureFormat))
                        {
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
                }

                av_free_packet(&packet);
            }
#if HAVE_PORTAUDIO
            // Reading the audio
            else if (packet.stream_index == audioStreamIndex)
            {
                if (av_dup_packet(&packet) >= 0)
                {
                    unique_lock<mutex> lock(_portAudioMutex);
                    _portAudioQueue.push_back(&packet);
                }
            }
#endif
            else
            {
                av_free_packet(&packet);
            }
        }

        if (av_seek_frame(*avContext, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD) < 0)
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
void Image_FFmpeg::registerAttributes()
{
}

} // end of namespace
