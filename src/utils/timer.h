/*
 * Copyright (C) 2013 Emmanuel Durand
 *
 * This file is part of Log.
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
 * @timer.h
 * The Timer class
 */

#ifndef SPLASH_TIMER_H
#define SPLASH_TIMER_H

#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>

#include "./config.h"
#include "./core/coretypes.h"
#include "./core/spinlock.h"

namespace Splash
{

class Timer
{
  public:
    struct Point
    {
        uint32_t years{0};
        uint32_t months{0};
        uint32_t days{0};
        uint32_t hours{0};
        uint32_t mins{0};
        uint32_t secs{0};
        uint32_t frame{0};
        bool paused{false};
    };

    /**
     * \brief Get the singleton
     * \return Return the Timer singleton
     */
    static Timer& get()
    {
        static auto instance = new Timer;
        return *instance;
    }

    /**
     * \brief Returns whether the timer is set to debug mode
     * \return Return true if it is
     */
    bool isDebug() { return _isDebug; }

    /**
     * \brief Set the timer in debug mode
     * \param d If true, set to debug mode
     */
    void setDebug(bool d) { _isDebug = d; }

    /**
     * \brief Set the master clock to be a loose contraint
     * \param loose If true, the master clock is loose
     */
    void setLoose(bool loose) { _looseClock = loose; }

    /**
     * \brief Get whether the clock is loose or not
     */
    bool isLoose() const { return _looseClock; }

    /**
     * \brief Start a duration measurement
     * \param name Duration name
     */
    void start(const std::string& name)
    {
        if (!_enabled)
            return;

        auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        auto timeIt = _timeMap.find(name);
        if (timeIt == _timeMap.end())
            _timeMap[name].store(currentTime, std::memory_order_acq_rel);
        else
            timeIt->second = currentTime;
    }

    /**
     * \brief End a duration measurement
     * \param name Duration name
     */
    void stop(const std::string& name)
    {
        if (!_enabled)
            return;

        auto timeIt = _timeMap.find(name);
        if (timeIt != _timeMap.end())
        {
            auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            auto durationIt = _durationMap.find(name);
            if (durationIt == _durationMap.end())
                _durationMap[name].store(currentTime - timeIt->second.load(std::memory_order_acq_rel), std::memory_order_acq_rel);
            else
                durationIt->second = currentTime - timeIt->second.load(std::memory_order_acq_rel);
        }
    }

    /**
     * \brief Wait for the specified timer to reach a certain value, in us
     * \param name Duration name
     * \param duration Desired duration
     * \return Return false if the timer does not exist
     */
    bool waitUntilDuration(const std::string& name, unsigned long long duration)
    {
        if (!_enabled)
            return false;

        if (_timeMap.find(name) == _timeMap.end())
            return false;

        auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        auto timeIt = _timeMap.find(name);
        auto durationIt = _durationMap.find(name);
        unsigned long long elapsed;

        elapsed = currentTime - timeIt->second.load(std::memory_order_acq_rel);

        timespec nap;
        nap.tv_sec = 0;
        bool overtime = false;
        if (elapsed < duration)
            nap.tv_nsec = (duration - elapsed) * 1e3;
        else
        {
            nap.tv_nsec = 0;
            overtime = true;
        }

        if (durationIt == _durationMap.end())
            _durationMap[name].store(std::max(duration, elapsed), std::memory_order_acq_rel);
        else
            durationIt->second = std::max(duration, elapsed);

        nanosleep(&nap, NULL);

        return overtime;
    }

    /**
     * \brief Get the last occurence of the specified duration
     * \param name Duration name
     * \return Return the duration in us
     */
    unsigned long long getDuration(const std::string& name) const
    {
        auto durationIt = _durationMap.find(name);
        if (durationIt == _durationMap.end())
            return 0;
        return durationIt->second;
    }

    /**
     * \brief Get the whole duration map
     * \return Return the whole duration map
     */
    const std::unordered_map<std::string, std::atomic_ullong>& getDurationMap() const { return _durationMap; }

    /**
     * \brief Set an element in the duration map. Used for transmitting timings between pairs
     * \param name Duration name
     * \param value Duration in us
     */
    void setDuration(const std::string& name, unsigned long long value)
    {
        auto durationIt = _durationMap.find(name);
        if (durationIt == _durationMap.end())
            _durationMap[name].store(value, std::memory_order_acq_rel);
        else
            durationIt->second = value;
    }

