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

#ifndef LOG_H
#define LOG_H

#include "config.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <utility>
#include <string>
#include <vector>

namespace Splash {

class Log {
    public:
        enum Priority
        {
            MESSAGE = 0,
            DEBUG = 1,
            WARNING = 2,
            ERROR = 3
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
            if (p == MESSAGE)
                type = std::string("MESSAGE");
            else if (p == DEBUG)
                type = std::string("DEBUG");
            else if (p == WARNING)
                type = std::string("WARNING");
            else if (p == ERROR)
                type = std::string("ERROR");

            timedMsg = std::string(time_c) + std::string(" - [") + type + std::string("] - ");

            auto values = {args...};
            for (auto v : values)
            {
                timedMsg += std::string(v);
            }

            toConsole(timedMsg);

            _logs.push_back(std::pair<std::string, Priority>(timedMsg, p));
            if (_logs.size() > _logLength)
                _logs.erase(_logs.begin());
        }

        /**
         * Shortcut for any type of log
         */
        template<typename ... T>
        void operator()(Priority p, T ... args)
        {
            rec(p, args...);
        }

        /**
         * Shortcut for setting MESSAGE log
         */
        void operator<<(std::string msg)
        {
            rec(MESSAGE, msg);
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
        std::vector<std::string> getLogs(Priority p)
        {
            std::vector<std::string> logs;
            for (auto log : _logs)
                if (log.second == p)
                    logs.push_back(log.first);

            return logs;
        }

    private:
        std::vector<std::pair<std::string, Priority>> _logs;
        int _logLength = {500};
        bool _verbose = {true};

        void toConsole(std::string msg)
        {
            std::cout << msg << std::endl;
        }
};

static Log gLog;

} // end of namespace

#endif // LOG_H
