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
 * @serialize_mesh.h
 * Implements serialization of Mesh
 */

#ifndef SPLASH_SERIALIZE_MESH_H
#define SPLASH_SERIALIZE_MESH_H

#include <type_traits>

#include <glm/glm.hpp>

#include "./core/serializer.h"
#include "./mesh/mesh.h"

namespace Splash::Serial::detail
{

/*************/
template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_same<glm::vec2, T>::value>::type>
{
    static uint32_t value(const T& /*obj*/) { return 2 * sizeof(float); }
};

template <class T>
struct serializeHelper<T, typename std::enable_if<std::is_same<glm::vec2, T>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        const auto data = reinterpret_cast<const uint8_t*>(&obj);
        const auto size = 2 * sizeof(float);
        std::copy(data, data + size, it);
        it += size;
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_same<glm::vec2, T>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        T vector;
        auto data = reinterpret_cast<uint8_t*>(&vector);
        const auto size = 2 * sizeof(float);
        std::copy(it, it + size, data);
        it += size;
        return vector;
    }
};

/*************/
template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_same<glm::vec4, T>::value>::type>
{
    static uint32_t value(const T& /*obj*/) { return 4 * sizeof(float); }
};

template <class T>
struct serializeHelper<T, typename std::enable_if<std::is_same<glm::vec4, T>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        const auto data = reinterpret_cast<const uint8_t*>(&obj);
        const auto size = 4 * sizeof(float);
        std::copy(data, data + size, it);
        it += size;
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_same<glm::vec4, T>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        T vector;
        auto data = reinterpret_cast<uint8_t*>(&vector);
        const auto size = 4 * sizeof(float);
        std::copy(it, it + size, data);
        it += size;
        return vector;
    }
};

/*************/
template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_base_of<Mesh::MeshContainer, T>::value>::type>
{
    static uint32_t value(const T& obj)
    {
        uint32_t acc = getSize(obj.name);
        acc += getSize(obj.vertices);
        acc += getSize(obj.uvs);
        acc += getSize(obj.normals);
        acc += getSize(obj.annexe);
        return acc;
    }
};

template <class T>
struct serializeHelper<T, typename std::enable_if<std::is_base_of<Mesh::MeshContainer, T>::value>::type>
{
    static void apply(const T& obj, std::vector<uint8_t>::iterator& it)
    {
        serializer(obj.name, it);
        serializer(obj.vertices, it);
        serializer(obj.uvs, it);
        serializer(obj.normals, it);
        serializer(obj.annexe, it);
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_base_of<Mesh::MeshContainer, T>::value>::type>
{
    static T apply(std::vector<uint8_t>::const_iterator& it)
    {
        Mesh::MeshContainer meshContainer;
        meshContainer.name = deserializer<std::string>(it);
        meshContainer.vertices = deserializer<std::vector<glm::vec4>>(it);
        meshContainer.uvs = deserializer<std::vector<glm::vec2>>(it);
        meshContainer.normals = deserializer<std::vector<glm::vec4>>(it);
        meshContainer.annexe = deserializer<std::vector<glm::vec4>>(it);
        return meshContainer;
    }
};
} // namespace Splash::Serial::detail

#endif // SPLASH_SERIALIZE_MESH_H
