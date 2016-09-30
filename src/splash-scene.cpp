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

using namespace Splash;
using namespace std;

/*************/
int main(int argc, char** argv)
{
    string name = "scene";
    int idx = 0;
    while (idx < argc)
    {
        if (string(argv[idx]) == "-d")
        {
            Log::get().setVerbosity(Log::DEBUGGING);
            idx++;
        }
        else if (string(argv[idx]) == "-t" || string(argv[idx]) == "--timer")
        {
            Timer::get().setDebug(true);
            idx++;
        }
        else
        {
            name = argv[idx];
            idx++;
        }
    }

    Log::get() << "splashScene::main - Creating Scene with name " << name << Log::endl;

    Scene scene(name);
    scene.run();

    return 0;
}
