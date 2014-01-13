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

/*
 * @world.h
 * The World class
 */

#ifndef WORLD_H
#define WORLD_H

#include <string>
#include <vector>
#include <json/reader.h>

#include "config.h"
#include "coretypes.h"
#include "log.h"
#include "scene.h"

namespace Splash {

class World {
    public:
        /**
         * Constructor
         */
        World(int argc, char** argv);

        /**
         * Destructor
         */
        ~World();

        /**
         * Get the status of the world
         */
        bool getStatus() const {return _status;}

        /**
         * Run the world
         */
        void run();

    private:
        bool _status {true};
        std::map<std::string, ScenePtr> _scenes;

        Json::Value _config;

        /**
         * Apply the configuration
         */
        void applyConfig();

        /**
         * Load the specified configuration file
         */
        bool loadConfig(std::string filename);

        /**
         * Parse the given arguments
         */
        void parseArguments(int argc, char** argv);
};

} // end of namespace

 #endif // WORLD_H
