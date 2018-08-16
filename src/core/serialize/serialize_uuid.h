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
 * @serialize_uui.h
 * Implements serialization of uuid
 */

#ifndef SPLASH_SERIALIZE_UUID_H
#define SPLASH_SERIALIZE_UUID_H

#include "./core/serializer.h"
#include "./utils/uuid.h"

namespace Splash
{
namespace Serial
{
namespace detail
{

template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_same<T, UUID>::value>::type>
{
    static size_t value(const T& obj)
    {
        constexpr size_t size = sizeof(obj);
        return size;
    };
};

template <class T>
struct serializeHelper<T, typename std::enable_if<std::is_same<T, UUID>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        constexpr size_t size = sizeof(T);
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&obj);
        std::copy(ptr, ptr + size, it);
        it += size;
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_same<T, UUID>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        constexpr size_t size = sizeof(T);
        T obj;
        std::copy(it, it + size, reinterpret_cast<uint8_t*>(&obj));
        it += size;
        return obj;
    }
};

} // namespace detail
} // namespace Serial
} // namespace Splash

#endif // SPLASH_SERIALIZE_UUID_H
