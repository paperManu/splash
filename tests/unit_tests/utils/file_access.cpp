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

#include <doctest.h>
#include <filesystem>
#include <fstream>
#include <unistd.h> // for getpid()

#include "./utils/osutils.h"

using namespace Splash;
using namespace Utils;

/*************/
TEST_CASE("Testing Splash::Utils::isDir")
{
    std::filesystem::path dir_name = "/tmp/splash_" + std::to_string(getpid());
    std::filesystem::create_directory(dir_name);

    CHECK(isDir(dir_name) == true);
    CHECK(isDir("some/made/up/dir") == false);

    // Some special cases
    CHECK(isDir(".") == true);
    CHECK(isDir("/tmp/splash_" + std::to_string(getpid()) + "/..") == true);

    std::filesystem::remove_all(dir_name);
}

/*************/
TEST_CASE("Testing Splash::Utils::cleanPath")
{
    CHECK(cleanPath("/") == "/");
    CHECK(cleanPath(".") == ".");
    CHECK(cleanPath("..") == "..");
    CHECK(cleanPath("/some/path/") == "/some/path/");
    CHECK(cleanPath("/some/path/.") == "/some/path/");
    CHECK(cleanPath("/some/path/..") == "/some/");
    CHECK(cleanPath("/some//path//") == "/some/path/");
    CHECK(cleanPath("./some/path/") == "some/path/");
    CHECK(cleanPath("../some/path/") == "../some/path/");
    CHECK(cleanPath("/some/file.abc") == "/some/file.abc");
}

/*************/
TEST_CASE("Testing Splash::Utils::getPathFromFilePath")
{
    auto filename = std::string("/some/absolute/path.ext");
    CHECK(getPathFromFilePath(filename) == "/some/absolute/");

    filename = std::string("this/one/is/relative.ext");
    auto config_path = std::string("/some/config/path");
    CHECK(getPathFromFilePath(filename) == getCurrentWorkingDirectory() + "/this/one/is/");
    CHECK(getPathFromFilePath(filename, config_path) == config_path + "/this/one/is/");
}

/*************/
TEST_CASE("Testing Splash::Utils::getCurrentWorkingDirectory")
{
    char workingPathChar[256];
    auto workingPath = std::string(getcwd(workingPathChar, 255));
    CHECK(getCurrentWorkingDirectory() == workingPath);
}

/*************/
TEST_CASE("Testing Splash::Utils::getFilenameFromFilePath")
{
    auto filename = std::filesystem::path("filename.ext");
    auto filepath = std::filesystem::path("this/is/just/some/path" / filename);
    auto name = getFilenameFromFilePath(filepath);
    CHECK(name == filename);

    // Checking when the path is just a file
    name = getFilenameFromFilePath(filename);
    CHECK(name == filename);

    // Some special cases
    name = getFilenameFromFilePath(".");
    CHECK(name == ".");
    name = getFilenameFromFilePath("..");
    CHECK(name == "..");
    name = getFilenameFromFilePath("/");
    CHECK(name == "");
}

/*************/
TEST_CASE("Testing Splash::Utils::getFullPathFromFilePath")
{
    auto filepath = std::filesystem::path("just_a_file.ext");
    auto config_path = std::filesystem::path("/a_config/path");
    CHECK(getFullPathFromFilePath(filepath, config_path) == config_path / filepath);
}

/*************/
TEST_CASE("Testing Splash::Utils::listDirContent")
{
    std::filesystem::path dir_name = "/tmp/splash_" + std::to_string(getpid());
    std::filesystem::create_directories(dir_name / "a/b");
    std::ofstream(dir_name / "file1.txt");
    std::ofstream(dir_name / "file2.txt");

    std::vector<std::string> files = listDirContent(dir_name);

    // In the directory we have 3 elements:
    // a
    // file1.txt
    // file2.txt
    CHECK(files.size() == 3);

    // Special case: path is actually a file
    files = listDirContent(dir_name / "file1.txt");
    CHECK(files.size() == 3);

    std::filesystem::remove_all(dir_name);
}
