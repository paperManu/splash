#include "./sound/listener.h"

#include "./utils/log.h"
#include "./utils/timer.h"

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
void Listener::setParameters(uint32_t channels, uint32_t sampleRate, Sound_Engine::SampleFormat format, const string& deviceName)
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

    _abortCallback = true;
}

/*************/
void Listener::initResources()
{
    if (_ready)
        freeResources();

    if (!_engine.getDevice(true, _deviceName))
        return;

    if (!_engine.setParameters(_sampleRate, _sampleFormat, _channels, 256))
        return;

    _engine.getParameters(_sampleRate, _sampleSize, _planar);
    if (!_engine.startStream(Listener::portAudioCallback, this))
        return;

    _ready = true;
}

/*************/
int Listener::portAudioCallback(
    const void* in, void* /*out*/, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* /*timeInfo*/, PaStreamCallbackFlags /*statusFlags*/, void* userData)
{
    auto that = (Listener*)userData;
    uint8_t* input = (uint8_t*)in;

    if (!input)
        return paContinue;

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

    if (that->_abortCallback)
        return paComplete;
    else
        return paContinue;
}

/*************/
void Listener::registerAttributes()
{
    BaseObject::registerAttributes();
}

} // end of namespace
