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
 * @dense_map.h
 * Dense map, a cache friendly unordered map based on std::vector
 * It matches as much as possible std::map, check https://en.cppreference.com/w/cpp/container/map
 *
 * Known issues:
 * - modifying the DenseMap while iterating over it with a for range may invalide the std::pair references
 */

#ifndef SPLASH_DENSE_MAP_H
#define SPLASH_DENSE_MAP_H

#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <vector>

#include "./utils/dense_set.h"

namespace Splash
{

template <class Key, class T>
class DenseMap
{
  public:
    class const_iterator;
#if __cplusplus < 201703L
    class iterator : public std::iterator<std::input_iterator_tag, std::pair<const Key&, T&>, int, std::pair<const Key&, T&>*, std::pair<const Key&, T&>>
    {
#else // C++17
    class iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<const Key&, T&>;
        using difference_type = int;
        using pointer = std::pair<const Key&, T&>*;
        using reference = std::pair<const Key&, T&>;
#endif

        friend const_iterator;

      public:
        explicit iterator(const size_t& index, DenseMap<Key, T>& map)
            : _index(index)
            , _map(&map)
        {
        }

        iterator(const iterator& rhs)
        {
            operator=(rhs);
        }
        iterator& operator=(const iterator& rhs)
        {
            if (&rhs == this)
                return *this;

            _index = rhs._index;
            _map = rhs._map;
            return *this;
        }

        iterator& operator++()
        {
            _index = std::min(_index + 1, _map->_keys.size());
            return *this;
        }

        iterator operator++(int)
        {
            iterator retval = *this;
            ++(*this);
            return retval;
        }

        iterator& operator--()
        {
            if (_index != 0)
                --_index;
            return *this;
        }
        iterator operator--(int)
        {
            iterator retval = *this;
            --(*this);
            return retval;
        }

        bool operator==(const iterator& other) const
        {
            return _map == other._map && _index == other._index;
        }
        bool operator!=(const iterator& other) const
        {
            return !operator==(other);
        }
        const std::pair<const Key&, T&> operator*() const
        {
            return std::pair<const Key&, T&>(*(_map->_keys.begin() + _index), _map->_values[_index]);
        }
        std::pair<const Key&, T&>* operator->() const
        {
            _entry.reset(new std::pair<const Key&, T&>(*(_map->_keys.begin() + _index), _map->_values[_index]));
            return _entry.get();
        }

      protected:
        size_t _index;
        DenseMap<Key, T>* _map;
        mutable std::unique_ptr<std::pair<const Key&, T&>> _entry{nullptr};
    };

#if __cplusplus < 201703L
    class const_iterator : public std::iterator<std::input_iterator_tag, std::pair<const Key&, const T&>, int, std::pair<const Key&, const T&>*, std::pair<const Key&, const T&>>
    {
#else // C++17
    class const_iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<const Key&, const T&>;
        using difference_type = int;
        using pointer = std::pair<const Key&, const T&>*;
        using reference = std::pair<const Key&, const T&>;
#endif

      public:
        explicit const_iterator(const size_t& index, const DenseMap<Key, T>& map)
            : _index(index)
            , _map(&map)
        {
        }

        const_iterator(const const_iterator& rhs)
        {
            operator=(rhs);
        }
        const_iterator& operator=(const const_iterator& rhs)
        {
            if (&rhs == this)
                return *this;

            _index = rhs._index;
            _map = rhs._map;
            return *this;
        }

        const_iterator(const iterator& rhs)
        {
            operator=(rhs);
        }
        const_iterator& operator=(const iterator& rhs)
        {
            _index = rhs._index;
            _map = rhs._map;
            return *this;
        }

        const_iterator& operator++()
        {
            _index = std::min(_index + 1, _map->_keys.size());
            return *this;
        }

        const_iterator operator++(int)
        {
            iterator retval = *this;
            ++(*this);
            return retval;
        }

        iterator& operator--()
        {
            if (_index != 0)
                --_index;
            return *this;
        }
        iterator operator--(int)
        {
            iterator retval = *this;
            --(*this);
            return retval;
        }

        bool operator==(const const_iterator& other) const
        {
            return _map == other._map && _index == other._index;
        }
        bool operator!=(const const_iterator& other) const
        {
            return !operator==(other);
        }
        const std::pair<const Key&, const T&> operator*() const
        {
            return std::pair<const Key&, const T&>(*(_map->_keys.begin() + _index), _map->_values[_index]);
        }
        std::pair<const Key&, const T&>* operator->() const
        {
            _entry.reset(new std::pair<const Key&, const T&>(*(_map->_keys.begin() + _index), _map->_values[_index]));
            return _entry.get();
        }

