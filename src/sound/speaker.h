/*
 * Copyright (C) 2015 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @speaker.h
 * The Speaker class, to output sound
 */

#ifndef SPLASH_SPEAKER_H
#define SPLASH_SPEAKER_H

#define SPLASH_SPEAKER_RINGBUFFER_SIZE (4 * 1024 * 1024)

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "./config.h"
#include "./core/attribute.h"
#include "./core/graph_object.h"
#include "./sound/sound_engine.h"

namespace Splash
{

/*************/
class Speaker : public GraphObject
{
  public:
    /**
     * \brief Constructor
     */
    Speaker();

    /**
     * \brief Destructor
     */
    ~Speaker() final;

    /**
     * \brief Safe bool idiom
     */
    explicit operator bool() const { return _ready; }

    /**
     * No copy, but some move constructors
     */
    Speaker(const Speaker&) = delete;
    Speaker& operator=(const Speaker&) = delete;

    /**
     * \brief Add a buffer to the playing queue
     * \param buffer Buffer to add
     * \return Return false if there was an error
     */
    template <typename T>
    bool addToQueue(const ResizableArray<T>& buffer);

    /**
     * \brief Clear the queue
     */
    void clearQueue();

    /**
     * \brief Set the audio parameters
     * \param channels Channel count
     * \param sampleRate Sample rate
     * \param format Sample format
     */
    void setParameters(uint32_t channels, uint32_t sampleRate, Sound_Engine::SampleFormat format, const std::string& deviceName = "");

  private:
    Sound_Engine _engine;
    bool _ready{false};
    unsigned int _channels{2};
    bool _planar{false};
    unsigned int _sampleRate{44100};
    Sound_Engine::SampleFormat _sampleFormat{Sound_Engine::SAMPLE_FMT_S16};
    size_t _sampleSize{2};
    std::string _deviceName{""};

    bool _abortCallback{false};

    std::array<uint8_t, SPLASH_SPEAKER_RINGBUFFER_SIZE> _ringBuffer;
    std::atomic_int _ringWritePosition{0};
    std::atomic_int _ringReadPosition{0};
    std::atomic_int _ringUnusedSpace{0};
    std::mutex _ringWriteMutex;

    /**
     * \brief Free all PortAudio resources
     */
    void freeResources();

    /**
     * \brief Initialize PortAudio resources
     */
    void initResources();

    /**
     * \brief PortAudio callback
     * \param in Unused
     * \param out Pointer to output data
     * \param framesPerBuffer Frame count
     * \param timeInfo Unused
     * \param userData Pointer to this object
     */
    static int portAudioCallback(
        const void* in, void* out, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

/*************/
template <typename T>
bool Speaker::addToQueue(const ResizableArray<T>& buffer)
{
    std::lock_guard<std::mutex> lock(_ringWriteMutex);

    // If the input buffer is planar, we need to interlace it
    ResizableArray<T> interleavedBuffer(buffer.size());
    if (_planar)
    {
        size_t step = _sampleSize / sizeof(T);
        uint32_t sampleNbr = (buffer.size() / step) / _channels;
        int linesize = sampleNbr * step;
        for (uint32_t channel = 0; channel < _channels; ++channel)
            for (uint32_t sample = 0; sample < sampleNbr; ++sample)
            {
                int index = sample * step + channel * linesize;
                std::copy(&buffer[index], &buffer[index + step], &interleavedBuffer[(sample * _channels + channel) * step]);
            }
    }

    auto bufferSampleSize = sizeof(T);
    int readPosition = _ringReadPosition;
    int writePosition = _ringWritePosition;

    int delta = 0;
    if (readPosition > writePosition)
        delta = readPosition - writePosition;
    else
        delta = SPLASH_SPEAKER_RINGBUFFER_SIZE - writePosition + readPosition;

    if (delta < static_cast<int>(buffer.size() * bufferSampleSize))
        return false;

    int spaceLeft = SPLASH_SPEAKER_RINGBUFFER_SIZE - writePosition;

    const uint8_t* bufferPtr;
    if (_planar)
        bufferPtr = static_cast<const uint8_t*>(interleavedBuffer.data());
    else
        bufferPtr = static_cast<const uint8_t*>(buffer.data());

    if (spaceLeft < static_cast<int>(buffer.size() * bufferSampleSize))
    {
        _ringUnusedSpace = spaceLeft;
        writePosition = 0;
    }

    std::copy(bufferPtr, bufferPtr + buffer.size() * bufferSampleSize, &_ringBuffer[writePosition]);
    writePosition = (writePosition + buffer.size() * bufferSampleSize);

    _ringWritePosition = writePosition;
    return true;
}

} // end of namespace

#endif
