/*
 * Copyright (C) 2018 Splash authors
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
 * @uui.h
 * Wrapper for libuuid
 */

#ifndef SPLASH_UUID_H
#define SPLASH_UUID_H

#include <string>

#include <uuid.h> // stduuid

#include "./core/serializer.h"

namespace Splash
{

class UUID
{
  public:
    /**
     * Constructor
     */
    UUID() { init(false); }

    /**
     * Constructor
     * \param generate Generate a UUID if true, otherwise the UUID will be empty
     */
    explicit UUID(bool generate) { init(generate); }

    /**
     * Comparison operators
     */
    bool operator==(const UUID& rhs) const { return _uuid == rhs._uuid; }
    bool operator!=(const UUID& rhs) const { return operator==(rhs); }

    /**
     * Initialize a new UUID
     * \param generate Generate a UUID if true, otherwise the UUID will be empty
     */
    void init(bool generate);

    /**
     * Return the UUID as a string
     * \return Returns a string representing the UUID
     */
    std::string to_string() const { return uuids::to_string(_uuid); }

  private:
    uuids::uuid _uuid;
};
} // namespace Splash

#endif // SPLASH_UUID_H
