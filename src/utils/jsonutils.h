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

#include "./config.h"
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
bool checkAndUpgradeConfiguration(Json::Value& configuration)
{
    std::string configurationVersion = configuration.isMember("version") ? configuration["version"].asString() : "";
    uint32_t versionMajor = 0;
    uint32_t versionMinor = 0;
    uint32_t versionMaintainance = 0;

    if (!configurationVersion.empty())
    {
        try
        {
            auto pos = configurationVersion.find(".");
            versionMajor = std::stoi(configurationVersion.substr(0, pos));
            if (pos != std::string::npos)
            {
                auto versionSubstr = configurationVersion.substr(pos + 1);
                pos = versionSubstr.find(".");
                versionMinor = std::stoi(versionSubstr.substr(0, pos));
                if (pos != std::string::npos)
                    versionMaintainance = std::stoi(versionSubstr.substr(pos + 1));
            }
        }
        catch (...)
        {
            Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Invalid version number in configuration file: " << configurationVersion << Log::endl;
        }
    }

    if (versionMajor == 0 && versionMinor <= 7 && versionMaintainance < 15)
    {
        Json::Value newConfig;

        newConfig["description"] = SPLASH_FILE_CONFIGURATION;
        newConfig["version"] = std::string(PACKAGE_VERSION);
        newConfig["world"] = configuration["world"];

        std::vector<std::string> sceneNames = {};
        for (const auto& scene : configuration["scenes"])
        {
            std::string sceneName = scene.isMember("name") ? scene["name"].asString() : "";
            sceneNames.push_back(sceneName);
            if (sceneName.empty())
                continue;

            for (const auto& attr : scene.getMemberNames())
            {
                if (attr == "name")
                    continue;

                newConfig["scenes"][sceneName][attr] = scene[attr];
            }
        }

        for (const auto& sceneName : sceneNames)
        {
            if (!configuration.isMember(sceneName))
                continue;

            for (const auto& attr : configuration[sceneName].getMemberNames())
            {
                if (attr == "links")
                {
                    newConfig["scenes"][sceneName]["links"] = configuration[sceneName]["links"];
                    continue;
                }
                else
                {
                    newConfig["scenes"][sceneName]["objects"][attr] = configuration[sceneName][attr];
                }
            }
        }

        configuration = newConfig;
    }

    return true;
}

/**
 * Load a Json file
 * \param filename Json file path
 * \param configuration Holds the Json tree
 * \return Return true if everything went well
 */
bool loadJsonFile(const std::string& filename, Json::Value& configuration)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    std::string contents;
    if (in)
    {
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }
    else
    {
        Log::get() << Log::WARNING << "World::" << __FUNCTION__ << " - Unable to open file " << filename << Log::endl;
        return false;
    }

    Json::Value config;
    Json::Reader reader;

    bool success = reader.parse(contents, config);
    if (!success)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - Unable to parse file " << filename << Log::endl;
        Log::get() << Log::WARNING << reader.getFormattedErrorMessages() << Log::endl;
        return false;
    }

    configuration = config;
    return true;
}

} // end of namespace
} // end of namespace

#endif
