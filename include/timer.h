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

#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <string>
#include <thread>

#include "config.h"
#include "coretypes.h"

namespace Splash
{

class Timer
{
    public:
        /**
         * Get the singleton
         */
        static Timer& get()
        {
            static auto instance = new Timer;
            return *instance;
        }

        /**
         * Returns whether the timer is set to debug mode
         */
        bool isDebug() {return _isDebug;}
        void setDebug(bool d) {_isDebug = d;}

        /**
         * Start / end a timer
         */
        void start(const std::string& name)
        {
            if (!_enabled)
                return;

            auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto timeIt = _timeMap.find(name);
            if (timeIt == _timeMap.end())
                _timeMap[name] = currentTime;
            else
                timeIt->second = currentTime;
        }

        void stop(const std::string& name)
        {
            if (!_enabled)
                return;

            auto timeIt = _timeMap.find(name);
            if (timeIt != _timeMap.end())
            {
                auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

                auto durationIt = _durationMap.find(name);
                if (durationIt == _durationMap.end())
                    _durationMap[name] = currentTime - timeIt->second;
                else
                    durationIt->second = currentTime - timeIt->second;
            }
        }

        /**
         * Wait for the specified timer to reach a certain value, in us
         */
        bool waitUntilDuration(const std::string& name, unsigned long long duration)
        {
           if (!_enabled)
               return false;
        
           if (_timeMap.find(name) == _timeMap.end())
               return false;
        
           auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
           auto timeIt = _timeMap.find(name);
           auto durationIt = _durationMap.find(name);
           unsigned long long elapsed;

           elapsed = currentTime - timeIt->second;
        
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
               _durationMap[name] = std::max(duration, elapsed);
           else
               durationIt->second = std::max(duration, elapsed);
        
           nanosleep(&nap, NULL);
        
           return overtime;
        }
        
        /**
         * Get the last occurence of the specified duration
         */
        unsigned long long getDuration(const std::string& name) const
        {
           auto durationIt = _durationMap.find(name);
           if (durationIt == _durationMap.end())
               return 0;
           return durationIt->second;
        }
        
        /**
         * Get the whole time map
         */
        const std::unordered_map<std::string, std::atomic_ullong>& getDurationMap() const
        {
           return _durationMap;
        }
        
        /**
         * Set an element in the duration map. Used for transmitting timings between pairs
         */
        void setDuration(const std::string& name, unsigned long long value)
        {
           auto durationIt = _durationMap.find(name);
           if (durationIt == _durationMap.end())
               _durationMap[name] = value;
           else
               durationIt->second = value;
        }
        
        /**
         * Return the time since the last call with this name,
         * or 0 if it is the first time
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
           _currentDuration = 0;
           return *this;
        }
        
        Timer& operator>>(unsigned long long duration)
        {
           _timerMutex.lock(); // We lock the mutex to prevent this value to be reset by another call to timer
           _currentDuration = duration;
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
               duration = _currentDuration;
               _currentDuration = 0;
               _timerMutex.unlock();
           }
        
           bool overtime = false;
           if (duration > 0)
               overtime = waitUntilDuration(name, duration);
           else
               stop(name);
           return overtime;
        }
        
        unsigned long long operator[](const std::string& name) {return getDuration(name);}
        
        /**
         * Enable / disable the timers
         */
        void setStatus(bool enabled) {_enabled = enabled;}
        
        /**
         * Master clock related
         */
        void setMasterClock(const Values& clock)
        {
            if (clock.size() == 8)
            {
                std::unique_lock<std::mutex> lockClock(_clockMutex);
                _clock = clock;
            }
        }
        
        bool getMasterClock(Values& clock) const
        {
            if (_clock.size() > 0)
            {
                std::unique_lock<std::mutex> lockClock(_clockMutex);
                clock = _clock;
                return true;
            }
            else
            {
                return false;
            }
        }

        template <typename T>
        bool getMasterClock(int64_t& time, bool& paused) const
        {
            if (_clock.size() == 0)
            {
                paused = true;
                return false;
            }

            std::unique_lock<std::mutex> lockClock(_clockMutex);
            auto clock = _clock;
            lockClock.unlock();

            int64_t frames = clock[6].asInt() + (clock[5].asInt() + (clock[4].asInt() + (clock[3].asInt() + clock[2].asInt()) * 24) * 60) * 120;
            std::chrono::microseconds useconds((frames * 1000000) / 120);
            time = std::chrono::duration_cast<T>(useconds).count();

            paused = clock[7].asInt();

            return true;
        }

        template <typename T>
        static inline int64_t getTime()
        {
            return std::chrono::duration_cast<T>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        }

    private:
        Timer() {}
        ~Timer() {}
        Timer(const Timer&) = delete;
        const Timer& operator=(const Timer&) = delete;

    private:
        std::unordered_map<std::string, std::atomic_ullong> _timeMap; 
        std::unordered_map<std::string, std::atomic_ullong> _durationMap;
        std::atomic_ullong _currentDuration {0};
        bool _isDurationSet {false};
		std::thread::id _durationThreadId;
        mutable std::mutex _timerMutex;
        mutable std::mutex _clockMutex;
        bool _enabled {true};
        bool _isDebug {false};
        Values _clock;
};

} // end of namespace

#endif // SPLASH_TIMER_H
