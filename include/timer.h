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

#ifndef TIMER_H
#define TIMER_H

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
    public:
        Timer() {}
        ~Timer() {}

        /**
         * Start / end a timer
         */
        void start(std::string name)
        {
            _timeMap[name] = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        }

        void stop(std::string name)
        {
            if (_timeMap.find(name) != _timeMap.end())
            {
                auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                _durationMap[name] = now - _timeMap[name];
            }
        }

        /**
         * Wait for the specified timer to reach a certain value, in us
         */
         void waitUntilDuration(std::string name, unsigned long long duration)
         {
            if (_timeMap.find(name) == _timeMap.end())
                return;

            auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto elapsed = now - _timeMap[name];
            _timeMap.erase(name);

            timespec nap;
            nap.tv_sec = 0;
            if (elapsed < duration)
                nap.tv_nsec = (duration - elapsed) * 1e3;
            else
                nap.tv_nsec = 0;

            std::lock_guard<std::mutex> lock(_mutex);
            _durationMap[name] = std::max(duration, elapsed);
            nanosleep(&nap, NULL);
         }

         /**
          * Get the last occurence of the specified duration
          */
         unsigned long long getDuration(std::string name)
         {
            if (_durationMap.find(name) == _durationMap.end())
                return 0;
            std::lock_guard<std::mutex> lock(_mutex);
            return _durationMap[name];
         }

         /**
          * Some facilities
          */
         Timer& operator<<(std::string name)
         {
            start(name);
            _currentDuration = 0;
            return *this;
         }

         Timer& operator>>(unsigned long long duration)
         {
            _currentDuration = duration;
            return *this;
         }

         Timer& operator>>(std::string name)
         {
            if (_currentDuration > 0)
                waitUntilDuration(name, _currentDuration);
            else
                stop(name);
            return *this;
         }

         unsigned long long operator[](std::string name)
         {
            return getDuration(name);
         }

    private:
        std::map<std::string, unsigned long long> _timeMap; 
        std::map<std::string, unsigned long long> _durationMap;
        unsigned long long _currentDuration;
        std::mutex _mutex;
};

struct STimer
{
    public:
        static Timer timer;
};

} // end of namespace

#endif // TIMER_H
