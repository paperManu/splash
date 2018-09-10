#include "./sound/sound_engine.h"

#if HAVE_JACK
#include <pa_jack.h>
#endif

#include "./utils/log.h"

using namespace std;

namespace Splash
{

mutex Sound_Engine::_engineMutex;

/*************/
Sound_Engine::Sound_Engine()
{
    lock_guard<mutex> lock(_engineMutex);
#if HAVE_JACK
    PaJack_SetClientName("splash");
#endif
    auto error = Pa_Initialize();
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Sound_Engine::" << __FUNCTION__ << " - Could not initialized PortAudio: " << Pa_GetErrorText(error) << Log::endl;
        return;
    }

    _ready = true;
}

/*************/
Sound_Engine::~Sound_Engine()
{
    lock_guard<mutex> lock(_engineMutex);

    if (_portAudioStream)
    {
        Pa_AbortStream(_portAudioStream);
        Pa_CloseStream(_portAudioStream);
    }
    _portAudioStream = nullptr;

    if (_ready)
        Pa_Terminate();
}

/*************/
#if HAVE_JACK
bool Sound_Engine::getDevice(bool inputDevice, const string& name)
#else
bool Sound_Engine::getDevice(bool inputDevice, const string& /*name*/)
#endif
{
    lock_guard<mutex> lock(_engineMutex);

    _inputDevice = inputDevice;
    _streamParameters = PaStreamParameters();
    _streamParameters.device = -1;
#if HAVE_JACK
    // If a JACK device name is set, we try to connect to it
    if (name != "")
    {
        auto numDevices = Pa_GetDeviceCount();
        for (int i = 0; i < numDevices; ++i)
        {
            auto deviceInfo = Pa_GetDeviceInfo(i);
            if (string(deviceInfo->name) == name)
                _streamParameters.device = i;
        }

        if (_streamParameters.device >= 0)
            Log::get() << Log::MESSAGE << "Sound_Engine::" << __FUNCTION__ << " - Connecting to device: " << name << " as " << (_inputDevice ? "input" : "output") << Log::endl;
    }
#endif

    if (_streamParameters.device < 0)
    {
        if (_inputDevice)
            _streamParameters.device = Pa_GetDefaultInputDevice();
        else
            _streamParameters.device = Pa_GetDefaultOutputDevice();
        if (_streamParameters.device == paNoDevice)
        {
            Log::get() << Log::WARNING << "Sound_Engine::" << __FUNCTION__ << " - Could not find default audio output device" << Log::endl;
            _connected = false;
            return false;
        }
    }

    _connected = true;
    auto deviceInfo = Pa_GetDeviceInfo(_streamParameters.device);
    Log::get() << Log::MESSAGE << "Sound_Engine::" << __FUNCTION__ << " - Connected to device: " << deviceInfo->name << Log::endl;

    return true;
}

/*************/
bool Sound_Engine::setParameters(double sampleRate, SampleFormat sampleFormat, int channelCount, unsigned long framesPerBuffer)
{
    if (!_connected)
        return false;

    lock_guard<mutex> lock(_engineMutex);

    auto deviceInfo = Pa_GetDeviceInfo(_streamParameters.device);
    if (sampleRate == 0.0)
        _sampleRate = deviceInfo->defaultSampleRate;
    else
        _sampleRate = sampleRate;

    _framesPerBuffer = framesPerBuffer;
    _sampleFormat = sampleFormat;
    _channels = channelCount;

    _streamParameters.channelCount = _channels;
    switch (_sampleFormat)
    {
    default:
        Log::get() << Log::WARNING << "Sound_Engine::" << __FUNCTION__ << " - Unsupported sample format" << Log::endl;
        return false;
    case SAMPLE_FMT_U8:
        _streamParameters.sampleFormat = paUInt8;
        _sampleSize = sizeof(unsigned char);
        _planar = false;
        break;
    case SAMPLE_FMT_S16:
        _streamParameters.sampleFormat = paInt16;
        _sampleSize = sizeof(short);
        _planar = false;
        break;
    case SAMPLE_FMT_S32:
        _streamParameters.sampleFormat = paInt32;
        _sampleSize = sizeof(int);
        _planar = false;
        break;
    case SAMPLE_FMT_FLT:
        _streamParameters.sampleFormat = paFloat32;
        _sampleSize = sizeof(float);
        _planar = false;
        break;
    case SAMPLE_FMT_U8P:
        _streamParameters.sampleFormat = paUInt8;
        _planar = true;
        _sampleSize = sizeof(unsigned char);
        break;
    case SAMPLE_FMT_S16P:
        _streamParameters.sampleFormat = paInt16;
        _sampleSize = sizeof(short);
        _planar = true;
        break;
    case SAMPLE_FMT_S32P:
        _streamParameters.sampleFormat = paInt32;
        _sampleSize = sizeof(int);
        _planar = true;
        break;
    case SAMPLE_FMT_FLTP:
        _streamParameters.sampleFormat = paFloat32;
        _sampleSize = sizeof(float);
        _planar = true;
        break;
    }

    _streamParameters.suggestedLatency = Pa_GetDeviceInfo(_streamParameters.device)->defaultLowInputLatency;
    _streamParameters.hostApiSpecificStreamInfo = nullptr;

    return true;
}

/*************/
bool Sound_Engine::startStream(PaStreamCallback* callback, void* userData)
{
    lock_guard<mutex> lock(_engineMutex);

    int error;
    if (_inputDevice)
        error = Pa_OpenStream(&_portAudioStream, &_streamParameters, nullptr, _sampleRate, _framesPerBuffer, paClipOff, callback, userData);
    else
        error = Pa_OpenStream(&_portAudioStream, nullptr, &_streamParameters, _sampleRate, _framesPerBuffer, paClipOff, callback, userData);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Sound_Engine::" << __FUNCTION__ << " - Could not open PortAudio stream: " << Pa_GetErrorText(error) << Log::endl;
        return false;
    }

    error = Pa_StartStream(_portAudioStream);
    if (error != paNoError)
    {
        Log::get() << Log::WARNING << "Sound_Engine::" << __FUNCTION__ << " - Could not start PortAudio stream: " << Pa_GetErrorText(error) << Log::endl;
        return false;
    }

    Log::get() << Log::MESSAGE << "Sound_Engine::" << __FUNCTION__ << " - Successfully opened PortAudio stream" << Log::endl;

    return true;
}
}
