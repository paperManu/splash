/*
 * Copyright (C) 2021 Emmanuel Durand
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
 * @subprocess.h
 * Subprocess utility class to handle a spawned process
 */

#include <optional>
#include <string>

#include <spawn.h>

namespace Splash
{
namespace Utils
{

class Subprocess
{
  public:
    /**
     * Constructor
     * \param command Program to run
     * \param args Arguments to pass to the program
     * \param env Environment variables
     */
    Subprocess(const std::string& command, const std::string& args = "", const std::string& env = "");

    /**
     * Destructor
     */
    ~Subprocess();

    /**
     * Get the process PID, if it exists
     * \return Return the PID in a std::optional
     */
    std::optional<int> getPID() const;

    /**
     * Get the running state of the subprocess
     * \return Return true if the subprocess is running, false otherwise
     */
    bool isRunning();

  private:
    bool _success{false};

    int32_t _pid{-1};
    posix_spawnattr_t _spawnAttr;
};

} // namespace Utils
} // namespace Splash
