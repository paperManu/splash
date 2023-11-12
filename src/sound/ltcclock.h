/*
 * Copyright (C) 2015 Splash authors
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
 * @ltcclock.h
 * The LtcClock class, which reads a clock on an audio input
 */

#ifndef SPLASH_LTCCLOCK_H
#define SPLASH_LTCCLOCK_H

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <ltc.h>
#include <portaudio.h>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/graph_object.h"
#include "./sound/listener.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
class LtcClock final : public GraphObject
{
  public:
    /**
     * Constructor
     */
    explicit LtcClock(bool masterClock = false, const std::string& deviceName = "");

    /**
     * Destructor
     */
    ~LtcClock() final;

    /**
     * Constructors/operators
     */
    LtcClock(const LtcClock&) = delete;
    LtcClock& operator=(const LtcClock&) = delete;
    LtcClock(LtcClock&&) = delete;
    LtcClock& operator=(LtcClock&&) = delete;

    /**
     * Safe bool idiom
     */
    explicit operator bool() const { return _ready; }

    /**
     * Get the clock as a Clock struct
     * \return Return a Timer::Point
     */
    Timer::Point getClock();

  private:
    bool _ready{false};
    bool _masterClock{false};
    bool _continue{false};
    std::thread _ltcThread;

    bool _framerateChanged{false};
    uint8_t _previousFrame{0};
    uint8_t _maximumFramePerSec{30};

    Timer::Point _clock;
    std::unique_ptr<Listener> _listener;

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes() {}
};

} // namespace Splash

#endif
