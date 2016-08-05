/*
 * Copyright (C) 2014 Emmanuel Durand
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
 * @mesh_shmdata.h
 * The Mesh_Shmdata_Shmdata class
 */

#ifndef SPLASH_MESH_SHMDATA_H
#define SPLASH_MESH_SHMDATA_H

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <shmdata/follower.hpp>

#include "config.h"

#include "mesh.h"
#include "osUtils.h"

namespace Splash {

class Mesh_Shmdata : public Mesh
{
    public:
        /**
         * Constructor
         */
        Mesh_Shmdata();
        Mesh_Shmdata(std::weak_ptr<RootObject> root);

        /**
         * Destructor
         */
        ~Mesh_Shmdata();

        /**
         * No copy constructor, only move
         */
        Mesh_Shmdata(const Mesh_Shmdata&) = delete;
        Mesh_Shmdata& operator=(const Mesh_Shmdata&) = delete;

        /**
         * Set the path to read from
         */
        bool read(const std::string& filename);

    protected:
        std::string _caps {""};
        Utils::ConsoleLogger _logger;
        std::unique_ptr<shmdata::Follower> _reader {nullptr};
        bool _capsIsValid {false};

        /**
         * Base init for the class
         */
        void init();

        /**
         * \brief Callback called when receiving a new caps
         * \param dataType String holding the data type
         */
        void onCaps(const std::string& dataType);
        void onData(void* data, int data_size);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

} // end of namespace

#endif // SPLASH_MESH_SHMDATA_H
