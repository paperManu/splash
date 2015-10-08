#include "speaker.h"

#include "log.h"
#include "timer.h"
#include "threadpool.h"

using namespace std;

namespace Splash
{

/*************/
Speaker::Speaker()
{
    registerAttributes();
}

/*************/
Speaker::~Speaker()
{
    freeResources();
}

/*************/
void Speaker::clearQueue()
{
    std::unique_lock<std::mutex> lock(_ringWriteMutex);
    _ringReadPosition = 0;
    _ringWritePosition = 0;
    _ringUnusedSpace = 0;
}

/*************/
void Speaker::setParameters(uint32_t channels, uint32_t sampleRate, SampleFormat format)
{
    _channels = std::max((uint32_t)1, channels);
    _sampleRate = std::max((uint32_t)1, sampleRate);
    _sampleFormat = format;

    initResources();
}

/*************/
void Speaker::freeResources()
{
    if (!_ready)
        return;

    _abortCallback = true;

    if (_portAudioStream)
    {
        Pa_AbortStream(_portAudioStream);
        Pa_CloseStream(_portAudioStream);
    }

    Pa_Terminate();
    _portAudioStream = nullptr;
}

/*************/
void Speaker::initResources()
{
    if (_ready)
        freeResources();

    auto error = Pa_Initialize();
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Speaker::" << __FUNCTION__ << " - Could not initialized PortAudio: " << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return;
    }

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice)
    {
        Log::get() << Log::WARNING << "Speaker::" << __FUNCTION__ << " - Could not find default audio output device" << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return;
    }

    outputParams.channelCount = _channels;
    switch (_sampleFormat)
    {
    default:
        Log::get() << Log::WARNING << "Speaker::" << __FUNCTION__ << " - Unsupported sample format" << Log::endl;
        Pa_Terminate();
        return;
    case SAMPLE_FMT_U8:
        outputParams.sampleFormat = paUInt8;
        _sampleSize = sizeof(unsigned char);
        _planar = false;
        break;
    case SAMPLE_FMT_S16:
        outputParams.sampleFormat = paInt16;
        _sampleSize = sizeof(short);
        _planar = false;
        break;
    case SAMPLE_FMT_S32:
        outputParams.sampleFormat = paInt32;
        _sampleSize = sizeof(int);
        _planar = false;
        break;
    case SAMPLE_FMT_FLT:
        outputParams.sampleFormat = paFloat32;
        _sampleSize = sizeof(float);
        _planar = false;
        break;
    case SAMPLE_FMT_U8P:
        outputParams.sampleFormat = paUInt8;
        _planar = true;
        _sampleSize = sizeof(unsigned char);
        break;
    case SAMPLE_FMT_S16P:
        outputParams.sampleFormat = paInt16;
        _sampleSize = sizeof(short);
        _planar = true;
        break;
    case SAMPLE_FMT_S32P:
        outputParams.sampleFormat = paInt32;
        _sampleSize = sizeof(int);
        _planar = true;
        break;
    case SAMPLE_FMT_FLTP:
        outputParams.sampleFormat = paFloat32;
        _sampleSize = sizeof(float);
        _planar = true;
        break;
    }

    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    error = Pa_OpenStream(&_portAudioStream, nullptr, &outputParams, _sampleRate, 1024, paClipOff, Speaker::portAudioCallback, this);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Speaker::" << __FUNCTION__ << " - Could not open PortAudio stream: " << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return;
    }

    error = Pa_StartStream(_portAudioStream);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Speaker::" << __FUNCTION__ << " - Could not start PortAudio stream: " << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return;
    }

    Log::get() << Log::MESSAGE << "Speaker::" << __FUNCTION__ << " - Successfully opened PortAudio stream" << Log::endl;

    _ready = true;
}

/*************/
int Speaker::portAudioCallback(const void* in, void* out, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
    auto that = (Speaker*)userData;
    uint8_t* output = (uint8_t*)out;

    int readPosition = that->_ringReadPosition;
    int writePosition = that->_ringWritePosition;

    // If the ring buffer is not filled enough, fill with zeros instead
    int delta = 0;
    if (writePosition >= readPosition)
        delta = writePosition - readPosition;
    else
        delta = SPLASH_SPEAKER_RINGBUFFER_SIZE - readPosition + writePosition;
    int step = framesPerBuffer * that->_channels * that->_sampleSize;
    if (delta < step)
    {
        for (unsigned int i = 0; i < step; ++i)
            *output++ = 0;
    }
    // Else, we copy the values and move the read position
    else
    {
        int effectiveSpace = SPLASH_SPEAKER_RINGBUFFER_SIZE - that->_ringUnusedSpace;

        int ringBufferEndLength = effectiveSpace - readPosition;

        if (step <= ringBufferEndLength)
        {
            copy(&that->_ringBuffer[readPosition], &that->_ringBuffer[readPosition + step], output);
        }
        else
        {
            copy(&that->_ringBuffer[readPosition], &that->_ringBuffer[effectiveSpace], output);
            output += ringBufferEndLength;
            copy(&that->_ringBuffer[0], &that->_ringBuffer[step - ringBufferEndLength], output);
        }

        readPosition = (readPosition + step) % effectiveSpace;
        that->_ringReadPosition = readPosition;
    }

    if (that->_abortCallback)
        return paComplete;
    else
        return paContinue;
}

/*************/
void Speaker::registerAttributes()
{
}

} // end of namespace
