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
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
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
            SLog::log.setVerbosity(Log::DEBUGGING);
            idx++;
        }
        else
        {
            name = argv[idx];
            idx++;
        }
    }

    SLog::log << "splashScene::main - Creating Scene with name " << name << Log::endl;

    ScenePtr scene(new Scene(name));
    scene->setName(name);

    timespec nap;
    nap.tv_sec = 0;
    nap.tv_nsec = 5e8;
    while (scene->isRunning())
    {
        nanosleep(&nap, NULL);
    }

    return 0;
}
