/*
 * Copyright (C) 2025 Splash authors
 *
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

/*
 * @files.h
 * Some file utilities
 */

#ifndef SPLASH_FILES_H
#define SPLASH_FILES_H

#include <filesystem>
#include <fstream>

namespace Splash
{
namespace Utils
{

inline std::string getTextFileContent(const std::filesystem::path& path)
{
    if (path.empty())
        return {};

    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();

        return contents;
    }

    return {};
}

} // namespace Utils
} // namespace Splash

#endif
