/*
 * Copyright (C) 2018 Emmanuel Durand
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
 * @name_registry.h
 * Name registry, responsible for storing and creating unique names
 */

#ifndef SPLASH_NAME_REGISTRY_H
#define SPLASH_NAME_REGISTRY_H

#include <atomic>
#include <list>
#include <mutex>
#include <string>

namespace Splash
{

/*************/
class NameRegistry
{
  public:
    /**
     * Generate a name given a prefix
     * \param prefix Prefix
     * \return Return the generated name
     */
    std::string generateName(const std::string& prefix);

    /**
     * Register a name
     * \param name Name to check for
     * \return Return true if the name was not already registered
     */
    bool registerName(const std::string& name);

    /**
     * Unregister a name
     * \param name Name to unregister
     */
    void unregisterName(const std::string& name);

  private:
    static std::atomic_uint _counter;
    std::list<std::string> _registry{};
    std::mutex _registryMutex{};
};

} // namespace Splash

#endif // SPLASH_NAME_REGISTRY_H
