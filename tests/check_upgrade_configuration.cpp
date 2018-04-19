#include <doctest.h>
#include <json/json.h>

#include "./splash.h"
#include "./utils/jsonutils.h"
#include "./utils/osutils.h"

using namespace std;
using namespace Splash;

/*************/
TEST_CASE("Testing World::checkAndUpgradeConfiguration for a single Scene")
{
    auto filePath = Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.0.0.json";
    Json::Value configuration;
    Utils::loadJsonFile(filePath, configuration);
    Utils::checkAndUpgradeConfiguration(configuration);

    filePath = Utils::getCurrentWorkingDirectory() + "/data/sample_scene_0.7.15.json";
    Json::Value upgradedConfiguration;
    Utils::loadJsonFile(filePath, upgradedConfiguration);

    CHECK(configuration == upgradedConfiguration);
}
