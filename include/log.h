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
 * @log.h
 * The Log class
 */

#ifndef SPLASH_LOG_H
#define SPLASH_LOG_H

#include "config.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <utility>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "coretypes.h"

namespace Splash
{

/*************/
class Log
{
    public:
        enum Priority
        {
            DEBUGGING = 0,
            MESSAGE,
            WARNING,
            ERROR,
            NONE
        };

        enum Action
        {
            endl = 0
        };

        /**
         * Constructor
         */
        Log()
        {
        }

        /**
         * Destructor
         */
        ~Log()
        {
        }

        /**
         * Shortcut for any type of log
         */
        template<typename ... T>
        void operator()(Priority p, T ... args)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            rec(p, args...);
        }

        /**
         * Shortcut for setting MESSAGE log
         */
        template <typename T>
        Log& operator<<(T msg)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            addToString(_tempString, msg);
            return *this;
        }

        Log& operator<<(Value v)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            addToString(_tempString, v.asString());
            return *this;
        }

        Log& operator<<(Log::Action action)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (action == endl)
            {
                if (_tempPriority >= _verbosity)
                    rec(_tempPriority, _tempString);
                _tempString.clear();
                _tempPriority = MESSAGE;
            }
            return *this;
        }

        Log& operator<<(Log::Priority p)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _tempPriority = p;
            return *this;
        }

        /**
         * Get the full logs
         */
        std::vector<std::pair<std::string, Priority>> getFullLogs()
        {
            return _logs;
        }

        /**
         * Get the logs by priority
         */
        template<typename ... T>
        std::vector<std::string> getLogs(T ... args)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            std::vector<Log::Priority> priorities {args...};
            std::vector<std::string> logs;
            for (auto log : _logs)
                for (auto p : priorities)
                    if (log.second == p)
                        logs.push_back(log.first);

            return logs;
        }

        /**
         * Get the verbosity of the console output
         */
        Priority getVerbosity()
        {
            return _verbosity;
        }

        /**
         * Set the verbosity of the console output
         */
        void setVerbosity(Priority p)
        {
            _verbosity = p;
        }

    private:
        mutable std::mutex _mutex;
        std::vector<std::pair<std::string, Priority>> _logs;
        int _logLength {5000};
        Priority _verbosity {MESSAGE};

        std::string _tempString;
        Priority _tempPriority {MESSAGE};

        /*****/
        template <typename T, typename... Ts>
        void addToString(std::string& str, const T& t, Ts& ... args) const
        {
            str += std::to_string(t);
            addToString(str, args...);
        }

        template <typename... Ts>
        void addToString(std::string& str, const std::string& s, Ts& ...args) const
        {
            str += s;
            addToString(str, args...);
        }

        template <typename... Ts>
        void addToString(std::string& str, const char* s, Ts& ...args) const
        {
            str += std::string(s);
            addToString(str, args...);
        }

        void addToString(std::string& str) const
        {
            return;
        }

        /**
         * Set a new log message
         */
        template<typename ... T>
        void rec(Priority p, T ... args)
        {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            char time_c[64];
            strftime(time_c, 64, "%T", std::localtime(&now_c));

            std::string timedMsg;
            std::string type;
            if (p == Priority::MESSAGE)
                type = std::string("[MESSAGE]");
            else if (p == Priority::DEBUGGING)
                type = std::string(" [DEBUG] ");
            else if (p == Priority::WARNING)
                type = std::string("[WARNING]");
            else if (p == Priority::ERROR)
                type = std::string(" [ERROR] ");

            timedMsg = std::string(time_c) + std::string(" / ") + type + std::string(" / ");

            addToString(timedMsg, args...);

            if (p >= _verbosity)
                toConsole(timedMsg);

            _logs.push_back(std::pair<std::string, Priority>(timedMsg, p));
            if (_logs.size() > _logLength)
            {
                _logs.erase(_logs.begin());
            }
        }

        /*****/
        void toConsole(std::string msg)
        {
            std::cout << msg << std::endl;
        }
};

struct SLog
{
    public:
        static Log log;
};

} // end of namespace

#endif // SPLASH_LOG_H