      protected:
        size_t _index;
        const DenseMap<Key, T>* _map;
        mutable std::unique_ptr<std::pair<const Key&, const T&>> _entry{nullptr};
    };

    class const_reverse_iterator;
#if __cplusplus < 201703L
    class reverse_iterator : public std::iterator<std::input_iterator_tag, std::pair<Key&, T&>, int, std::pair<Key, T&>*, std::pair<Key, T&>>
    {
#else // C++17
    class reverse_iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<Key&, T&>;
        using difference_type = int;
        using pointer = std::pair<Key&, T&>*;
        using reference = std::pair<Key&, T&>;
#endif

        friend const_reverse_iterator;

      public:
        explicit reverse_iterator(const size_t& index, DenseMap<Key, T>& map)
            : _index(index)
            , _map(&map)
        {
        }

        reverse_iterator(const reverse_iterator& rhs)
        {
            operator=(rhs);
        }
        reverse_iterator& operator=(const reverse_iterator& rhs)
        {
            if (&rhs == this)
                return *this;

            _index = rhs._index;
            _map = rhs._map;
            return *this;
        }

        reverse_iterator& operator++()
        {
            if (_index != 0)
                --_index;
            return *this;
        }
        reverse_iterator operator++(int)
        {
            iterator retval = *this;
            ++(*this);
            return retval;
        }

        iterator& operator--()
        {
            _index = std::min(_index + 1, _map->_keys.size());
            return *this;
        }
        iterator operator--(int)
        {
            iterator retval = *this;
            --(*this);
            return retval;
        }

        bool operator==(const reverse_iterator& other) const
        {
            return _map == other._map && _index == other._index;
        }
        bool operator!=(const reverse_iterator& other) const
        {
            return !operator==(other);
        }
        const std::pair<const Key&, T&> operator*() const
        {
            return std::pair<const Key&, T&>(*(_map->_keys.rbegin() + _index), _map->_values[_map->_values.size() - 1 - _index]);
        }
        std::pair<const Key&, T&>* operator->() const
        {
            _entry.reset(new std::pair<const Key&, T&>(*(_map->_keys.begin() + _index), _map->_values[_index]));
            return _entry.get();
        }

      protected:
        size_t _index;
        DenseMap<Key, T>* _map;
        mutable std::unique_ptr<std::pair<const Key&, const T&>> _entry{nullptr};
    };

#if __cplusplus < 201703L
    class const_reverse_iterator
        : public std::iterator<std::input_iterator_tag, std::pair<const Key&, const T&>, int, std::pair<const Key, const T&>*, std::pair<const Key, const T&>>
    {
#else // C++17
    class const_reverse_iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<const Key&, const T&>;
        using difference_type = int;
        using pointer = std::pair<const Key&, const T&>*;
        using reference = std::pair<const Key&, const T&>;
#endif

      public:
        explicit const_reverse_iterator(const size_t& index, const DenseMap<Key, T>& map)
            : _index(index)
            , _map(&map)
        {
        }

        const_reverse_iterator(const const_reverse_iterator& rhs)
        {
            operator=(rhs);
        }
        const_reverse_iterator& operator=(const const_reverse_iterator& rhs)
        {
            if (&rhs == this)
                return *this;
            _index = rhs._index;
            _map = rhs._map;
            return *this;
        }

        const_reverse_iterator(const reverse_iterator& rhs)
        {
            operator=(rhs);
        }
        const_reverse_iterator& operator=(const reverse_iterator& rhs)
        {
            _index = rhs._index;
            _map = rhs._map;
            return *this;
        }

        const_reverse_iterator& operator++()
        {
            if (_index != 0)
                --_index;
            return *this;
        }
        const_reverse_iterator operator++(int)
        {
            const_reverse_iterator retval = *this;
            ++(*this);
            return retval;
        }

        iterator& operator--()
        {
            _index = std::min(_index + 1, _map->_keys.size());
            return *this;
        }
        iterator operator--(int)
        {
            iterator retval = *this;
            --(*this);
            return retval;
        }

        bool operator==(const const_reverse_iterator& other) const
        {
            return _map == other._map && _index == other._index;
        }
        bool operator!=(const const_reverse_iterator& other) const
        {
            return !operator==(other);
        }
        const std::pair<const Key&, const T&> operator*() const
        {
            return std::pair<const Key&, const T&>(*(_map->_keys.rbegin() + _index), _map->_values[_map->_values.size() - 1 - _index]);
        }
        std::pair<const Key&, const T&>* operator->() const
        {
            _entry.reset(new std::pair<const Key&, const T&>(*(_map->_keys.begin() + _index), _map->_values[_index]));
            return _entry.get();
        }

      protected:
        size_t _index;
        const DenseMap<Key, T>* _map;
        mutable std::unique_ptr<std::pair<const Key&, const T&>> _entry{nullptr};
    };

