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
 * @value.h
 * Class which can hold different data types, and convert between them
 */

#ifndef SPLASH_VALUE_H
#define SPLASH_VALUE_H

#include <cassert>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>

#include "./utils/dense_deque.h"
#include "./utils/resizable_array.h"

namespace Splash
{

struct Value;
using Values = DenseDeque<Value>;

/*************/
struct Value
{
  public:
    using Buffer = ResizableArray<uint8_t>;

  public:
    enum Type : uint8_t
    {
        integer = 0, // integer
        real,        // float
        string,      // string
        values,      // values
        buffer       // buffer
    };

    Value() = default;

    template <class T>
    Value(const T& v, const std::string& name = "")
        : _name(name)
    {
        if constexpr (std::is_integral_v<T>)
        {
            _type = Type::integer;
            _data = static_cast<int64_t>(v);
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            _type = Type::real;
            _data = static_cast<double>(v);
        }
        else if constexpr (std::is_same_v<T, std::string> || std::is_convertible_v<T, const char*>)
        {
            _type = Type::string;
            _data = v;
        }
        else if constexpr (std::is_same_v<T, Values>)
        {
            _type = Type::values;
            _data = v;
        }
        else if constexpr (std::is_same_v<T, Buffer>)
        {
            _type = Type::buffer;
            _data = v;
        }
        else
        {
            assert(false);
        }
    }

    template <class InputIt>
    Value(InputIt first, InputIt last)
        : _type(Type::values)
        , _data(Values(first, last))
    {
    }

    bool operator==(const Value& v) const
    {
        if (_type != v._type)
            return false;

        if (_name != v._name)
            return false;

        switch (_type)
        {
        default:
            assert(false);
            return false;
        case Type::integer:
            return std::get<int64_t>(_data) == std::get<int64_t>(v._data);
        case Type::real:
            return std::get<double>(_data) == std::get<double>(v._data);
        case Type::string:
            return std::get<std::string>(_data) == std::get<std::string>(v._data);
        case Type::values:
        {
            auto& data = std::get<Values>(_data);
            auto& other = std::get<Values>(v._data);
            if (data.size() != other.size())
                return false;
            bool isEqual = true;
            for (uint32_t i = 0; i < data.size(); ++i)
                isEqual &= (data[i] == other[i]);
            return isEqual;
        }
        case Type::buffer:
        {
            auto& data = std::get<Buffer>(_data);
            auto& other = std::get<Buffer>(v._data);
            if (data.size() != other.size())
                return false;
            for (uint32_t i = 0; i < data.size(); ++i)
                if (data[i] != other[i])
                    return false;
            return true;
        }
        }
    }

    bool operator==(const Values v) const
    {
        if (_type != Type::values)
            return false;

        auto& data = std::get<Values>(_data);
        if (data.size() != v.size())
            return false;
        bool isEqual = true;
        for (uint32_t i = 0; i < data.size(); ++i)
            isEqual &= (data[i] == v[i]);
        return isEqual;
    }

    bool operator!=(const Value& v) const { return !operator==(v); }

    Value& operator[](int index)
    {
        if (_type == Type::values)
        {
            auto& data = std::get<Values>(_data);
            return data[index];
        }
        else
        {
            return *this;
        }
    }

