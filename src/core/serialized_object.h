/*
 * Copyright (C) 2015 Emmanuel Durand
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
 * @serialized_object.h
 * Object holding the serialized form of a BufferObject
 */

#ifndef SPLASH_SERIALIZED_OBJECT_H
#define SPLASH_SERIALIZED_OBJECT_H

#include "./core/resizable_array.h"

namespace Splash
{

/*************/
struct SerializedObject
{
    /**
     * \brief Constructor
     */
    SerializedObject() = default;

    /**
     * \brief Constructor with an initial size
     * \param size Initial array size
     */
    explicit SerializedObject(int size) { _data.resize(size); }

    /**
     * \brief Constructor from two iterators
     * \param start Begin iterator
     * \param end End iterator
     */
    SerializedObject(uint8_t* start, uint8_t* end)
        : _data(start, end)
    {
    }

    /**
     * \brief Get the pointer to the data
     * \return Return a pointer to the data
     */
    inline uint8_t* data() { return _data.data(); }

    /**
     * \brief Get ownership over the inner buffer. Use with caution, as it invalidates the SerializedObject
     * \return Return the inner buffer as a rvalue
     */
    inline ResizableArray<uint8_t>&& grabData() { return std::move(_data); }

    /**
     * \brief Get the size of the data
     * \return Return the size
     */
    inline std::size_t size() { return _data.size(); }

    /**
     * \brief Modify the size of the data
     * \param s New size
     */
    inline void resize(size_t s) { _data.resize(s); }

    //! Inner buffer
    ResizableArray<uint8_t> _data{};
};

} // end of namespace

#endif // SPLASH_SERIALIZED_OBJECT_H
