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

#if HAVE_PORTAUDIO
    if (_avFormatContext)
    {
        avformat_close_input((AVFormatContext**)&_avFormatContext);
        Pa_Terminate();
    }
#endif
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

    outputParams.channelCount = _audioCodecContext->channels;
    switch (_audioCodecContext->sample_fmt)
    {
    default:
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Unsupported sample format" << Log::endl;
        Pa_Terminate();
        return false;
    case AV_SAMPLE_FMT_U8:
        outputParams.sampleFormat = paUInt8;
        _portAudioSampleSize = sizeof(unsigned char);
        break;
    case AV_SAMPLE_FMT_S16:
        outputParams.sampleFormat = paInt16;
        _portAudioSampleSize = sizeof(short);
        break;
    case AV_SAMPLE_FMT_S32:
        outputParams.sampleFormat = paInt32;
        _portAudioSampleSize = sizeof(int);
        break;
    case AV_SAMPLE_FMT_FLT:
        outputParams.sampleFormat = paFloat32;
        _portAudioSampleSize = sizeof(float);
        break;
    case AV_SAMPLE_FMT_U8P:
        outputParams.sampleFormat = paUInt8 | paNonInterleaved;
        _portAudioSampleSize = sizeof(unsigned char);
        break;
    case AV_SAMPLE_FMT_S16P:
        outputParams.sampleFormat = paInt16 | paNonInterleaved;
        _portAudioSampleSize = sizeof(short);
        break;
    case AV_SAMPLE_FMT_S32P:
        outputParams.sampleFormat = paInt32 | paNonInterleaved;
        _portAudioSampleSize = sizeof(int);
        break;
    case AV_SAMPLE_FMT_FLTP:
        outputParams.sampleFormat = paFloat32 | paNonInterleaved;
        _portAudioSampleSize = sizeof(float);
        break;
    }

    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    //error = Pa_OpenStream(&_portAudioStream, nullptr, &outputParams, 48000, 1024, paClipOff, Image_FFmpeg::portAudioCallback, this);
    error = Pa_OpenStream(&_portAudioStream, nullptr, &outputParams, _audioCodecContext->sample_rate, 512, paClipOff, nullptr, nullptr);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not open PortAudio stream: " << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return false;
    }

    error = Pa_StartStream(_portAudioStream);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Could not start PortAudio stream: " << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return false;
    }
    else
    {
        Log::get() << Log::MESSAGE << "Image_FFmpeg::" << __FUNCTION__ << " - Successfully opened PortAudio stream" << Log::endl;
    }

    return true;
}

/*************/
int Image_FFmpeg::portAudioCallback(const void* in, void* out, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
    auto that = (Image_FFmpeg*)userData;
    uint8_t* output = (uint8_t*)out;

    for (unsigned int i = 0; i < framesPerBuffer;)
    {
        if (that->_portAudioQueue.size() == 0)
        {
            that->_portAudioPosition = 0;
            this_thread::sleep_for(chrono::microseconds(10));
            continue;
        }

        size_t frameSize = that->_audioCodecContext->channels * that->_portAudioSampleSize;
        memcpy(output, &(that->_portAudioQueue[0][that->_portAudioPosition]), frameSize);
        output += frameSize;

        that->_portAudioPosition += frameSize;
        if (that->_portAudioPosition >= that->_portAudioQueue[0].size())
        {
            unique_lock<mutex> lock(that->_portAudioMutex);
            that->_portAudioPosition = 0;
            that->_portAudioQueue.pop_front();
        }

        i++;
    }

    return paContinue;
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

        if (_audioCodecContext)
        {
            if (!initPortAudio())
                return;
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
                auto frame = unique_ptr<AVFrame>(new AVFrame());
                int gotFrame = 0;
                int length = avcodec_decode_audio4(_audioCodecContext, frame.get(), &gotFrame, &packet);
                if (length < 0)
                    Log::get() << Log::WARNING << "Image_FFmpeg::" << __FUNCTION__ << " - Error while decoding audio frame, skipping" << Log::endl;

                if (gotFrame)
                {
                    //size_t dataSize = av_samples_get_buffer_size(nullptr, _audioCodecContext->channels, frame->nb_samples, _audioCodecContext->sample_fmt, 1);
                    //vector<char> buffer((char*)frame->data, (char*)frame->data + dataSize * sizeof(char));

                    //unique_lock<mutex> lock(_portAudioMutex);
                    //_portAudioQueue.push_back(buffer);

                    Pa_WriteStream(_portAudioStream, frame->data, frame->nb_samples);
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
                    startTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() - _seekFrame * timeBase * 1e6;
                    previousTime = _seekFrame * timeBase * 1e6;
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
void Image_FFmpeg::registerAttributes()
{
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
