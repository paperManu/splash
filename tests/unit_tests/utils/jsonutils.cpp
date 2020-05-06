#include <string>

#include <doctest.h>
#include <json/json.h>

#include "./utils/jsonutils.h"
#include "./utils/osutils.h"

using namespace std;
using namespace Splash;

/*************/
TEST_CASE("Testing World::checkAndUpgradeConfiguration for a single Scene")
{
    auto filePath = Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.0.0.json";
    Json::Value configuration_0_0_0;
    Utils::loadJsonFile(filePath, configuration_0_0_0);
    Utils::checkAndUpgradeConfiguration(configuration_0_0_0);

    filePath = Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.7.15.json";
    Json::Value configuration_0_7_15;
    Utils::loadJsonFile(filePath, configuration_0_7_15);
    Utils::checkAndUpgradeConfiguration(configuration_0_7_15);

    filePath = Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.7.21.json";
    Json::Value configuration_0_7_21;
    Utils::loadJsonFile(filePath, configuration_0_7_21);
    Utils::checkAndUpgradeConfiguration(configuration_0_7_21);

    CHECK(configuration_0_0_0 == configuration_0_7_15);
    CHECK(configuration_0_0_0 == configuration_0_7_21);
}
