#include "./utils/jsonutils.h"

#include "./core/coretypes.h"

namespace Splash
{
namespace Utils
{

/*************/
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
            Log::get() << Log::WARNING << "Utils::" << __FUNCTION__ << " - Invalid version number in configuration file: " << configurationVersion << Log::endl;
        }
    }

    if (versionMajor == 0 && versionMinor <= 7 && versionMaintainance < 15)
    {
        Json::Value newConfig;

        newConfig["description"] = SPLASH_FILE_CONFIGURATION;
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

    if (versionMajor == 0 && versionMinor <= 7 && versionMaintainance < 21)
    {
        Json::Value newConfig = configuration;
        for (auto& scene : newConfig["scenes"])
        {
            if (!scene.isMember("objects"))
                continue;

            for (auto& object : scene["objects"])
            {
                if (object["type"] != "window")
                    continue;
                
                object["layout"] = Json::Value(Json::arrayValue);
                object["layout"].append(0);
                object["layout"].append(1);
                object["layout"].append(2);
                object["layout"].append(3);
            }
        }

        configuration = newConfig;
    }

    configuration["version"] = std::string(PACKAGE_VERSION);

    return true;
}

/*************/
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
        Log::get() << Log::WARNING << "Utils::" << __FUNCTION__ << " - Unable to open file " << filename << Log::endl;
        return false;
    }

    Json::Value config;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> const reader(builder.newCharReader());
    std::string errs;

    bool success = reader->parse(contents.c_str(), contents.c_str() + contents.size(), &config, &errs);
    if (!success)
    {
        Log::get() << Log::WARNING << __FUNCTION__ << " - Unable to parse file " << filename << Log::endl;
        Log::get() << Log::WARNING << errs << Log::endl;
        return false;
    }

    configuration = config;
    return true;
}

} // end of namespace
} // end of namespace
