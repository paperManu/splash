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