    /**
     * \brief Return the duration since the last call with this name, or 0 if it is the first time.
     * \param name Duration name
     * \return Return the duration
     */
    unsigned long long sinceLastSeen(const std::string& name)
    {
        if (_timeMap.find(name) == _timeMap.end())
        {
            start(name);
            return 0;
        }

        stop(name);
        unsigned long long duration = getDuration(name);
        start(name);
        return duration;
    }

    /**
     * Some facilities
     */
    Timer& operator<<(const std::string& name)
    {
        start(name);
        _currentDuration.store(0, std::memory_order_acq_rel);
        return *this;
    }

    Timer& operator>>(unsigned long long duration)
    {
        _timerMutex.lock(); // We lock the mutex to prevent this value to be reset by another call to timer
        _currentDuration.store(duration, std::memory_order_acq_rel);
        _durationThreadId = std::this_thread::get_id();
        _isDurationSet = true;
        return *this;
    }

    bool operator>>(const std::string& name)
    {
        unsigned long long duration = 0;
        if (_isDurationSet && _durationThreadId == std::this_thread::get_id())
        {
            _isDurationSet = false;
            duration = _currentDuration.load(std::memory_order_acq_rel);
            _currentDuration.store(0, std::memory_order_acq_rel);
            _timerMutex.unlock();
        }

        bool overtime = false;
        if (duration > 0)
            overtime = waitUntilDuration(name, duration);
        else
            stop(name);
        return overtime;
    }

    unsigned long long operator[](const std::string& name) { return getDuration(name); }

    /**
     * \brief Enable / disable the timers
     */
    void setStatus(bool enabled) { _enabled = enabled; }

    /**
     * \brief Set the master clock time
     * \param clock Master clock value
     */
    void setMasterClock(const Timer::Point& clock)
    {
        std::lock_guard<Spinlock> lockClock(_clockMutex);
        _clockSet = true;
        _clock = clock;
        _lastMasterClockUpdate = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
    }

    /**
     * \brief Get the master clock time
     * \param clock Master clock value
     * \return Return true if the master clock is set
     */
    bool getMasterClock(Timer::Point& clock) const
    {
        if (_clockSet)
        {
            std::lock_guard<Spinlock> lockClock(_clockMutex);
            clock = _clock;
            return true;
        }
        else
        {
            return false;
        }
    }

    /**
     * \brief Get the master clock time, corrected from the last update time
     * \param time Master clock time, unit based on template parameter
     * \param paused True if the clock is paused
     * \return Return true if the master clock is set
     */
    template <typename T>
    bool getMasterClock(int64_t& time, bool& paused) const
    {
        if (!_clockSet)
        {
            paused = false;
            return false;
        }

        std::unique_lock<Spinlock> lockClock(_clockMutex);
        auto clock = _clock;
        auto lastMasterClockUpdate = _lastMasterClockUpdate;
        lockClock.unlock();

        int64_t frames = clock.frame + (clock.secs + (clock.mins + (clock.hours + clock.days * 24ll) * 60ll) * 60ll) * 120ll;
        std::chrono::microseconds useconds((frames * 1000000) / 120);
        auto currentTime = std::chrono::microseconds(getTime());
        time = std::chrono::duration_cast<T>(useconds).count();

        paused = clock.paused;
        // If the clock is not paused, we correct the received master clock to add time since last update
        if (!paused)
            time += std::chrono::duration_cast<T>(currentTime - lastMasterClockUpdate).count();

        if (_looseClock && paused)
        {
            time += std::chrono::duration_cast<T>(std::chrono::steady_clock::now().time_since_epoch() - lastMasterClockUpdate).count();
            paused = false;
        }

        return true;
    }

    /**
     * \brief Get the current time in us from epoch
     * \return Return the duration since epoch
     */
    static inline int64_t getTime() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

  private:
    Timer() {}
    ~Timer() {}
    Timer(const Timer&) = delete;
    const Timer& operator=(const Timer&) = delete;

  private:
    std::unordered_map<std::string, std::atomic_ullong> _timeMap;
    std::unordered_map<std::string, std::atomic_ullong> _durationMap;
    std::atomic_ullong _currentDuration{0};
    bool _isDurationSet{false};
    std::thread::id _durationThreadId;
    mutable Spinlock _timerMutex;
    mutable Spinlock _clockMutex;
    bool _enabled{true};
    bool _isDebug{false};
    bool _looseClock{false};
    std::chrono::microseconds _lastMasterClockUpdate{};
    Timer::Point _clock;
    bool _clockSet{false};
};

} // namespace Splash

#endif // SPLASH_TIMER_H
