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
 * @uui.h
 * Wrapper for libuuid
 */

#ifndef SPLASH_UUID_H
#define SPLASH_UUID_H

#include <cstring>

#include <uuid/uuid.h>

#include "./core/serializer.h"

namespace Splash
{

class UUID
{
  public:
    /**
     * Constructor
     */
    UUID() { uuid_generate(_uuid); }
    explicit UUID(bool generate)
    {
        if (generate)
            uuid_generate(_uuid);
        else
            memset(_uuid, 0, 16);
    }

    bool operator==(const UUID& rhs) const { return uuid_compare(_uuid, rhs._uuid) == 0; }
    bool operator!=(const UUID& rhs) const { return operator==(rhs); }

  private:
    uuid_t _uuid;
};
} // namespace Splash

#endif // SPLASH_UUID_H
