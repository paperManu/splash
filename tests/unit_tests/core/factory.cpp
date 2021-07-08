#include <stdlib.h>
#include <string>

#include <doctest.h>

#include "./core/constants.h"
#include "./core/factory.h"
#include "./image/image.h"
#include "./image/image_ffmpeg.h"

using namespace Splash;

/*************/
TEST_CASE("Testing Factory subtype evaluation")
{
    auto factory = Factory();
    CHECK(factory.isSubtype<Image>("image_ffmpeg"));
}

/*************/
TEST_CASE("Testing Factory default values")
{
    const std::string defaultFile = "./data/splashrc.json";

    std::string previousDefaultFile;
    if (getenv(Constants::DEFAULT_FILE_ENV) != nullptr)
        previousDefaultFile = getenv(Constants::DEFAULT_FILE_ENV);

    auto retValue = setenv(Constants::DEFAULT_FILE_ENV, defaultFile.c_str(), 1);
    CHECK_EQ(retValue, 0);

    auto factory = Factory();
    auto defaults = factory.getDefaults();
    REQUIRE_NE(defaults.find("image"), defaults.end());
    auto imageDefaultsIt = defaults.find("image");
    CHECK_NE(imageDefaultsIt->second.find("flip"), imageDefaultsIt->second.end());

    if (!previousDefaultFile.empty())
        setenv(Constants::DEFAULT_FILE_ENV, previousDefaultFile.c_str(), 1);
}

/*************/
TEST_CASE("Testing Factory getters")
{
    auto factory = Factory();
    auto types = factory.getObjectTypes();
    CHECK(!types.empty());
    CHECK(!factory.getObjectsOfCategory(GraphObject::Category::MISC).empty());
    CHECK(!factory.getObjectsOfCategory(GraphObject::Category::IMAGE).empty());
    CHECK(!factory.getObjectsOfCategory(GraphObject::Category::MESH).empty());
    CHECK(!factory.getObjectsOfCategory(GraphObject::Category::TEXTURE).empty());

    for (const auto& type : types)
    {
        CHECK(!factory.getShortDescription(type).empty());
        CHECK(!factory.getDescription(type).empty());
        CHECK(factory.isCreatable(type));
    }
}
