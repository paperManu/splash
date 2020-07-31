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
 * @osutils.h
 * Some system utilities
 */

#ifndef SPLASH_OSUTILS_H
#define SPLASH_OSUTILS_H

#include <algorithm>
#include <filesystem>
#include <pwd.h>
#include <sched.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "./core/constants.h"
#include "./utils/log.h"

#if HAVE_SHMDATA
#include <shmdata/abstract-logger.hpp>
#endif
#if HAVE_CALIMIRO
#include <calimiro/abstract_logger.h>
#endif

namespace Splash
{
namespace Utils
{
/**
 * \brief Get the current thread id
 * \return Return the thread id
 */
#if HAVE_LINUX
inline int getThreadId()
{
    pid_t threadId;
    threadId = syscall(SYS_gettid);
    return threadId;
}
#endif

/**
 * \brief Get the CPU core count
 * \return Return the core count
 */
inline int getCoreCount()
{
    return sysconf(_SC_NPROCESSORS_CONF);
}

/**
 * \brief Set the CPU core affinity. If one of the specified cores is not reachable, does nothing.
 * \param cores Vector of the target cores
 * \return Return true if all went well
 */
inline bool setAffinity(const std::vector<int>& cores)
{
#if HAVE_LINUX
    auto ncores = getCoreCount();
    for (auto& core : cores)
        if (core >= ncores)
            return false;

    cpu_set_t set;
    CPU_ZERO(&set);
    for (auto& core : cores)
        CPU_SET(core, &set);

    if (sched_setaffinity(getThreadId(), sizeof(set), &set) != 0)
        return false;

    return true;
#else
    return false;
#endif
}

/**
 * \brief Set the current thread as realtime (nice = 99, SCHED_RR)
 * \return Return true if it was able to set the scheduling
 */
inline bool setRealTime()
{
#if HAVE_LINUX
    sched_param params;
    params.sched_priority = 99;
    if (sched_setscheduler(getThreadId(), SCHED_RR, &params) != 0)
        return false;

    return true;
#else
    return false;
#endif
}

/**
 * Utils function to handle ioctl being interrupted
 * See SA_RESTART here: http://pubs.opengroup.org/onlinepubs/009695399/functions/sigaction.html
 * \param fd File descriptor
 * \param request Request
 * \param arg Arguments
 * \return Return 0 if all went well, see manpage for ioctl otherwise
 */
inline int xioctl(int fd, int request, void* arg)
{
    int res;
    do {
        res = ioctl(fd, request, arg);
    } while(res == -1 && errno == EINTR);

    return res;
}

/**
 * Converts a string to lower case
 * \param input string to convert
 * \return the string in lower case format
 */
inline void toLower(std::string& input)
{
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
}

/**
 * \brief Check whether a path is a directory
 * \param filepath Path to test
 * \param Return true if the path is a directory
 */
inline bool isDir(const std::string& filepath)
{
    return std::filesystem::is_directory(filepath);
}

/**
 * \brief Clean up a path, removing extra slashes and such
 * \param filepath Path to clean
 * \return Return the path cleaned
 */
inline std::string cleanPath(const std::string& filepath)
{
    return std::filesystem::path(filepath).lexically_normal();
}

/**
 * \brief Get the current user home path
 * \return Return the home path
 */
inline std::string getHomePath()
{
    if (getenv("HOME"))
        return std::string(getenv("HOME"));

    struct passwd* pw = getpwuid(getuid());
    return std::string(pw->pw_dir);
}

/**
 * \brief Get the directory path from the file path.
 * \param filepath File path
 * \param configPath Configuration path
 * \return Return the path
 * If a config path is given, a relative file path is evaluated from there. Otherwise it is evaluated from the current working directory.
 */
inline std::string getPathFromFilePath(const std::string& filepath, const std::string& configPath = "")
{
    auto path = std::filesystem::path(filepath);

    if(path.is_absolute())
        return path.remove_filename().lexically_normal();

    // The path is relative
    path = path.remove_filename().lexically_normal();
    if (configPath.empty())
        return std::filesystem::current_path() / path;
    else
        return std::filesystem::path(configPath).lexically_normal() / path;
}

/**
 * Get the current working directory
 * \return Return the working directory
 */
inline std::string getCurrentWorkingDirectory()
{
    return std::filesystem::current_path();
}

/**
 * \brief Extract the filename from a file path
 * \param filepath File path
 * \return Return the file name
 */
inline std::string getFilenameFromFilePath(const std::string& filepath)
{
    return std::filesystem::path(filepath).filename();
}

/**
 * Get the full file path from a filename or file path
 * \param filepath File name or path
 * \param configurationPath Configuration path
 * \return Return the full path
 */
inline std::string getFullPathFromFilePath(const std::string& filepath, const std::string& configurationPath)
{
    auto path = Utils::getPathFromFilePath(filepath, configurationPath);
    return path / std::filesystem::path(filepath).filename();
}

/**
 * \brief Get a list of the entries in a directory
 * \param path Directory path
 * \return Return the entries list
 * Note that an entry can be a file or a directory
 * In the case where the path is a file, it is processed as its parent directory instead
 */
inline std::vector<std::string> listDirContent(const std::string& path)
{
    auto clean_path = std::filesystem::path(path).lexically_normal();
    // Special case where the path is a file -> we consider its parent directory instead
    if (std::filesystem::is_regular_file(clean_path))
        clean_path = clean_path.remove_filename();

    std::vector<std::string> files{};
    auto dir_iterator = std::filesystem::directory_iterator(clean_path);
    std::transform(std::filesystem::begin(dir_iterator), std::filesystem::end(dir_iterator), std::back_inserter(files), [](auto dir_entry) { return dir_entry.path(); });

    return files;
}

/**
 * \brief Get the file descriptor from a file path.
 * \param filepath File path to look for
 * \return Return the file descriptor, or 0 if it was not able to find the file in the list of opened file.
 */
inline int getFileDescriptorForOpenedFile(const std::string& filepath)
{
#if HAVE_LINUX
    auto pid = getpid();
    auto procDir = "/proc/" + std::to_string(pid) + "/fd/";
    auto files = listDirContent(procDir);

    char* realPath = nullptr;
    for (auto& file : files)
    {
        realPath = realpath((procDir + file).c_str(), nullptr);
        if (realPath)
        {
            auto linkedPath = std::string(realPath);
            free(realPath);
            if (filepath == linkedPath)
                return std::stoi(file);
        }
    }
#endif
    return 0;
}

/**
 * \brief Get the path of the currently executed file
 * \return Return the path as a string
 */
inline std::string getCurrentExecutablePath()
{
    std::string currentExePath = "";
#if HAVE_LINUX
    auto pid = getpid();
    auto procExe = "/proc/" + std::to_string(pid) + "/exe";
    char* realPath = realpath(procExe.c_str(), nullptr);
    if (realPath)
    {
        currentExePath = std::string(realPath);
        free(realPath);
    }
#endif
    return currentExePath;
}

#if HAVE_SHMDATA
/**
 * \brief Shmdata logger dedicated to splash
 */
class ShmdataLogger : public shmdata::AbstractLogger
{
  private:
    void on_error(std::string&& str) final { Log::get() << Log::ERROR << "Shmdata::ShmdataLogger - " << str << Log::endl; }
    void on_critical(std::string&& str) final { Log::get() << Log::ERROR << "Shmdata::ShmdataLogger - " << str << Log::endl; }
    void on_warning(std::string&& str) final { Log::get() << Log::WARNING << "Shmdata::ShmdataLogger - " << str << Log::endl; }
    void on_message(std::string&& str) final { Log::get() << Log::MESSAGE << "Shmdata::ShmdataLogger - " << str << Log::endl; }
    void on_info(std::string&& str) final { Log::get() << Log::MESSAGE << "Shmdata::ShmdataLogger - " << str << Log::endl; }
    void on_debug(std::string&& str) final { Log::get() << Log::DEBUGGING << "Shmdata::ShmdataLogger - " << str << Log::endl; }
};
#endif

#if HAVE_CALIMIRO
/**
 * \brief Calimiro logger dedicated to splash
 */
class CalimiroLogger : public calimiro::AbstractLogger
{
  private:
    void on_error(std::string&& str) final { Log::get() << Log::ERROR << "Calimiro::CalimiroLogger - " << str << Log::endl; }
    void on_critical(std::string&& str) final { Log::get() << Log::ERROR << "Calimiro::CalimiroLogger - " << str << Log::endl; }
    void on_warning(std::string&& str) final { Log::get() << Log::WARNING << "Calimiro::CalimiroLogger - " << str << Log::endl; }
    void on_message(std::string&& str) final { Log::get() << Log::MESSAGE << "Calimiro::CalimiroLogger - " << str << Log::endl; }
    void on_info(std::string&& str) final { Log::get() << Log::MESSAGE << "Calimiro::CalimiroLogger - " << str << Log::endl; }
    void on_debug(std::string&& str) final { Log::get() << Log::DEBUGGING << "Calimiro::CalimiroLogger - " << str << Log::endl; }
};
#endif
} // namespace Utils
} // namespace Splash

#endif
