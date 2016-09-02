#include "listener.h"

#include "log.h"
#include "threadpool.h"
#include "timer.h"

#if not HAVE_OSX
#include "pa_jack.h"
#endif

using namespace std;

namespace Splash
{

/*************/
Listener::Listener()
{
    registerAttributes();
}

/*************/
Listener::~Listener()
{
    freeResources();
}

/*************/
void Listener::setParameters(uint32_t channels, uint32_t sampleRate, SampleFormat format, const string& deviceName)
{
    _channels = std::max((uint32_t)1, channels);
    _sampleRate = sampleRate;
    _sampleFormat = format;
    _deviceName = deviceName;

    initResources();
}

/*************/
void Listener::freeResources()
{
    if (!_ready)
        return;

    _abordCallback = true;
    if (_portAudioStream)
    {
        Pa_AbortStream(_portAudioStream);
        // Pa_CloseStream(_portAudioStream);
        _portAudioStream = nullptr;
    }

    Pa_Terminate();
}

/*************/
void Listener::initResources()
{
    if (_ready)
        freeResources();

#if not HAVE_OSX
    PaJack_SetClientName("splash");
#endif
    auto error = Pa_Initialize();
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Listener::" << __FUNCTION__ << " - Could not initialized PortAudio: " << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return;
    }

    PaStreamParameters inputParams;
    inputParams.device = -1;
#if not HAVE_OSX
    // If a JACK device name is set, we try to connect to it
    if (_deviceName != "")
    {
        auto numDevices = Pa_GetDeviceCount();
        for (int i = 0; i < numDevices; ++i)
        {
            auto deviceInfo = Pa_GetDeviceInfo(i);
            if (string(deviceInfo->name) == _deviceName)
            {
                inputParams.device = i;
                if (deviceInfo->hostApi == Pa_HostApiTypeIdToHostApiIndex(paJACK))
                    _useJack = true;
            }
        }

        if (inputParams.device >= 0)
            Log::get() << Log::MESSAGE << "Listener::" << __FUNCTION__ << " - Connecting to device " << _deviceName << Log::endl;
    }
#endif

    if (inputParams.device < 0)
    {
        inputParams.device = Pa_GetDefaultInputDevice();
        if (inputParams.device == paNoDevice)
        {
            Log::get() << Log::WARNING << "Listener::" << __FUNCTION__ << " - Could not find default audio input device" << Pa_GetErrorText(error) << Log::endl;
            Pa_Terminate();
            return;
        }
    }

    auto deviceInfo = Pa_GetDeviceInfo(inputParams.device);
    Log::get() << Log::MESSAGE << "Listener::" << __FUNCTION__ << " - Connected to device: " << deviceInfo->name << Log::endl;

    if (_sampleRate == 0)
        _sampleRate = deviceInfo->defaultSampleRate;

    inputParams.channelCount = _channels;
    switch (_sampleFormat)
    {
    default:
        Log::get() << Log::WARNING << "Listener::" << __FUNCTION__ << " - Unsupported sample format" << Log::endl;
        Pa_Terminate();
        return;
    case SAMPLE_FMT_U8:
        inputParams.sampleFormat = paUInt8;
        _sampleSize = sizeof(unsigned char);
        break;
    case SAMPLE_FMT_S16:
        inputParams.sampleFormat = paInt16;
        _sampleSize = sizeof(short);
        break;
    case SAMPLE_FMT_S32:
        inputParams.sampleFormat = paInt32;
        _sampleSize = sizeof(int);
        break;
    case SAMPLE_FMT_FLT:
        inputParams.sampleFormat = paFloat32;
        _sampleSize = sizeof(float);
        break;
    }

    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    error = Pa_OpenStream(&_portAudioStream, &inputParams, nullptr, _sampleRate, 256, paClipOff, Listener::portAudioCallback, this);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Listener::" << __FUNCTION__ << " - Could not open PortAudio stream: " << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return;
    }

    error = Pa_StartStream(_portAudioStream);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Listener::" << __FUNCTION__ << " - Could not start PortAudio stream: " << Pa_GetErrorText(error) << Log::endl;
        Pa_Terminate();
        return;
    }

    Log::get() << Log::MESSAGE << "Listener::" << __FUNCTION__ << " - Successfully opened PortAudio stream" << Log::endl;

    _ready = true;
}

/*************/
int Listener::portAudioCallback(
    const void* in, void* out, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
    auto that = (Listener*)userData;
    uint8_t* input = (uint8_t*)in;

    int readPosition = that->_ringReadPosition;
    int writePosition = that->_ringWritePosition;

    int delta = 0;
    if (readPosition > writePosition)
        delta = readPosition - writePosition;
    else
        delta = SPLASH_LISTENER_RINGBUFFER_SIZE - writePosition + readPosition;

    int step = framesPerBuffer * that->_channels * that->_sampleSize;
    if (delta < step)
        return paContinue;

    int spaceLeft = SPLASH_LISTENER_RINGBUFFER_SIZE - writePosition;

    if (spaceLeft < step)
        writePosition = 0;
    else
        spaceLeft = 0;

    std::copy(input, input + step, &that->_ringBuffer[writePosition]);
    writePosition = (writePosition + step);

    that->_ringUnusedSpace = spaceLeft;
    that->_ringWritePosition = writePosition;

    if (that->_abordCallback)
        return paComplete;
    else
        return paContinue;
}

/*************/
void Listener::registerAttributes()
{
}

} // end of namespace
