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
 * @log.h
 * The Log class
 */

#ifndef SPLASH_LOG_H
#define SPLASH_LOG_H

#include <chrono>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "./config.h"

#include "./core/coretypes.h"
#include "./core/spinlock.h"

#define SPLASH_LOG_FILE "/var/log/splash.log"

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
     * \brief Get the singleton
     */
    static Log& get()
    {
        static auto instance = new Log;
        return *instance;
    }

    /**
     * \brief Shortcut for any type of log
     * \param p Message priority
     * \param args Message
     */
    template <typename... T>
    void operator()(Priority p, T... args)
    {
        std::lock_guard<Spinlock> lock(_mutex);
        rec(p, args...);
    }

    /**
     * \brief Shortcut for setting MESSAGE log
     * \param msg Message
     * \return Return this Log object
     */
    template <typename T>
    Log& operator<<(const T& msg)
    {
        std::lock_guard<Spinlock> lock(_mutex);
        addToString(_tempString, msg);
        return *this;
    }

    /**
     * \brief Shortcut for setting a MESSAGE log from a Value
     * \param v Value to set
     * \return Return this Log object
     */
    Log& operator<<(const Value& v)
    {
        std::lock_guard<Spinlock> lock(_mutex);
        addToString(_tempString, v.as<std::string>());
        return *this;
    }

    /**
     * \brief Set an action
     * \param action Action to set
     * \return Return this Log object
     */
    Log& operator<<(Log::Action action)
    {
        std::lock_guard<Spinlock> lock(_mutex);
        if (action == endl)
        {
            if (_tempPriority >= _verbosity)
                rec(_tempPriority, _tempString);
            _tempString.clear();
            _tempPriority = MESSAGE;
        }
        return *this;
    }

    /**
     * \brief Set the priority
     * \param p Priority
     * \return Return this Log object
     */
    Log& operator<<(Log::Priority p)
    {
        std::lock_guard<Spinlock> lock(_mutex);
        _tempPriority = p;
        return *this;
    }

    /**
     * \brief Get the full logs
     * \return Return the full logs
     */
    std::deque<std::tuple<uint64_t, std::string, Priority>> getFullLogs() { return _logs; }

    /**
     * \brief Get the logs by priority
     * \param args Priorities
     * \return Return the logs
     */
    template <typename... T>
    std::vector<std::string> getLogs(T... args)
    {
        std::lock_guard<Spinlock> lock(_mutex);
        std::vector<Log::Priority> priorities{args...};
        std::vector<std::string> logs;
        for (auto log : _logs)
            for (auto p : priorities)
                if (std::get<2>(log) == p)
                    logs.push_back(std::get<1>(log));

        return logs;
    }

    /**
     * \brief Get the new logs (from last call to this method)
     * \return Return the new logs
     */
    std::vector<std::tuple<uint64_t, std::string, Priority>> getNewLogs()
    {
        std::lock_guard<Spinlock> lock(_mutex);
        std::vector<std::tuple<uint64_t, std::string, Priority>> logs;
        for (uint32_t i = _logPointer; i < _logs.size(); ++i)
            logs.push_back(_logs[i]);
        _logPointer = _logs.size();
        return logs;
    }

    /**
     * \brief Get the verbosity of the console output
     * \return Return the verbosity (= the priority)
     */
    Priority getVerbosity() { return _verbosity; }

    /**
     * \brief Activate logging to /var/log/splash.log
     * \param active Activated if true
     */
    void logToFile(bool activate) { _logToFile = activate; }

    /**
     * \brief Set the verbosity of the console output
     * \param p Priority
     */
    void setVerbosity(Priority p) { _verbosity = p; }

    /**
     * \brief Add new logs from an outside source, i.e. another process
     * \param log Log
     * \param priority Priority
     */
    void setLog(uint64_t timestamp, const std::string& log, Priority priority)
    {
        std::lock_guard<Spinlock> lock(_mutex);
        _logs.push_back(std::make_tuple(timestamp, log, priority));

        if (_logs.size() > _logLength)
        {
            _logPointer = _logPointer > 0 ? _logPointer - 1 : _logPointer;
            _logs.pop_front();
        }
    }

  private:
    /**
     * \brief Constructor
     */
    Log() {}

    /**
     * \brief Destructor
     */
    ~Log() {}

    /**
     * Delete some constructors
     */
    Log(const Log&) = delete;
    const Log& operator=(const Log&) = delete;

  private:
    mutable Spinlock _mutex;
    std::deque<std::tuple<uint64_t, std::string, Priority>> _logs;
    bool _logToFile{false};
    uint32_t _logLength{500};
    int _logPointer{0};
    Priority _verbosity{MESSAGE};

    std::string _tempString;
    Priority _tempPriority{MESSAGE};

    /*****/
    template <typename T, typename... Ts>
    void addToString(std::string& str, const T& t, Ts&... args) const
    {
        str += std::to_string(t);
        addToString(str, args...);
    }

    template <typename... Ts>
    void addToString(std::string& str, const std::string& s, Ts&... args) const
    {
        str += s;
        addToString(str, args...);
    }

    template <typename... Ts>
    void addToString(std::string& str, const char* s, Ts&... args) const
    {
        str += std::string(s);
        addToString(str, args...);
    }

    void addToString(std::string&) const { return; }

    /**
     * \brief Set a new log message
     */
    template <typename... T>
    void rec(Priority p, T... args)
    {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::string message("");
        addToString(message, args...);

        // Write to log file, if we may
        if (_logToFile)
        {
            std::ofstream logFile(SPLASH_LOG_FILE, std::ostream::out | std::ostream::app);
            if (logFile.good())
            {
                logFile << formatMessage(now, message, p) << "\n";
                logFile.close();
            }
        }

        // Write to console
        if (p >= _verbosity)
            toConsole(formatMessage(now, message, p));

        uint64_t timeAsUsecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        _logs.push_back(std::make_tuple(timeAsUsecs, message, p));
        if (_logs.size() > _logLength)
        {
            _logPointer = _logPointer > 0 ? _logPointer - 1 : _logPointer;
            _logs.erase(_logs.begin());
        }
    }

    /*********/
    std::string formatMessage(const std::chrono::system_clock::time_point& timestamp, const std::string& message, Priority priority)
    {
        std::time_t now_c = std::chrono::system_clock::to_time_t(timestamp);
        char time_c[64];
        strftime(time_c, 64, "%FT%T", std::localtime(&now_c));

        std::string type;
        if (priority == Priority::MESSAGE)
            type = std::string("[MESSAGE]");
        else if (priority == Priority::DEBUGGING)
            type = std::string(" [DEBUG] ");
        else if (priority == Priority::WARNING)
            type = std::string("[WARNING]");
        else if (priority == Priority::ERROR)
            type = std::string(" [ERROR] ");

        return std::string(time_c) + " / " + type + " / " + message;
    }

    /*****/
    void toConsole(const std::string& message)
    {
        auto msg = message;
        if (msg.find("[MESSAGE]") != std::string::npos)
            msg.replace(msg.find("[MESSAGE]"), 9, "\033[32;1m[MESSAGE]\033[0m");
        else if (msg.find("[DEBUG]") != std::string::npos)
            msg.replace(msg.find("[DEBUG]"), 7, "\033[36;1m[DEBUG]\033[0m");
        else if (msg.find("[WARNING]") != std::string::npos)
            msg.replace(msg.find("[WARNING]"), 9, "\033[33;1m[WARNING]\033[0m");
        else if (msg.find("[ERROR]") != std::string::npos)
            msg.replace(msg.find("[ERROR]"), 7, "\033[31;1m[ERROR]\033[0m");

        std::cout << msg << "\n";
    }
};

} // namespace Splash

#endif // SPLASH_LOG_H
