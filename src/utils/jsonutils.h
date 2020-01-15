/*
 * Copyright (C) 2018 Emmanuel Durand
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
 * @jsonutils.h
 * Utilities to handle Json files
 */

#ifndef SPLASH_JSONUTILS_H
#define SPLASH_JSONUTILS_H

#include <fstream>
#include <string>
#include <vector>

#include <json/json.h>

#include "./core/constants.h"

#include "./core/value.h"
#include "./utils/log.h"

namespace Splash
{
namespace Utils
{

/**
 * Check the given configuration, and upgrades it if necessary
 * \param configuration Configuration to upgrade
 * \return Return true if configuration is a valid Splash configuration
 */
bool checkAndUpgradeConfiguration(Json::Value& configuration);

/**
 * Load a Json file
 * \param filename Json file path
 * \param configuration Holds the Json tree
 * \return Return true if everything went well
 */
bool loadJsonFile(const std::string& filename, Json::Value& configuration);

/**
 * Helper function to convert Json::Value to Splash::Values
 * \param values JSon to be processed
 * \return Return a Values converted from the JSon
 */
Values jsonToValues(const Json::Value& values);

} // end of namespace
} // end of namespace

#endif
