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

#include "./image/image_list.h"

#include <doctest.h>
#include <filesystem>

#include "./utils/osutils.h"

using namespace std;
using namespace Splash;

/*************/
TEST_CASE("Testing Image_List Initialization")
{
    auto root = RootObject();
    auto image = Image_List(&root);
    CHECK_EQ(image.getType(), "image_list");
}

TEST_CASE("Testing read function")
{
    auto root = RootObject();
    auto image = Image_List(&root);
    image.read(Utils::getCurrentWorkingDirectory() + "/data/");
    auto fileList = image.getFileList();
    CHECK_EQ(fileList.size(), 1);
    CHECK_EQ(std::filesystem::path(fileList[0]).filename(), "color_map.png");
}

TEST_CASE("Testing capture")
{
    auto root = RootObject();
    auto image = Image_List(&root);
    image.read(Utils::getCurrentWorkingDirectory() + "/data/");
    auto fileList = image.getFileList();
    // there is only one image in the directory
    CHECK_EQ(fileList.size(), 1);

    // When capture is called, the next image is read
    // the vector of filepaths will be empty as there
    // is only one image in the directory
    image.capture();
    fileList = image.getFileList();
    CHECK_EQ(fileList.empty(), true);
}