  public:
    friend iterator;

    DenseMap() = default;
    template <typename InputIt>
    DenseMap(InputIt first, InputIt last)
    {
        for (auto& it = first; it != last; ++it)
            insert(*it);
    }

    DenseMap(const DenseMap<Key, T>& other)
    {
        operator=(other);
    }
    DenseMap<Key, T>& operator=(const DenseMap<Key, T>& other)
    {
        _keys = other._keys;
        _values = other._values;
        return *this;
    }

    DenseMap(const DenseMap<Key, T>&& other)
    {
        operator=(other);
    }
    DenseMap<Key, T>& operator=(DenseMap<Key, T>&& other) noexcept
    {
        _keys = other._keys;
        _values = other._values;
        return *this;
    }

    DenseMap(std::initializer_list<std::pair<Key, T>> init)
    {
        operator=(init);
    }
    DenseMap<Key, T>& operator=(std::initializer_list<std::pair<Key, T>> init)
    {
        _keys.clear();
        _values.clear();
        for (auto p = init.begin(); p != init.end(); ++p)
        {
            if (_keys.find(p->first) == _keys.end())
            {
                auto keyIt = _keys.insert(p->first);
                _values.push_back(p->second);
            }
        }
        return *this;
    }

    // Element access
    T& at(const Key& key)
    {
        typename DenseSet<Key>::const_iterator it = _keys.find(key);
        if (it == _keys.cend())
            throw std::out_of_range("Container does not have a element with key " + std::to_string(key));
        return _values[std::distance(_keys.cbegin(), it)];
    }

    const T& at(const Key& key) const
    {
        typename DenseSet<Key>::const_iterator it = _keys.find(key);
        if (it == _keys.cend())
            throw std::out_of_range("Container does not have a element with key " + std::to_string(key));
        return _values[std::distance(_keys.cbegin(), it)];
    }

    T& operator[](const Key& key)
    {
        typename DenseSet<Key>::const_iterator it = _keys.find(key);
        if (it != _keys.cend())
            return _values[std::distance(_keys.cbegin(), it)];

        insert({key, T()});
        return _values.back();
    }

    T& operator[](Key&& key)
    {
        typename DenseSet<Key>::const_iterator it = _keys.find(key);
        if (it != _keys.cend())
            return _values[std::distance(_keys.cbegin(), it)];

        insert({key, T()});
        return _values.back();
    }

    // Comparison operators
    bool operator==(const DenseMap<Key, T>& rhs) const
    {
        for (const auto& key : _keys)
        {
            const_iterator it = rhs.find(key);
            if (it == rhs.cend())
                return false;
            if (it->second != _values[std::distance(rhs.cbegin(), it)])
                return false;
        }
        return true;
    }
    bool operator!=(const DenseMap<Key, T>& rhs) const
    {
        return !operator==(rhs);
    }
    bool operator<(const DenseMap<Key, T>& rhs) const
    {
        return std::lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend());
    }
    bool operator<=(const DenseMap<Key, T>& rhs) const
    {
        return !operator>(rhs);
    }
    bool operator>(const DenseMap<Key, T>& rhs) const
    {
        return std::lexicographical_compare(rhs.cbegin(), rhs.cend(), cbegin(), cend());
    }
    bool operator>=(const DenseMap<Key, T>& rhs) const
    {
        return !operator<(rhs);
    }

    // Iterators
    iterator begin() noexcept
    {
        return iterator(0, *this);
    }
    const_iterator begin() const noexcept
    {
        return const_iterator(0, *this);
    }
    const_iterator cbegin() const noexcept
    {
        return const_iterator(0, *this);
    }

    iterator end() noexcept
    {
        return iterator(_keys.size(), *this);
    }
    const_iterator end() const noexcept
    {
        return const_iterator(_keys.size(), *this);
    }
    const_iterator cend() const noexcept
    {
        return const_iterator(_keys.size(), *this);
    }

    iterator rbegin() noexcept
    {
        return iterator(_keys.size(), *this);
    }
    const_iterator rbegin() const noexcept
    {
        return const_iterator(_keys.size(), *this);
    }
    const_iterator crbegin() const noexcept
    {
        return const_iterator(_keys.size(), *this);
    }

    iterator rend() noexcept
    {
        return iterator(0, *this);
    }
    const_iterator rend() const noexcept
    {
        return const_iterator(0, *this);
    }
    const_iterator crend() const noexcept
    {
        return const_iterator(0, *this);
    }

