/*
 * Copyright (C) 2021 Splash authors
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
 * @serialize_imagebuffer.h
 * Implements serialization of ImageBuffer
 */

#ifndef SPLASH_SERIALIZE_IMAGEBUFFER_H
#define SPLASH_SERIALIZE_IMAGEBUFFER_H

#include <iostream>

#include "./core/imagebuffer.h"
#include "./core/serializer.h"

namespace Splash::Serial::detail
{

template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_same<ImageBuffer, T>::value>::type>
{
    static uint32_t value(const T& obj)
    {
        const auto name = obj.getName();
        uint32_t acc = getSize(name);
        const auto spec = obj.getSpec();
        acc += getSize(spec.to_string());
        acc += spec.rawSize();
        return acc;
    }
};

template <class T>
struct serializeHelper<T, typename std::enable_if<std::is_same<ImageBuffer, T>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        const auto name = obj.getName();
        serializer(name, it);
        const auto spec = obj.getSpec();
        serializer(spec.to_string(), it);
        const auto size = static_cast<uint32_t>(spec.rawSize());
        const auto data = obj.data();
        std::copy(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size, it);
        it += size;
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_same<ImageBuffer, T>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        const auto name = deserializer<std::string>(it);
        const auto specString = deserializer<std::string>(it);
        const ImageBufferSpec spec(specString);
        T imageBuffer(spec, const_cast<uint8_t*>(&*it));
        imageBuffer.setName(name);
        return imageBuffer;
    }
};
} // namespace Splash::Serial::detail

#endif // SPLASH_SERIALIZE_IMAGEBUFFER_H
