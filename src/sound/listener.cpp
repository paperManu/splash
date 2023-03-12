#include "./sound/listener.h"

#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
Listener::Listener()
    : GraphObject(nullptr)
{
    registerAttributes();
}

/*************/
Listener::~Listener()
{
    freeResources();
}

/*************/
void Listener::setParameters(uint32_t channels, uint32_t sampleRate, Sound_Engine::SampleFormat format, const std::string& deviceName)
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
    auto that = static_cast<Listener*>(userData);
    auto input = static_cast<const uint8_t*>(in);

    if (!input)
        return paContinue;

    const uint32_t readPosition = that->_ringReadPosition;
    uint32_t writePosition = that->_ringWritePosition;

    uint32_t delta = 0;
    if (readPosition > writePosition)
        delta = readPosition - writePosition;
    else
        delta = _ringbufferSize - writePosition + readPosition;

    const uint32_t step = static_cast<uint32_t>(framesPerBuffer) * that->_channels * that->_sampleSize;
    if (delta < step)
        return paContinue;

    uint32_t spaceLeft = _ringbufferSize - writePosition - 1;

    if (spaceLeft < step)
        writePosition = 0;
    else
        spaceLeft = 0;

    std::copy(input, input + step, &that->_ringBuffer[writePosition]);
    writePosition = writePosition + step;

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
    GraphObject::registerAttributes();
}

} // namespace Splash
