#include <string>
#include <vector>

#include <doctest.h>
#include <json/json.h>

#include "./utils/jsonutils.h"
#include "./utils/osutils.h"

using namespace Splash;

/*************/
TEST_CASE("Testing World::checkAndUpgradeConfiguration for a single Scene")
{
    const std::vector<std::string> files = {Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.0.0.json",
        Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.7.15.json",
        Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.7.21.json",
        Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.8.17.json",
        Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.10.0.json",
        Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.11.0.json"};

    std::vector<Json::Value> configs;
    for (const auto& file : files)
    {
        Json::Value configuration;
        Utils::loadJsonFile(file, configuration);
        Utils::checkAndUpgradeConfiguration(configuration);
        configs.push_back(configuration);
    }

    assert(!configs.empty());
    const auto& firstConfig = configs[0];
    for (uint32_t i = 1; i < configs.size(); ++i)
        CHECK(firstConfig == configs[i]);
}

/*************/
TEST_CASE("Testing World::checkAndUpgradeConfiguration for a scene exported from the Blender addon")
{
    const std::string file = Utils::getCurrentWorkingDirectory() + "/data/sample_blender_scene.json";
    Json::Value configuration;
    Utils::loadJsonFile(file, configuration);
    CHECK(Utils::checkAndUpgradeConfiguration(configuration));
}
