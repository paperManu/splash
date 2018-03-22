#include "./speaker.h"

#include "./utils/log.h"
#include "./utils/timer.h"

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
    std::lock_guard<std::mutex> lock(_ringWriteMutex);
    _ringReadPosition = 0;
    _ringWritePosition = 0;
    _ringUnusedSpace = 0;
}

/*************/
void Speaker::setParameters(uint32_t channels, uint32_t sampleRate, Sound_Engine::SampleFormat format, const string& deviceName)
{
    _channels = std::max((uint32_t)1, channels);
    _sampleRate = std::max((uint32_t)1, sampleRate);
    _sampleFormat = format;
    _deviceName = deviceName;

    Log::get() << Log::MESSAGE << "Speaker::" << __FUNCTION__ << " - Set audio output: " << _sampleRate << "kHz on " << _channels << " channels" << Log::endl;

    initResources();
}

/*************/
void Speaker::freeResources()
{
    if (!_ready)
        return;

    _abortCallback = true;
    _ready = false;
}

/*************/
void Speaker::initResources()
{
    if (_ready)
        freeResources();

    if (!_engine.getDevice(false, _deviceName))
        return;

    if (!_engine.setParameters(_sampleRate, _sampleFormat, _channels, 1024))
        return;

    _engine.getParameters(_sampleRate, _sampleSize, _planar);

    if (!_engine.startStream(Speaker::portAudioCallback, this))
        return;

    _ready = true;
}

/*************/
int Speaker::portAudioCallback(
    const void* /*in*/, void* out, uint64_t framesPerBuffer, const PaStreamCallbackTimeInfo* /*timeInfo*/, PaStreamCallbackFlags /*statusFlags*/, void* userData)
{
    auto that = static_cast<Speaker*>(userData);
    uint8_t* output = (uint8_t*)out;

    if (!output)
        return paContinue;

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
        for (int i = 0; i < step; ++i)
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
    return paContinue;
}

/*************/
void Speaker::registerAttributes()
{
    BaseObject::registerAttributes();
}

} // end of namespace
