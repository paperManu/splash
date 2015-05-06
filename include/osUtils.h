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
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @osUtils.h
 * Some system utilities
 */

#ifndef SPLASH_OSUTILS_H
#define SPLASH_OSUTILS_H

#include <string>
#include <unistd.h>
#include <shmdata/abstract-logger.hpp>

#include "log.h"

namespace Splash
{
    namespace Utils
    {
        /*****/
        inline std::string getPathFromFilePath(const std::string& filepath)
        {
            size_t slashPos = filepath.rfind("/");
            bool isRelative = filepath.find(".") == 0 ? true : false;
            bool isAbsolute = filepath.find("/") == 0 ? true : false;
            auto filePath = std::string("");
            if (slashPos != std::string::npos)
            {
                if (isAbsolute)
                    filePath = filepath.substr(0, slashPos) + "/";
                else if (isRelative)
                {
                    char workingPathChar[256];
                    auto workingPath = std::string(getcwd(workingPathChar, 255));
                    if (filepath.find("/") == 1)
                        filePath = workingPath + filepath.substr(1, slashPos) + "/";
                    else if (filepath.find("/") == 2)
                        filePath = workingPath + "/" + filepath.substr(0, slashPos) + "/";
                }
            }
            return filePath;
        }

        /*****/
        inline std::string getFilenameFromFilePath(const std::string& filepath)
        {
            size_t slashPos = filepath.rfind("/");
            auto filename = std::string("");
            if (slashPos == std::string::npos)
                filename = filepath;
            else
                filename = filepath.substr(slashPos);
            return filename;
        }
    
        /*****/
        // A shmdata logger dedicated to splash
        class ConsoleLogger: public shmdata::AbstractLogger
        {
            private:
                void on_error(std::string &&str) final
                {
                    Log::get() << Log::ERROR << "Shmdata::ConsoleLogger - " << str << Log::endl;
                }
                void on_critical(std::string &&str) final
                {
                    Log::get() << Log::ERROR << "Shmdata::ConsoleLogger - " << str << Log::endl;
                }
                void on_warning(std::string &&str) final
                {
                    Log::get() << Log::WARNING << "Shmdata::ConsoleLogger - " << str << Log::endl;
                }
                void on_message(std::string &&str) final
                {
                    Log::get() << Log::MESSAGE << "Shmdata::ConsoleLogger - " << str << Log::endl;
                }
                void on_info(std::string &&str) final
                {
                    Log::get() << Log::MESSAGE << "Shmdata::ConsoleLogger - " << str << Log::endl;
                }
                void on_debug(std::string &&str) final
                {
                    Log::get() << Log::DEBUGGING << "Shmdata::ConsoleLogger - " << str << Log::endl;
                }
        };
    } // end of namespace
} // end of namespace

#endif
