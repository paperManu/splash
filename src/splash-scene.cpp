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
 * @splash-scene.cpp
 * Subprogram which handles the display on a single display adapter
 */

#include "splash.h"

#include <getopt.h>

using namespace Splash;
using namespace std;

/*************/
int main(int argc, char** argv)
{
    string name = "scene";
    string socketPrefix = "";

    while (true)
    {
        static struct option longOptions[] = {{"debug", no_argument, 0, 'd'}, {"prefix", required_argument, 0, 'p'}, {"timer", no_argument, 0, 't'}, {0, 0, 0, 0}};

        int optionIndex = 0;
        auto ret = getopt_long(argc, argv, "dp:t", longOptions, &optionIndex);

        if (ret == -1)
            break;

        switch (ret)
        {
        case 'd':
        {
            Log::get().setVerbosity(Log::DEBUGGING);
            break;
        }
        case 'p':
        {
            socketPrefix = string(optarg);
            break;
        }
        case 't':
        {
            Timer::get().setDebug(true);
            break;
        }
        }
    }

    if (optind < argc)
        name = string(argv[argc - 1]);

    Log::get() << "splashScene::main - Creating Scene with name " << name << Log::endl;

    Scene scene(name, socketPrefix);
    scene.run();

    return 0;
}
