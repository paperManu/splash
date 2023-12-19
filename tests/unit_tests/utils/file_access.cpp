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

#include "./config.h"
#include "./utils/osutils.h"

using namespace Splash;
using namespace Utils;
namespace fs = std::filesystem;

/*************/
TEST_CASE("Testing Splash::Utils::isDir")
{
    fs::path dir_name = "splash_" + std::to_string(getpid());
    fs::create_directory(dir_name);

    CHECK(isDir(dir_name.string()) == true);
    CHECK(isDir("some/made/up/dir") == false);

    // Some special cases
    CHECK(isDir(".") == true);
    CHECK(isDir("splash_" + std::to_string(getpid()) + "/..") == true);

    fs::remove_all(dir_name);
}

/*************/
TEST_CASE("Testing Splash::Utils::cleanPath")
{
    CHECK(fs::path(cleanPath("/")) == fs::path("/"));
    CHECK(fs::path(cleanPath(".")) == fs::path("."));
    CHECK(fs::path(cleanPath("..")) == fs::path(".."));
    CHECK(fs::path(cleanPath("/some/path/")) == fs::path("/some/path/"));
    CHECK(fs::path(cleanPath("/some/path/.")) == fs::path("/some/path/"));
    CHECK(fs::path(cleanPath("/some/path/..")) == fs::path("/some/"));
    CHECK(fs::path(cleanPath("/some//path//")) == fs::path("/some/path/"));
    CHECK(fs::path(cleanPath("./some/path/")) == fs::path("some/path/"));
    CHECK(fs::path(cleanPath("../some/path/")) == fs::path("../some/path/"));
    CHECK(fs::path(cleanPath("/some/file.abc")) == fs::path("/some/file.abc"));
}

/*************/
TEST_CASE("Testing Splash::Utils::getPathFromFilePath")
{
    auto filename = std::string("/some/absolute/path.ext");
#if HAVE_LINUX
    CHECK(fs::path(getPathFromFilePath(filename)) == fs::path("/some/absolute/"));
#elif HAVE_WINDOWS
    CHECK(fs::path(getPathFromFilePath(filename)) == fs::path("C:\\some\\absolute\\"));
#endif

    filename = std::string("this/one/is/relative.ext");
    auto config_path = std::string("/some/config/path");
    CHECK(fs::path(getPathFromFilePath(filename)) == fs::path(getCurrentWorkingDirectory() + "/this/one/is/"));
    CHECK(fs::path(getPathFromFilePath(filename, config_path)) == fs::path(config_path + "/this/one/is/"));
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
    auto filename = fs::path("filename.ext");
    auto filepath = fs::path("this/is/just/some/path" / filename);
    auto name = getFilenameFromFilePath(filepath.string());
    CHECK(name == filename);

    // Checking when the path is just a file
    name = getFilenameFromFilePath(filename.string());
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
    auto filepath = fs::path("just_a_file.ext");
    auto config_path = fs::path("/a_config/path");
    CHECK(fs::path(getFullPathFromFilePath(filepath.string(), config_path.string())) == fs::path((config_path / filepath).string()));
}

/*************/
TEST_CASE("Testing Splash::Utils::listDirContent")
{
    fs::path dir_name = "/tmp/splash_" + std::to_string(getpid());
    fs::create_directories(dir_name / "a/b");
    std::ofstream(dir_name / "file1.txt");
    std::ofstream(dir_name / "file2.txt");

    std::vector<std::string> files = listDirContent(dir_name.string());

    // In the directory we have 3 elements:
    // a
    // file1.txt
    // file2.txt
    CHECK(files.size() == 3);

    // Special case: path is actually a file
    files = listDirContent((dir_name / "file1.txt").string());
    CHECK(files.size() == 3);

    fs::remove_all(dir_name);
}
