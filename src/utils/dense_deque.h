/*
 * Copyright (C) 2019 Emmanuel Durand
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
 * @dense_deque.h
 * Dense double-ended queue, a cache-friendly deque based on std::vector
 */

#ifndef SPLASH_DENSE_DEQUE_H
#define SPLASH_DENSE_DEQUE_H

#include <cstddef>
#include <initializer_list>
#include <vector>

namespace Splash
{

template <typename T>
class DenseDeque
{
  public:
    using value_type = T;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

  public:
    DenseDeque() {}
    explicit DenseDeque(size_t count, const T& value)
        : _data(count, value)
    {
    }
    explicit DenseDeque(size_t count)
        : _data(count)
    {
    }
    template <typename InputIt>
    DenseDeque(InputIt first, InputIt last)
        : _data(first, last)
    {
    }
    DenseDeque(std::initializer_list<T> init)
        : _data(init)
    {
    }

    DenseDeque(const DenseDeque<T>& value) { operator=(value); }
    DenseDeque<T>& operator=(const DenseDeque<T>& other)
    {
        if (&other == this)
            return *this;
        _data = other._data;
        return *this;
    }

    DenseDeque(const DenseDeque<T>&& value) { operator=(value); }
    DenseDeque<T>& operator=(DenseDeque<T>&& other) noexcept
    {
        if (&other == this)
            return *this;
        _data = other._data;
        return *this;
    }
    
    // Comparison operators
    inline bool operator==(const DenseDeque<T>& rhs) const { return _data == rhs._data; }
    inline bool operator!=(const DenseDeque<T>& rhs) const { return _data != rhs._data; }
    inline bool operator<(const DenseDeque<T>& rhs) const { return _data < rhs._data; }
    inline bool operator<=(const DenseDeque<T>& rhs) const { return _data <= rhs._data; }
    inline bool operator>(const DenseDeque<T>& rhs) const { return _data > rhs._data; }
    inline bool operator>=(const DenseDeque<T>& rhs) const { return _data >= rhs._data; }

    // Element access
    inline T& at(size_t pos) { return _data.at(pos); }
    inline const T& at(size_t pos) const { return _data.at(pos); }
    inline T& operator[](size_t pos) { return _data[pos]; }
    inline const T& operator[](size_t pos) const { return _data[pos]; }
    inline T& front() { return _data.front(); }
    inline const T& front() const { return _data.front(); }
    inline T& back() { return _data.back(); }
    inline const T& back() const { return _data.back(); }
    inline T* data() noexcept { return _data.data(); }
    inline const T* data() const noexcept { return _data.data(); }

    // Iterators
    inline iterator begin() noexcept { return _data.begin(); }
    inline const_iterator begin() const noexcept { return _data.begin(); }
    inline const_iterator cbegin() const noexcept { return _data.cbegin(); }

    inline iterator end() noexcept { return _data.end(); }
    inline const_iterator end() const noexcept { return _data.end(); }
    inline const_iterator cend() const noexcept { return _data.cend(); }

    inline iterator rbegin() noexcept { return _data.rbegin(); }
    inline const_iterator rbegin() const noexcept { return _data.rbegin(); }
    inline const_iterator rcbegin() const noexcept { return _data.rcbegin(); }

    inline iterator rend() noexcept { return _data.rend(); }
    inline const_iterator rend() const noexcept { return _data.rend(); }
    inline const_iterator rcend() const noexcept { return _data.rcend(); }

    // Capacity
    inline bool empty() const { return _data.empty(); }
    inline size_t size() const { return _data.size(); }
    inline bool max_size() const noexcept { return _data.max_size(); }
    inline void reserve(size_t count) { _data.reserve(count); }
    inline size_t capacity() const noexcept { return _data.capacity(); }
    inline void shrink_to_fit() { _data.shrink_to_fit(); }

    // Modifiers
    inline void clear() noexcept { _data.clear(); }
    inline iterator erase(const_iterator pos) { return _data.erase(pos); }
    inline iterator erase(const_iterator first, const_iterator last) { return _data.erase(first, last); }

    inline void push_back(const T& value) { _data.push_back(value); }
    inline void push_back(T&& value) { _data.push_back(std::forward<T>(value)); }
    template <class... Args>
    inline T& emplace_back(Args&&... args)
    {
        return _data.emplace_back(std::forward<Args>(args)...);
    }
    inline void pop_back() { _data.pop_back(); }

    inline void push_front(const T& value)
    {
        _data.resize(_data.size() + 1);
        for (size_t i = _data.size() - 1; i > 0; --i)
            _data[i] = std::move(_data[i - 1]);
        _data[0] = value;
    }

    template <class... Args>
    inline T& emplace_front(T& value, Args&&... args)
    {
        _data.resize(_data.size() + 1);
        for (size_t i = _data.size() - 1; i > 0; --i)
            _data[i] = std::move(_data[i - 1]);
        _data[0] = value;
        return emplace_front(value, args...);
    }
    inline T& emplace_front(T&& value)
    {
        _data.resize(_data.size() + 1);
        for (size_t i = _data.size() - 1; i > 0; --i)
            _data[i] = std::move(_data[i - 1]);
        _data[0] = value;
        return _data[0];
    }
    inline void pop_front()
    {
        for (size_t i = 0; i < _data.size() - 1; ++i)
            _data[i] = std::move(_data[i + 1]);
        _data.resize(_data.size() - 1);
    }

    inline void resize(size_t count, const T& value) { _data.resize(count, value); }
    inline void resize(size_t count) { _data.resize(count); }

    inline void swap(DenseDeque<T>& other) { _data.swap(other._data); }

  private:
    std::vector<T> _data;
};

} // namespace Splash

#endif // SPLASH_DENSE_DEQUE_H
