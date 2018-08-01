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
 * @serializer.h
 * The serializer and deserializer methods
 * This has been inspired a lot by https://github.com/motonacciu/meta-serialization
 */

#ifndef SPLASH_SERIALIZER_H
#define SPLASH_SERIALIZER_H

#include <chrono>
#include <iterator>
#include <numeric>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace Splash
{

namespace Serial
{

namespace detail
{

// Utility struct to help process tuples
template <std::size_t>
struct int_
{
};

} // namespace detail

/**
 * Helper tool to check whether the given template type is iterable
 * source: https://stackoverflow.com/questions/13830158/check-if-a-variable-is-iterable#29634934
 */
namespace detail
{

template <typename T>
auto isIterableImpl(int) -> decltype(std::cbegin(std::declval<T&>()) != std::cend(std::declval<T&>()),
    void(),
    ++std::declval<decltype(std::cbegin(std::declval<T&>()))&>(),
    void(*std::cbegin(std::declval<T&>())),
    std::true_type{});

template <typename T>
std::false_type isIterableImpl(...);

} // namespace detail

template <typename T>
using isIterable = decltype(detail::isIterableImpl<T>(0));

/**
 * Helper tool to check whether a type is a specialization of another
 * source: https://stackoverflow.com/questions/11251376/how-can-i-check-if-a-type-is-an-instantiation-of-a-given-class-template#comment14786989_11251408
 */
template <template <typename...> class Template, typename T>
struct is_specialisation_of : std::false_type
{
};
template <template <typename...> class Template, typename... Args>
struct is_specialisation_of<Template, Template<Args...>> : std::true_type
{
};

/*************/
template <class T>
size_t getSize(const T& obj);

namespace detail
{

template <class T, class Enable = void>
struct getSizeHelper;

template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value>::type>
{
    static size_t value(const T& obj) { return sizeof(obj); };
};

template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_same<T, std::string>::value>::type>
{
    static size_t value(const T& obj) { return sizeof(size_t) + obj.size() * sizeof(char); }
};

template <class T>
struct getSizeHelper<T, typename std::enable_if<is_specialisation_of<std::chrono::time_point, T>::value>::type>
{
    static size_t value(const T& /*obj*/)
    {
        return sizeof(int64_t);
    }
};

template <class T>
struct getSizeHelper<T, typename std::enable_if<isIterable<T>::value && !std::is_same<T, std::string>::value>::type>
{
    static size_t value(const T& obj)
    {
        return std::accumulate(obj.cbegin(), obj.cend(), sizeof(size_t), [](const size_t& acc, const auto& i) { return acc + getSize(i); });
    }
};

template <class T>
inline size_t getTupleSize(const T& obj, int_<0>)
{
    constexpr size_t idx = std::tuple_size<T>::value - 1;
    return getSize(std::get<idx>(obj));
}

template <class T, size_t pos>
inline size_t getTupleSize(const T& obj, int_<pos>)
{
    constexpr size_t idx = std::tuple_size<T>::value - pos - 1;
    size_t acc = getSize(std::get<idx>(obj));
    return acc + getTupleSize(obj, int_<pos - 1>());
}

template <class T>
struct getSizeHelper<T, typename std::enable_if<is_specialisation_of<std::tuple, T>::value>::type>
{
    static size_t value(const T& obj)
    {
        constexpr size_t tupleSize = std::tuple_size<T>::value - 1;
        return getTupleSize(obj, int_<tupleSize>());
    }
};

} // namespace detail

/**
 * Get the size of the serialized data
 * \param obj Object to get the serialized size from
 * \return Return the expected serialized size
 */
template <class T>
inline size_t getSize(const T& obj)
{
    return detail::getSizeHelper<T>::value(obj);
}

/*************/
namespace detail
{

template <class T, class Enable = void>
struct serializeHelper;

template <class T>
void serializer(const T& obj, std::vector<uint8_t>::iterator& it);

template <class T>
struct serializeHelper<T, typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&obj);
        std::copy(ptr, ptr + sizeof(T), it);
        it += sizeof(T);
    }
};

template <class T>
struct serializeHelper<T, typename std::enable_if<is_specialisation_of<std::chrono::time_point, T>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        using namespace std::chrono;
        int64_t value = duration_cast<milliseconds>(obj.time_since_epoch()).count();
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&value);
        std::copy(ptr, ptr + sizeof(value), it);
        it += sizeof(value);
    }
};