    // Capacity
    bool empty() const noexcept
    {
        return _keys.empty();
    }
    size_t size() const noexcept
    {
        return _keys.size();
    }
    size_t max_size() const noexcept
    {
        return std::min(_keys.max_size(), _values.max_size());
    }
    void reserve(size_t size)
    {
        _keys.reserve(size);
        _values.reserve(size);
    }

    // Modifiers
    void clear()
    {
        _keys.clear();
        _values.clear();
    }

    std::pair<iterator, bool> insert(const std::pair<Key, T>& value)
    {
        auto result = _keys.insert(value.first);
        if (result.second)
        {
            _values.emplace_back(value.second);
            return std::make_pair(iterator(_keys.size() - 1, *this), true);
        }

        return std::make_pair(iterator(std::distance(_keys.begin(), result.first), *this), false);
    }
    std::pair<iterator, bool> insert(std::pair<Key, T>&& value)
    {
        auto result = _keys.insert(value.first);
        if (result.second)
        {
            _values.emplace_back(std::move(value.second));
            return std::make_pair(iterator(_keys.size() - 1, *this), true);
        }

        return std::make_pair(iterator(std::distance(_keys.begin(), result.first), *this), false);
    }
    template <class InputIt>
    void insert(InputIt first, InputIt last)
    {
        for (auto& it = first; it != last; ++it)
            insert(*it);
    }
    void insert(std::initializer_list<std::pair<Key, T>> init)
    {
        for (auto it = init.begin(); it != init.end(); ++it)
            insert(*it);
    }

    std::pair<iterator, bool> insert_or_assign(const Key& key, T&& obj)
    {
        auto result = _keys.insert(key);
        if (!result.second)
        {
            auto index = std::distance(_keys.begin(), result.first);
            _values[index] = obj;
            return std::make_pair(iterator(index, *this), false);
        }

        _values.emplace_back(obj);
        return std::make_pair(iterator(_keys.size() - 1, *this), true);
    }

    std::pair<iterator, bool> insert_or_assign(Key&& key, T&& obj)
    {
        auto result = _keys.insert(key);
        if (!result.second)
        {
            auto index = std::distance(_keys.begin(), result.first);
            _values[index] = obj;
            return std::make_pair(iterator(index, *this), false);
        }

        _values.emplace_back(obj);
        return std::make_pair(iterator(_keys.size() - 1, *this), true);
    }

    template <class... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        return insert({std::forward<Args>(args)...});
    }

    template <class... Args>
    std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args)
    {
        auto result = _keys.insert(key);
        if (!result.second)
            return std::make_pair(iterator(std::distance(_keys.begin(), result.first), *this), false);

        _values.emplace_back(args...);
        return std::make_pair(iterator(_keys.size() - 1, *this), true);
    }

    iterator erase(iterator pos)
    {
        auto it = _keys.find(pos->first);
        if (it == _keys.end())
            return end();
        auto index = std::distance(_keys.begin(), it);
        _keys.erase(it);
        _values.erase(_values.begin() + index);
        return iterator(index, *this);
    }
    iterator erase(const_iterator pos)
    {
        auto it = _keys.find(pos->first);
        if (it == _keys.end())
            return end();
        auto index = std::distance(_keys.begin(), it);
        _keys.erase(it);
        _values.erase(_values.begin() + index);
        return iterator(index, *this);
    }
    iterator erase(const_iterator first, const_iterator last)
    {
        auto current = first;
        while (current != last && current != cend())
            current = erase(current);
        auto index = std::distance(cbegin(), current);
        return iterator(index, *this);
    }
    size_t erase(const Key& key)
    {
        auto it = _keys.find(key);
        if (it == _keys.end())
            return 0;
        auto index = std::distance(_keys.begin(), it);
        _keys.erase(key);
        _values.erase(_values.begin() + index);
        return 1;
    }

    void swap(DenseMap& other)
    {
        std::swap(_keys, other._keys);
        std::swap(_values, other._values);
    }

    // Lookup
    iterator find(const Key& key)
    {
        typename DenseSet<Key>::const_iterator it = _keys.find(key);
        if (it == _keys.cend())
            return end();
        auto index = std::distance(_keys.cbegin(), it);
        return iterator(index, *this);
    }
    const_iterator find(const Key& key) const
    {
        typename DenseSet<Key>::const_iterator it = _keys.find(key);
        if (it == _keys.cend())
            return cend();
        auto index = std::distance(_keys.cbegin(), it);
        return const_iterator(index, *this);
    }

  protected:
    DenseSet<Key> _keys;
    std::vector<T> _values;
};

} // namespace Splash

#endif // SPLASH_DENSE_MAP_H
