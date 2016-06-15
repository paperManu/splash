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

#define SPLASH_SPEAKER_RINGBUFFER_SIZE (8 * 1024 * 1024)

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
    
#include <portaudio.h>

#include "config.h"
#include "basetypes.h"

namespace Splash {

/*************/
class Speaker : public BaseObject
{
    public:
        enum SampleFormat
        {
            SAMPLE_FMT_UNKNOWN = -1,
            SAMPLE_FMT_U8 = 0,
            SAMPLE_FMT_S16,
            SAMPLE_FMT_S32,
            SAMPLE_FMT_FLT,
            SAMPLE_FMT_U8P,
            SAMPLE_FMT_S16P,
            SAMPLE_FMT_S32P,
            SAMPLE_FMT_FLTP
        };
        
        /**
         * Constructor
         */
        Speaker();

        /**
         * Destructor
         */
        ~Speaker();

        /**
         * Safe bool idiom
         */
        explicit operator bool() const
        {
            return _ready;
        }

        /**
         * No copy, but some move constructors
         */
        Speaker(const Speaker&) = delete;
        Speaker& operator=(const Speaker&) = delete;

        /**
         * Add a buffer to the playing queue
         * Returns false if there was an error
         */
        template<typename T>
        bool addToQueue(const ResizableArray<T>& buffer);

        /**
         * Clear the queue
         */
        void clearQueue();

        /**
         * Set the audio parameters
         */
        void setParameters(uint32_t channels, uint32_t sampleRate, SampleFormat format);

    private:
        bool _ready {false};
        unsigned int _channels {2};
        bool _planar {false};
        unsigned int _sampleRate {44100};
        SampleFormat _sampleFormat {SAMPLE_FMT_S16};
        size_t _sampleSize {2};

        PaStream* _portAudioStream {nullptr};
        bool _abortCallback {false};

        std::array<uint8_t, SPLASH_SPEAKER_RINGBUFFER_SIZE> _ringBuffer;
        std::atomic_int _ringWritePosition {0};
        std::atomic_int _ringReadPosition {0};
        std::atomic_int _ringUnusedSpace {0};
        std::mutex _ringWriteMutex;

        /**
         * Free all PortAudio resources
         */
        void freeResources();

        /**
         * Initialize PortAudio resources
         */
        void initResources();

        /**
         * PortAudio callback
         */
        static int portAudioCallback(const void* in, void* out, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

/*************/
template<typename T>
bool Speaker::addToQueue(const ResizableArray<T>& buffer)
{
    std::lock_guard<std::mutex> lock(_ringWriteMutex);

    // If the input buffer is planar, we need to interlace it
    ResizableArray<T> interleavedBuffer(buffer.size());
    if (_planar)
    {
        int sampleNbr = (buffer.size() / _sampleSize) / _channels;
        for (unsigned int i = 0; i < buffer.size(); i += _sampleSize)
        {
            int channel = (i / _sampleSize) / sampleNbr;
            int sample = (i / _sampleSize) % sampleNbr;
            std::copy(buffer[i], buffer[i + _sampleSize], interleavedBuffer[(sample * _channels + channel) * _sampleSize]);
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

    if (delta < buffer.size() * bufferSampleSize)
        return false;

    int spaceLeft = SPLASH_SPEAKER_RINGBUFFER_SIZE - writePosition;

    const uint8_t* bufferPtr;
    if (_planar)
        bufferPtr = static_cast<const uint8_t*>(interleavedBuffer.data());
    else
        bufferPtr = static_cast<const uint8_t*>(buffer.data());

    if (spaceLeft < buffer.size() * bufferSampleSize)
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