template <class T>
struct serializeHelper<T, typename std::enable_if<isIterable<T>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        serializer(obj.size(), it);
        for (const auto& cur : obj)
            serializer(cur, it);
    }
};

template <class T>
inline void serializeTuple(const T& obj, std::vector<uint8_t>::iterator& it, int_<0>)
{
    constexpr size_t idx = std::tuple_size<T>::value - 1;
    serializer(std::get<idx>(obj), it);
}

template <class T, size_t pos>
inline void serializeTuple(const T& obj, std::vector<uint8_t>::iterator& it, int_<pos>)
{
    constexpr size_t idx = std::tuple_size<T>::value - pos - 1;
    serializer(std::get<idx>(obj), it);
    serializeTuple(obj, it, int_<pos - 1>());
}

template <class T>
struct serializeHelper<T, typename std::enable_if<is_specialisation_of<std::tuple, T>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        constexpr size_t tupleSize = std::tuple_size<T>::value - 1;
        return serializeTuple(obj, it, int_<tupleSize>());
    }
};

template <class T>
inline void serializer(const T& obj, std::vector<uint8_t>::iterator& it)
{
    serializeHelper<T>::apply(obj, it);
}

} // namespace detail

/**
 * Serialize the given object
 * \param obj Object to serialize
 * \param buffer Buffer holding the serialized object
 */
template <class T>
inline void serialize(const T& obj, std::vector<uint8_t>& buffer)
{
    size_t offset = buffer.size();
    size_t size = getSize(obj);
    buffer.resize(offset + size);

    auto it = buffer.begin() + offset;
    detail::serializer(obj, it);
}

/*************/
namespace detail
{

template <class T, class Enable = void>
struct deserializeHelper;

template <class T>
T deserializer(std::vector<uint8_t>::const_iterator& it);

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        T obj;
        std::copy(it, it + sizeof(T), reinterpret_cast<uint8_t*>(&obj));
        it += sizeof(T);
        return obj;
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_same<T, std::string>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        auto size = deserializer<size_t>(it);
        auto obj = T(size, ' ');
        for (uint32_t i = 0; i < size; ++i)
            obj[i] = deserializer<typename T::value_type>(it);
        return obj;
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<is_specialisation_of<std::chrono::time_point, T>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        int64_t value;
        std::copy(it, it + sizeof(value), reinterpret_cast<uint8_t*>(&value));
        it += sizeof(value);
        return T(std::chrono::duration<int64_t, std::milli>(value));
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<isIterable<T>::value && !std::is_same<T, std::string>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        auto size = deserializer<size_t>(it);
        auto obj = T(size);
        auto objIt = obj.begin();
        for (uint32_t i = 0; i < size; ++i)
        {
            *objIt = deserializer<typename T::value_type>(it);
            ++objIt;
        }
        return obj;
    }
};

template <class T>
inline void deserializeTuple(T& obj, std::vector<uint8_t>::const_iterator& it, int_<0>)
{
    constexpr size_t idx = std::tuple_size<T>::value - 1;
    typedef typename std::tuple_element<idx, T>::type U;
    std::get<idx>(obj) = std::move(deserializer<U>(it));
}

template <class T, size_t pos>
inline void deserializeTuple(T& obj, std::vector<uint8_t>::const_iterator& it, int_<pos>)
{
    constexpr size_t idx = std::tuple_size<T>::value - pos - 1;
    typedef typename std::tuple_element<idx, T>::type U;
    std::get<idx>(obj) = std::move(deserializer<U>(it));
    deserializeTuple(obj, it, int_<pos - 1>());
}

template <class T>
struct deserializeHelper<T, typename std::enable_if<is_specialisation_of<std::tuple, T>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        T obj;
        constexpr size_t tupleSize = std::tuple_size<T>::value - 1;
        deserializeTuple(obj, it, int_<tupleSize>());
        return obj;
    }
};

template <class T>
T deserializer(std::vector<uint8_t>::const_iterator& it)
{
    return deserializeHelper<T>::apply(it);
}

} // namespace detail

/**
 * Deserialize the given buffer
 * \param buffer Buffer to deserialize
 * \return Return the deserialized object
 */
template <class T>
inline T deserialize(const std::vector<uint8_t>& buffer)
{
    auto it = buffer.cbegin();
    return detail::deserializer<T>(it);
}

} // namespace Serial

} // namespace Splash

#endif
