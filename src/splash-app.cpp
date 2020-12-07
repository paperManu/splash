/*
 * Copyright (C) 2013 Emmanuel Durand
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

/**
 * @splash.cpp
 * The main program from the Splash suite.
 */

#include <getopt.h>
#include <iostream>
#include <optional>
#include <regex>
#include <string>

#include "./core/constants.h"
#include "./core/scene.h"
#include "./core/world.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

using namespace Splash;

/*************/
RootObject::Context parseArguments(int argc, char** argv)
{
    RootObject::Context context;

    // Get the executable directory
    context.executableName = argv[0];
    context.executablePath = Utils::getCurrentExecutablePath();

    // Parse the other args
    context.configurationFile = std::string(DATADIR) + "splash.json";

    while (true)
    {
        static struct option longOptions[] = {
            {"debug", no_argument, 0, 'd'},
#if HAVE_LINUX
            {"forceDisplay", required_argument, 0, 'D'},
            {"displayServer", required_argument, 0, 'S'},
#endif
            {"help", no_argument, 0, 'h'},
            {"hide", no_argument, 0, 'H'},
            {"info", no_argument, 0, 'i'},
            {"log2file", no_argument, 0, 'l'},
            {"open", required_argument, 0, 'o'},
            {"prefix", required_argument, 0, 'p'},
            {"python", required_argument, 0, 'P'},
            {"silent", no_argument, 0, 's'},
            {"timer", no_argument, 0, 't'},
            {"child", no_argument, 0, 'c'},
            {"doNotSpawn", no_argument, 0, 'x'},
            {0, 0, 0, 0}
        };

        int optionIndex = 0;
        auto ret = getopt_long(argc, argv, "+cdD:S:hHilo:p:P:stx", longOptions, &optionIndex);

        if (ret == -1)
            break;

        switch (ret)
        {
        default:
        case 'h':
        {
            std::cout << "Basic usage: splash [options] [config.json] -- [python script argument]\n";
            std::cout << "Options:\n";
            std::cout << "\t-o (--open) [filename] : set [filename] as the configuration file to open\n";
            std::cout << "\t-d (--debug) : activate debug messages (if Splash was compiled with -DDEBUG)\n";
            std::cout << "\t-t (--timer) : activate more timers, at the cost of performance\n";
#if HAVE_LINUX
            std::cout << "\t-D (--forceDisplay) : force the display on which to show all windows\n";
            std::cout << "\t-S (--displayServer) : set the display server ID\n";
#endif
            std::cout << "\t-s (--silent) : disable all messages\n";
            std::cout << "\t-i (--info) : get description for all objects attributes\n";
            std::cout << "\t-H (--hide) : run Splash in background\n";
            std::cout << "\t-P (--python) : add the given Python script to the loaded configuration\n";
            std::cout << "                  any argument after -- will be sent to the script\n";
            std::cout << "\t-l (--log2file) : write the logs to /var/log/splash.log, if possible\n";
            std::cout << "\t-p (--prefix) : set the shared memory socket paths prefix (defaults to the PID)\n";
            std::cout << "\t-c (--child): run as a child controlled by a master Splash process\n";
            std::cout << "\t-x (--doNotSpawn): do not spawn subprocesses, which have to be ran manually\n";
            std::cout << "\n";
            exit(0);
        }
        case 'd':
        {
            Log::get().setVerbosity(Log::DEBUGGING);
            break;
        }
        case 'D':
        {
            auto regDisplayFull = std::regex("(:[0-9]\\.[0-9])", std::regex_constants::extended);
            auto regDisplayInt = std::regex("[0-9]", std::regex_constants::extended);
            std::smatch match;

            context.forcedDisplay = std::string(optarg);
            if (std::regex_match(*context.forcedDisplay, match, regDisplayFull))
            {
                Log::get() << Log::MESSAGE << "Splash::" << __FUNCTION__ << " - Display forced to " << *context.forcedDisplay << Log::endl;
            }
            else if (std::regex_match(*context.forcedDisplay, match, regDisplayInt))
            {
                Log::get() << Log::MESSAGE << "Splash::" << __FUNCTION__ << " - Display forced to :0." << *context.forcedDisplay << Log::endl;
            }
            else
            {
                Log::get() << Log::WARNING << "Splash::" << __FUNCTION__ << " - " << std::string(optarg) << ": argument expects a positive integer, or a string in the form of \":x.y\""
                           << Log::endl;
                exit(0);
            }
            break;
        }
        case 'S':
        {
            auto regInt = std::regex("[0-9]+", std::regex_constants::extended);
            std::smatch match;

            context.displayServer = std::string(optarg);
            if (std::regex_match(context.displayServer, match, regInt))
            {
                Log::get() << Log::MESSAGE << "Splash::" << __FUNCTION__ << " - Display server forced to :" << context.displayServer << Log::endl;
            }
            else
            {
                Log::get() << Log::WARNING << "Splash::" << __FUNCTION__ << " - " << std::string(optarg) << ": argument expects a positive integer" << Log::endl;
                exit(0);
            }
            break;
        }
        case 'H':
        {
            context.hide = true;
            break;
        }
        case 'P':
        {
            context.pythonScriptPath = Utils::getFullPathFromFilePath(std::string(optarg), Utils::getCurrentWorkingDirectory());
            context.pythonArgs.push_back(*context.pythonScriptPath);

            // Build the Python arg list
            bool isPythonArg = false;
            for (int i = 0; i < argc; ++i)
            {
                if (!isPythonArg && "--" == std::string(argv[i]))
                {
                    isPythonArg = true;
                    continue;
                }
                else if (!isPythonArg)
                {
                    continue;
                }
                else
                {
                    context.pythonArgs.push_back(std::string(argv[i]));
                }
            }
            break;
        }
        case 'i':
        {
            context.info = true;
            break;
        }
        case 'l':
        {
            context.log2file = true;
            break;
        }
        case 'o':
        {
            context.defaultConfigurationFile = false;
            context.configurationFile = std::string(optarg);
            break;
        }
        case 'p':
        {
            context.socketPrefix = std::string(optarg);
            break;
        }
        case 's':
        {
            Log::get().setVerbosity(Log::NONE);
            break;
        }
        case 't':
        {
            Timer::get().setDebug(true);
            break;
        }
        case 'c':
        {
            context.childProcess = true;
            break;
        }
        case 'x':
        {
            context.spawnSubprocesses = false;
            break;
        }
        }
    }

    // Find last argument index, or "--"
    int lastArgIndex = 0;
    for (; lastArgIndex < argc; ++lastArgIndex)
        if (std::string(argv[lastArgIndex]) == "--")
            break;

    std::string lastArg = "";
    if (optind < lastArgIndex)
        lastArg = std::string(argv[optind]);

    if (context.childProcess)
    {
        if (!lastArg.empty())
            context.childSceneName = lastArg;
    }
    else
    {
        if (!lastArg.empty())
        {
            context.defaultConfigurationFile = false;
            context.configurationFile = lastArg;
        }
    }

    return context;
}

/*************/
int main(int argc, char** argv)
{
    auto context = parseArguments(argc, argv);

    if (context.childProcess)
    {
        Scene scene(context);
        scene.run();

        return 0;
    }
    else
    {
        std::cout << "\n";
        std::cout << "\t             \033[33;1m- Splash -\033[0m\n";
        std::cout << "\t\033[1m- Modular multi-output video mapper -\033[0m\n";
        std::cout << "\t          \033[1m- Version " << PACKAGE_VERSION << " -\033[0m\n";
        std::cout << "\n";

        World world(context);
        world.run();
        return world.getStatus();
    }
}
