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
 * @serialize_value.h
 * Implements serialization of Value
 */

#ifndef SPLASH_SERIALIZE_VALUE_H
#define SPLASH_SERIALIZE_VALUE_H

#include "./core/serializer.h"
#include "./core/value.h"

namespace Splash
{
namespace Serial
{
namespace detail
{

// Specialisation of serialization for Splash::ResizableArray<uint8_t>, here known as Value::Buffer
template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_same<T, Value::Buffer>::value>::type>
{
    static size_t value(const T& obj) { return sizeof(size_t) + obj.size(); }
};

template <class T>
struct serializeHelper<T, typename std::enable_if<std::is_same<T, Value::Buffer>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        auto size = obj.size();
        serializer(size, it);
        auto data = reinterpret_cast<uint8_t*>(obj.data());
        std::copy(data, data + size, it);
        it += size;
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_same<T, Value::Buffer>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        auto size = deserializer<size_t>(it);
        T obj(size);
        auto data = reinterpret_cast<uint8_t*>(obj.data());
        std::copy(it, it + size, data);
        it += size;

        return obj;
    }
};

// Specialisation of serialization for Splash::Value
template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_same<T, Value>::value>::type>
{
    static size_t value(const Value& obj)
    {
        size_t acc = sizeof(Value::Type);
        auto objType = obj.getType();

        if (objType == Value::Type::string)
            return acc + getSize(obj.as<std::string>());
        else if (objType == Value::Type::values)
            return acc + getSize(obj.as<Values>());
        else if (objType == Value::Type::buffer)
            return acc + getSize(obj.as<Value::Buffer>());
        else
            return acc + obj.size();
    }
};

template <class T>
struct serializeHelper<T, typename std::enable_if<std::is_same<T, Value>::value>::type>
{
    static void apply(const Value& obj, std::vector<uint8_t>::iterator& it)
    {
        auto objType = obj.getType();
        serializer(static_cast<typename std::underlying_type<Value::Type>::type>(objType), it);

        if (objType == Value::Type::string)
        {
            serializer(obj.as<std::string>(), it);
        }
        else if (objType == Value::Type::values)
        {
            serializer(obj.as<Values>(), it);
        }
        else if (objType == Value::Type::buffer)
        {
            serializer(obj.as<Value::Buffer>(), it);
        }
        else
        {
            auto ptr = reinterpret_cast<uint8_t*>(obj.data());
            std::copy(ptr, ptr + obj.size(), it);
            it += obj.size();
        }
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_same<T, Value>::value>::type>
{
    static Value apply(std::vector<uint8_t>::const_iterator& it)
    {
        T obj;
        Value::Type type;
        std::copy(it, it + sizeof(Value::Type), reinterpret_cast<char*>(&type));
        it += sizeof(Value::Type);

        switch (type)
        {
        default:
            assert(false);
            break;
        case Value::Type::integer:
            obj = Value(deserializer<int64_t>(it));
            break;
        case Value::Type::real:
            obj = Value(deserializer<double>(it));
            break;
        case Value::Type::string:
            obj = Value(deserializer<std::string>(it));
            break;
        case Value::Type::values:
            obj = Value(deserializer<Values>(it));
            break;
        case Value::Type::buffer:
            obj = Value(deserializer<Value::Buffer>(it));
            break;
        }

        return obj;
    }
};

} // namespace detail
} // namespace Serial
} // namespace Splash

#endif // SPLASH_SERIALIZE_VALUE_H
