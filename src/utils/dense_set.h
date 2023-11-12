/*
 * Copyright (C) 2019 Splash authors
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
 * @dense_set.h
 * Dense set, cache-friendly ordered set based on std::vector
 * It matches as much as possible std::set, check https://en.cppreference.com/w/cpp/container/set
 */

#ifndef SPLASH_DENSE_SET_H
#define SPLASH_DENSE_SET_H

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <vector>

namespace Splash
{

template <typename T>
class DenseSet
{
  public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

  public:
    DenseSet() = default;
    template <typename InputIt>
    DenseSet(InputIt first, InputIt last)
        : _data(first, last)
    {
        for (auto it = _data.begin(); it != _data.end(); ++it)
            _data.erase(std::remove(it + 1, _data.end(), *it), _data.end());
    }
    DenseSet(std::initializer_list<T> init)
        : _data(init)
    {
        for (auto it = _data.begin(); it != _data.end(); ++it)
            _data.erase(std::remove(it + 1, _data.end(), *it), _data.end());
    }

    DenseSet(const DenseSet<T>& other) { operator=(other); }
    DenseSet<T>& operator=(const DenseSet<T>& other)
    {
        if (&other == this)
            return *this;
        _data = other._data;
        return *this;
    }

    DenseSet(const DenseSet<T>&& other) { operator=(other); }
    DenseSet<T>& operator=(DenseSet<T>&& other) noexcept
    {
        if (&other == this)
            return *this;
        _data = other._data;
        return *this;
    }

    DenseSet<T>& operator=(std::initializer_list<T> init)
    {
        _data = init;
        for (auto it = _data.begin(); it != _data.end(); ++it)
            _data.erase(std::remove(it + 1, _data.end(), *it), _data.end());
    }

    // Comparison operators
    bool operator==(const DenseSet<T>& rhs) const
    {
        if (size() != rhs.size())
            return false;
        for (const auto& value : _data)
            if (std::find(rhs._data.cbegin(), rhs._data.cend(), value) == rhs._data.cend())
                return false;
        return true;
    }
    bool operator!=(const DenseSet<T>& rhs) const { return !operator==(rhs); }

    // Iterators
    iterator begin() noexcept { return _data.begin(); }
    const_iterator begin() const noexcept { return _data.begin(); }
    const_iterator cbegin() const noexcept { return _data.cbegin(); }

    iterator end() noexcept { return _data.end(); }
    const_iterator end() const noexcept { return _data.end(); }
    const_iterator cend() const noexcept { return _data.cend(); }

    iterator rbegin() noexcept { return _data.rbegin(); }
    const_iterator rbegin() const noexcept { return _data.rbegin(); }
    const_iterator rcbegin() const noexcept { return _data.rcbegin(); }

    iterator rend() noexcept { return _data.rend(); }
    const_iterator rend() const noexcept { return _data.rend(); }
    const_iterator rcend() const noexcept { return _data.rcend(); }

    // Capacity
    bool empty() const { return _data.empty(); }
    size_t size() const { return _data.size(); }
    bool max_size() const noexcept { return _data.max_size(); }
    void reserve(size_t size) { _data.reserve(size); }

    // Modifiers
    void clear() noexcept { _data.clear(); }

    std::pair<iterator, bool> insert(const T& value)
    {
        auto it = std::find(_data.begin(), _data.end(), value);
        if (it != _data.end())
            return {it, false};
        _data.push_back(value);
        return {_data.end() - 1, true};
    }
    std::pair<iterator, bool> insert(const T&& value)
    {
        auto it = std::find(_data.begin(), _data.end(), value);
        if (it != _data.end())
            return {it, false};
        _data.push_back(value);
        return {_data.end() - 1, true};
    }
    template <class InputIt>
    void insert(InputIt first, InputIt last)
    {
        _data.insert(_data.end(), first, last);
        for (auto it = _data.begin(); it != _data.end(); ++it)
            _data.erase(std::remove(it + 1, _data.end(), *it), _data.end());
    }
    void insert(std::initializer_list<T> init)
    {
        _data.insert(_data.end(), init);
        for (auto it = _data.begin(); it != _data.end(); ++it)
            _data.erase(std::remove(it + 1, _data.end(), *it), _data.end());
    }

    template <class... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        auto value = T(std::forward<Args>(args)...);
        if (std::find(_data.cbegin(), _data.cend(), value) != _data.cend())
            return {_data.end(), false};
        auto it = _data.emplace(_data.end(), std::move(value));
        return {it, true};
    }

    iterator erase(const_iterator pos) { return _data.erase(pos); }
    iterator erase(const_iterator first, const_iterator last) { return _data.erase(first, last); }
    size_t erase(const T& key)
    {
        auto it = std::find(_data.cbegin(), _data.cend(), key);
        if (it == _data.cend())
            return 0;
        _data.erase(it);
        return 1;
    }

    void swap(DenseSet<T>& other) noexcept { _data.swap(other._data); }

    // Lookup
    size_t count(const T& key)
    {
        if (std::find(_data.cbegin(), _data.cend(), key) == _data.cend())
            return 0;
        return 1;
    }

    iterator find(const T& key) { return std::find(_data.begin(), _data.end(), key); }

    const_iterator find(const T& key) const { return std::find(_data.cbegin(), _data.cend(), key); }

  private:
    std::vector<T> _data;
};

} // namespace Splash

#endif // SPLASH_DENSE_SET_H
