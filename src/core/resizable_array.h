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
 * @resizable_array.h
 * Resizable array, used to hold big buffers (like raw images)
 */

#ifndef SPLASH_RESIZABLE_ARRAY_H
#define SPLASH_RESIZABLE_ARRAY_H

#include <cstring>
#include <memory>

namespace Splash
{

/*************/
template <typename T>
class ResizableArray
{
  public:
    /**
     * \brief Constructor with an initial size
     * \param size Initial array size
     */
    ResizableArray(size_t size = 0) { resize(size); }

    /**
     * \brief Constructor from two iterators
     * \param start Begin iterator
     * \param end End iterator
     */
    ResizableArray(T* start, T* end)
    {
        if (end <= start)
        {
            _size = 0;
            _shift = 0;
            _buffer.reset();

            return;
        }

        _size = static_cast<size_t>(end - start);
        _shift = 0;
        _buffer = std::unique_ptr<T[]>(new T[_size]);
        memcpy(_buffer.get(), start, _size * sizeof(T));
    }

    /**
     * \brief Copy constructor
     * \param a ResizableArray to copy
     */
    ResizableArray(const ResizableArray& a)
    {
        _size = a.size();
        _shift = 0;
        _buffer = std::unique_ptr<T[]>(new T[_size]);
        memcpy(data(), a.data(), _size);
    }

    /**
     * \brief Move constructor
     * \param a ResizableArray to move
     */
    ResizableArray(ResizableArray&& a)
        : _size(a._size)
        , _shift(a._shift)
        , _buffer(std::move(a._buffer))
    {
    }

    /**
     * \brief Copy operator
     * \param a ResizableArray to copy from
     */
    ResizableArray& operator=(const ResizableArray& a)
    {
        if (this == &a)
            return *this;

        _size = a.size();
        _shift = 0;
        _buffer = std::unique_ptr<T[]>(new T[_size]);
        memcpy(data(), a.data(), _size);

        return *this;
    }

    /**
     * \brief Move operator
     * \param a ResizableArray to move from
     */
    ResizableArray& operator=(ResizableArray&& a)
    {
        if (this == &a)
            return *this;

        _size = a._size;
        _shift = a._shift;
        _buffer = std::move(a._buffer);

        return *this;
    }

    /**
     * \brief Access operator
     * \param i Index
     * \return Return value at i
     */
    T& operator[](unsigned int i) const { return *(data() + i); }

    /**
     * \brief Get a pointer to the data
     * \return Return a pointer to the data
     */
    inline T* data() const { return _buffer.get() + _shift; }

    /**
     * \brief Shift the data, for example to get rid of a header without copying
     * \param shift Shift in size(T)
     */
    inline void shift(size_t shift)
    {
        if (shift < _size && _shift + shift > 0)
        {
            _shift += shift;
            _size -= shift;
        }
    }

    /**
     * \brief Get the size of the buffer
     * \return Return the size of the buffer
     */
    inline size_t size() const { return _size; }

    /**
     * \brief Resize the buffer
     * \param size New size
     */
    inline void resize(size_t size)
    {
        if (size == 0)
        {
            _size = 0;
            _shift = 0;
            _buffer.reset(nullptr);
        }

        auto newBuffer = std::unique_ptr<T[]>(new T[size]);
        if (size >= _size)
            memcpy(newBuffer.get(), _buffer.get(), _size);
        else
            memcpy(newBuffer.get(), _buffer.get(), size);

        std::swap(_buffer, newBuffer);
        _size = size;
        _shift = 0;
    }

  private:
    size_t _size{0};                       //!< Buffer size
    size_t _shift{0};                      //!< Buffer shift
    std::unique_ptr<T[]> _buffer{nullptr}; //!< Pointer to the buffer data
};

} // namespace Splash

#endif // SPLASH_RESIZABLE_ARRAY_H
