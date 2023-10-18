/*
 * Copyright (C) 2017 Emmanuel Durand
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
 * @sound_engine.h
 * The Sound Engine class, wrapping sound api stuff
 */

#ifndef SPLASH_SOUND_ENGINE_H
#define SPLASH_SOUND_ENGINE_H

#include <mutex>
#include <portaudio.h>
#include <string>

#include "./core/constants.h"

namespace Splash
{

class Sound_Engine
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
    Sound_Engine();

    /**
     * Destructor
     */
    ~Sound_Engine();

    /**
     * Constructors/operators
     */
    Sound_Engine(const Sound_Engine&) = default;
    Sound_Engine& operator=(const Sound_Engine&) = default;
    Sound_Engine(Sound_Engine&&) = default;
    Sound_Engine& operator=(Sound_Engine&&) = default;

    /**
     * Get device by name, or default device
     * \param inputDevice Set to true to get an input device, false otherwise
     * \param name Device name
     * \return Return true if all went well
     */
    bool getDevice(bool inputDevice = false, const std::string& name = "");

    /**
     * Get the stream parameters
     * \param sampleRate Sample rate
     * \param sampleSize Sample size
     * \param planar True if data stream is planar
     */
    void getParameters(unsigned int& sampleRate, size_t& sampleSize, bool& planar)
    {
        sampleRate = _sampleRate;
        sampleSize = _sampleSize;
        planar = _planar;
    }

    /**
     * Set the stream parameters
     * \param sampleRate Sample rate
     * \param sampleFormat Sample format
     * \param channelCount Channel count
     * \param framesPerBuffer Frames per buffer
     * \return Return true if all went well
     */
    bool setParameters(double sampleRate, SampleFormat sampleFormat, int channelCount, unsigned long framesPerBuffer);

    /**
     * Start the stream
     * \return Return true if all went well
     */
    bool startStream(PaStreamCallback* callback, void* userData);

  private:
    static std::mutex _engineMutex;
    bool _ready{false};
    bool _connected{false};

    bool _inputDevice{false};
    unsigned int _channels{2};
    bool _planar{false};
    unsigned int _sampleRate{44100};
    SampleFormat _sampleFormat{SAMPLE_FMT_S16};
    size_t _sampleSize{2};
    unsigned long _framesPerBuffer{256};
    std::string _deviceName{""};

    PaStreamParameters _streamParameters{};
    PaStream* _portAudioStream{nullptr};
};

} // namespace Splash

#endif // SPLASH_SOUND_ENGINE_H
