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

#include <memory>
#include <vector>

namespace Splash
{

/*************/
template <typename T>
class ResizableArray
{
  public:
    using value_type = T;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

  public:
    /**
     * Constructor with an initial size
     * \param size Initial array size
     */
    ResizableArray(size_t size = 0) { resize(size); }

    /**
     * Constructor from two iterators
     * \param start Begin iterator
     * \param end End iterator
     */
    ResizableArray(const T* start, const T* end)
    {
        if (end <= start)
        {
            _shift = 0;
            _buffer.clear();

            return;
        }

        const auto size = static_cast<size_t>(end - start);
        _shift = 0;
        _buffer.resize(size);
        std::copy(start, end, data());
    }

    /**
     * Copy constructor
     * \param a ResizableArray to copy
     */
    ResizableArray(const ResizableArray& a)
    {
        const auto size = a.size();
        _shift = 0;
        _buffer.resize(size);
        std::copy(a.data(), a.data() + size, data());
    }

    /**
     * Move constructor
     * \param a ResizableArray to move
     */
    ResizableArray(ResizableArray&& a)
        : _shift(a._shift)
        , _buffer(std::move(a._buffer))
    {
    }

    /**
     * Constructor from a std::vector<<T>
     * \param data Vector to use, grabbed as a r-value
     */
    ResizableArray(std::vector<T>&& data)
    {
        _shift = 0;
        _buffer = std::move(data);
    }

    /**
     * Copy operator
     * \param a ResizableArray to copy from
     */
    ResizableArray& operator=(const ResizableArray& a)
    {
        if (this == &a)
            return *this;

        const auto size = a.size();
        _shift = 0;
        _buffer.resize(size);
        std::copy(a.data(), a.data() + size, data());

        return *this;
    }

    /**
     * Move operator
     * \param a ResizableArray to move from
     */
    ResizableArray& operator=(ResizableArray&& a)
    {
        if (this == &a)
            return *this;

        _shift = a._shift;
        _buffer = std::move(a._buffer);

        return *this;
    }

    /**
     * Access operator
     * \param i Index
     * \return Return value at i
     */
    T& operator[](unsigned int i) { return *(data() + i); }
    const T& operator[](unsigned int i) const { return *(data() + i); }

    /**
     * Iterators
     */
    iterator begin() { return _buffer.begin() + _shift; }
    iterator end() { return _buffer.end(); }
    const_iterator cbegin() const { return _buffer.cbegin() + _shift; }
    const_iterator cend() const { return _buffer.cend(); }

    /**
     * Get a pointer to the data
     * \return Return a pointer to the data
     */
    inline T* data() { return _buffer.data() + _shift; }
    inline const T* data() const { return _buffer.data() + _shift; }

    /**
     * Shift the data, for example to get rid of a header without copying
     * \param shift Shift in size(T)
     */
    inline void shift(size_t shift)
    {
        if (shift < _buffer.size() && _shift + shift > 0)
            _shift += shift;
    }

    /**
     * Get the size of the buffer
     * \return Return the size of the buffer
     */
    inline size_t size() const { return _buffer.size() - _shift; }

    /**
     * Resize the buffer
     * \param size New size
     */
    inline void resize(size_t size)
    {
        if (size == _buffer.size())
        {
            return;
        }
        else if (size == 0)
        {
            _shift = 0;
            _buffer.clear();
        }
        else
        {
            const auto currentSize = _buffer.size();
            std::vector<T> newBuffer(size);
            if (currentSize != 0)
                std::copy(_buffer.data(), _buffer.data() + std::min(size, currentSize), newBuffer.data());
            std::swap(_buffer, newBuffer);
            _shift = 0;
        }
    }

  private:
    size_t _shift{0};         //!< Buffer shift
    std::vector<T> _buffer{}; //!< Pointer to the buffer data
};

} // namespace Splash

#endif // SPLASH_RESIZABLE_ARRAY_H
