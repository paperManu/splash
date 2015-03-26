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
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @timer.h
 * The Timer class
 */

#ifndef SPLASH_TIMER_H
#define SPLASH_TIMER_H

#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <mutex>
#include <string>

#include "config.h"

namespace Splash
{

class Timer
{
    using Timestamp = std::pair<std::string, unsigned long long>;

    public:
        Timer() {}
        ~Timer() {}

        /**
         * Start / end a timer
         */
        void start(std::string name)
        {
            if (!_enabled)
                return;
            _mutex.lock();

            auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto timeIt = _timeMap.find(name);
            if (timeIt == _timeMap.end())
                _timeMap.insert(Timestamp(name, currentTime));
            else
                timeIt->second = currentTime;

            _mutex.unlock();
        }

        void stop(std::string name)
        {
            if (!_enabled)
                return;
            _mutex.lock();

            auto timeIt = _timeMap.find(name);
            if (timeIt != _timeMap.end())
            {
                auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                auto durationIt = _durationMap.find(name);
                if (durationIt == _durationMap.end())
                    _durationMap.insert(Timestamp(name, currentTime));
                else
                    durationIt->second = currentTime - timeIt->second;
            }
            _mutex.unlock();
        }

        /**
         * Wait for the specified timer to reach a certain value, in us
         */
         bool waitUntilDuration(std::string name, unsigned long long duration)
         {
            if (!_enabled)
                return false;

            if (_timeMap.find(name) == _timeMap.end())
                return false;

            auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto timeIt = _timeMap.find(name);
            auto durationIt = _durationMap.find(name);
            unsigned long long elapsed;
            {
                _mutex.lock();
                elapsed = currentTime - timeIt->second;
                _timeMap.erase(timeIt);
                _mutex.unlock();
            }

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

            {
                _mutex.lock();
                if (durationIt == _durationMap.end())
                    _durationMap.insert(Timestamp(name, std::max(duration, elapsed)));
                else
                    durationIt->second = std::max(duration, elapsed);
                _mutex.unlock();
            }
            nanosleep(&nap, NULL);

            return overtime;
         }

         /**
          * Get the last occurence of the specified duration
          */
         unsigned long long getDuration(std::string name)
         {
            auto durationIt = _durationMap.find(name);
            if (durationIt == _durationMap.end())
                return 0;
            unsigned long long duration;
            {
                _mutex.lock();
                duration = durationIt->second;
                _mutex.unlock();
            }
            return duration;
         }

         /**
          * Get the whole time map
          */
         std::map<std::string, unsigned long long> getDurationMap()
         {
            _mutex.lock();
            auto durationMap = _durationMap;
            _mutex.unlock();
            return durationMap;
         }

         /**
          * Set an element in the duration map. Used for transmitting timings between pairs
          */
         void setDuration(std::string name, unsigned long long value)
         {
            _mutex.lock();
            auto durationIt = _durationMap.find(name);
            if (durationIt == _durationMap.end())
                _durationMap.insert(Timestamp(name, value));
            else
                durationIt->second = value;
            _mutex.unlock();
         }

         /**
          * Return the time since the last call with this name,
          * or 0 if it is the first time
          */
         unsigned long long sinceLastSeen(std::string name)
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
         Timer& operator<<(std::string name)
         {
            start(name);
            _mutex.lock();
            _currentDuration = 0;
            _mutex.unlock();
            return *this;
         }

         Timer& operator>>(unsigned long long duration)
         {
            _mutex.lock(); // We lock the mutex to prevent this value to be reset by another call to timer
            _currentDuration = duration;
            _isDurationSet = true;
            return *this;
         }

         bool operator>>(std::string name)
         {
            unsigned long long duration = 0;
            if (_isDurationSet)
            {
                _isDurationSet = false;
                duration = _currentDuration;
                _currentDuration = 0;
                _mutex.unlock();
            }

            bool overtime = false;
            if (duration > 0)
                overtime = waitUntilDuration(name, duration);
            else
                stop(name);
            return overtime;
         }

         unsigned long long operator[](std::string name) {return getDuration(name);}

         /**
          * Enable / disable the timers
          */
         void setStatus(bool enabled) {_enabled = enabled;}

    private:
        std::map<std::string, unsigned long long> _timeMap; 
        std::map<std::string, unsigned long long> _durationMap;
        unsigned long long _currentDuration {0};
        bool _isDurationSet {false};
        std::mutex _mutex;
        bool _enabled {true};
};

struct STimer
{
    public:
        static Timer timer;
};

} // end of namespace

#endif // SPLASH_TIMER_H
