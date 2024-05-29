/*
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

#include "./image/image.h"

#include <filesystem>

#include <doctest.h>

#include "./utils/osutils.h"

using namespace Splash;
namespace fs = std::filesystem;

/*************/
TEST_CASE("Testing Image initialization")
{
    auto root = RootObject();
    auto image = Image(&root);
    CHECK_EQ(image.getType(), "image");
}

/**************/
TEST_CASE("Testing Image timestamp")
{
    auto root = RootObject();
    auto image = Image(&root);
    auto previousTimestamp = image.getTimestamp();

    SUBCASE("Verifying timestamp after zero-ed")
    {
        image.zero();
        image.update();
        CHECK_GT(image.getTimestamp(), previousTimestamp);
        previousTimestamp = image.getTimestamp();
    }

    SUBCASE("Verifying timestamp after a no-op update")
    {
        image.update();
        CHECK_EQ(image.getTimestamp(), previousTimestamp);
        previousTimestamp = image.getTimestamp();
    }

    SUBCASE("Verifying timestamp after reading an image")
    {
        image.read(Utils::getCurrentWorkingDirectory() + "/data/color_map.png");
        image.update();
        CHECK_GT(image.getTimestamp(), previousTimestamp);
    }
}

/**************/
TEST_CASE("Testing Image read/write")
{
    auto root = RootObject();
    auto image = Image(&root);

    CHECK(image.read(Utils::getCurrentWorkingDirectory() + "/data/color_map.png"));
    const auto imageSize = image.get().getSize();

    const std::string directory = Utils::getCurrentWorkingDirectory() + "/tmp";
    fs::create_directory(directory);

    SUBCASE("Verifying Image::write")
    {
        CHECK(image.write(directory + "/test_image.png"));
        CHECK(image.write(directory + "/test_image.bmp"));
        CHECK(image.write(directory + "/test_image.tga"));
    }

    SUBCASE("Verifying Image::read")
    {
        auto otherImage = Image(&root);
        CHECK(otherImage.read(directory + "/test_image.png"));
        CHECK_EQ(otherImage.get().getSize(), imageSize);

        CHECK(otherImage.read(directory + "/test_image.bmp"));
        CHECK_EQ(otherImage.get().getSize(), imageSize);

        CHECK(otherImage.read(directory + "/test_image.tga"));
        CHECK_EQ(otherImage.get().getSize(), imageSize);
    }

    // Check 16bits images
    CHECK(image.read(Utils::getCurrentWorkingDirectory() + "/data/depthmap.png"));
}