    template <class T>
    T as() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return {};
        case Type::integer:
            if constexpr (std::is_same_v<T, std::string>)
                return std::to_string(std::get<int64_t>(_data));
            else if constexpr (std::is_arithmetic_v<T>)
                return std::get<int64_t>(_data);
            else if constexpr (std::is_same_v<T, Values>)
                return {std::get<int64_t>(_data)};
            else if constexpr (std::is_same_v<T, Buffer>)
                return {};
            else
            {
                assert(false);
                return {};
            }
        case Type::real:
            if constexpr (std::is_same_v<T, std::string>)
                return std::to_string(std::get<double>(_data));
            else if constexpr (std::is_arithmetic_v<T>)
                return std::get<double>(_data);
            else if constexpr (std::is_same_v<T, Values>)
                return {std::get<double>(_data)};
            else if constexpr (std::is_same_v<T, Buffer>)
                return {};
            else
            {
                assert(false);
                return {};
            }
        case Type::string:
            if constexpr (std::is_same_v<T, std::string>)
                return std::get<std::string>(_data);
            else if constexpr (std::is_arithmetic_v<T>)
            {
                try
                {
                    return std::stof(std::get<std::string>(_data));
                }
                catch (...)
                {
                    return 0;
                }
            }
            else if constexpr (std::is_same_v<T, Values>)
                return {std::get<std::string>(_data)};
            else if constexpr (std::is_same_v<T, Buffer>)
                return {};
            else
            {
                assert(false);
                return {};
            }
        case Type::values:
            if constexpr (std::is_same_v<T, std::string>)
            {
                auto& data = std::get<Values>(_data);
                std::string out = "[";
                for (uint32_t i = 0; i < data.size(); ++i)
                {
                    out += data[i].as<std::string>();
                    if (data.size() > 1 && i < data.size() - 1)
                        out += ", ";
                }
                out += "]";
                return out;
            }
            else if constexpr (std::is_arithmetic_v<T>)
                return 0;
            else if constexpr (std::is_same_v<T, Values>)
                return std::get<Values>(_data);
            else if constexpr (std::is_same_v<T, Buffer>)
                return {};
            else
            {
                assert(false);
                return {};
            }
        case Type::buffer:
            if constexpr (std::is_same_v<T, std::string>)
            {
                auto& data = std::get<Buffer>(_data);
                std::string out = "(";
                static constexpr size_t maxBufferPrinted = 16;
                for (uint32_t i = 0; i < std::min(static_cast<size_t>(maxBufferPrinted), data.size()); ++i)
                    out += std::to_string(data[i]);
                if (data.size() > maxBufferPrinted)
                    out += "...";
                out += ")";
                return out;
            }
            else if constexpr (std::is_arithmetic_v<T>)
                return 0;
            else if constexpr (std::is_same_v<T, Values>)
                return {};
            else if constexpr (std::is_same_v<T, Buffer>)
                return std::get<Buffer>(_data);
            else
            {
                assert(false);
                return {};
            }
        }
    }

    void* data()
    {
        switch (_type)
        {
        default:
            assert(false);
            return nullptr;
        case Type::integer:
            return reinterpret_cast<void*>(const_cast<int64_t*>(&std::get<int64_t>(_data)));
        case Type::real:
            return reinterpret_cast<void*>(const_cast<double*>(&std::get<double>(_data)));
        case Type::string:
            return reinterpret_cast<void*>(const_cast<char*>(std::get<std::string>(_data).c_str()));
        case Type::values:
            return nullptr;
        case Type::buffer:
            return std::get<Buffer>(_data).data();
        }
    }

    const void* data() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return nullptr;
        case Type::integer:
            return reinterpret_cast<void*>(const_cast<int64_t*>(&std::get<int64_t>(_data)));
        case Type::real:
            return reinterpret_cast<void*>(const_cast<double*>(&std::get<double>(_data)));
        case Type::string:
            return reinterpret_cast<void*>(const_cast<char*>(std::get<std::string>(_data).c_str()));
        case Type::values:
            return nullptr;
        case Type::buffer:
            return std::get<Buffer>(_data).data();
        }
    }

    std::string getName() const { return _name; }
    void setName(const std::string& name) { _name = name; }
    bool isNamed() const { return !_name.empty(); }

    Type getType() const { return _type; }
    char getTypeAsChar() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return ' ';
        case Type::integer:
            return 'n';
        case Type::real:
            return 'n';
        case Type::string:
            return 's';
        case Type::values:
            return 'v';
        case Type::buffer:
            return 'b';
        }
    }

    /**
     * Return the size of the data in bytes
     * \return The data size in byte
     */
    size_t byte_size() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return 0;
        case Type::integer:
            return sizeof(int64_t);
        case Type::real:
            return sizeof(double);
        case Type::string:
            return std::get<std::string>(_data).size();
        case Type::values:
        {
            size_t size = 0;
            for (const auto& value : std::get<Values>(_data))
                size += value.byte_size();
            return size;
        }
        case Type::buffer:
            return std::get<Buffer>(_data).size();
        }
    }

    /**
     * Return the number of elements held in this object
     * \return Return the element count
     */
    size_t size() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return 0;
        case Type::integer:
            return 1;
        case Type::real:
            return 1;
        case Type::string:
            return std::get<std::string>(_data).size();
        case Type::values:
            return std::get<Values>(_data).size();
        case Type::buffer:
            return std::get<Buffer>(_data).size();
        }
    }

  private:
    std::string _name{""};
    Type _type{Type::integer};
    std::variant<int64_t, double, std::string, Values, Buffer> _data{};
}; // namespace Splash

} // namespace Splash

#endif // SPLASH_VALUE_H
