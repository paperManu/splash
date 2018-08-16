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
#include <deque>
#include <memory>
#include <string>

#include "./core/resizable_array.h"

namespace Splash
{

struct Value;
using Values = std::deque<Value>;

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

    template <class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    Value(const T& v, const std::string& name = "")
        : _name(name)
        , _type(Type::integer)
        , _integer(v)
    {
    }

    template <class T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    Value(const T& v, const std::string& name = "")
        : _name(name)
        , _type(Type::real)
        , _real(v)
    {
    }

    template <class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
    Value(const T& v, const std::string& name = "")
        : _name(name)
        , _type(Type::string)
        , _string(v)
    {
    }

    template <class T, typename std::enable_if<std::is_same<T, const char*>::value>::type* = nullptr>
    Value(T c, const std::string& name = "")
        : _name(name)
        , _type(Type::string)
        , _string(c)
    {
    }

    template <class T, typename std::enable_if<std::is_same<T, Values>::value>::type* = nullptr>
    Value(const T& v, const std::string& name = "")
        : _name(name)
        , _type(Type::values)
        , _values(std::make_unique<Values>(v))
    {
    }

    template <class T, typename std::enable_if<std::is_same<T, Buffer>::value>::type* = nullptr>
    Value(const T& v, const std::string& name = "")
        : _name(name)
        , _type(Type::buffer)
        , _buffer(std::make_unique<Buffer>(v))
    {
    }

    Value(const Value& v) { operator=(v); }
    Value& operator=(const Value& v)
    {
        if (this != &v)
        {
            _name = v._name;
            _type = v._type;
            _integer = v._integer;
            _real = v._real;
            _string = v._string;

            if (v._values)
            {
                _values = std::make_unique<Values>();
                *_values = *(v._values);
            }
            else
            {
                _values.reset(nullptr);
            }

            if (v._buffer)
                _buffer = std::make_unique<Buffer>(*v._buffer);
            else
                _buffer.reset(nullptr);
        }

        return *this;
    }

    Value& operator[](const std::string& name)
    {
        _name = name;
        return *this;
    }

    template <class InputIt>
    Value(InputIt first, InputIt last)
        : _type(Type::values)
        , _values(std::make_unique<Values>())
    {
        auto it = first;
        while (it != last)
        {
            _values->push_back(Value(*it));
            ++it;
        }
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
            return _integer == v._integer;
        case Type::real:
            return _real == v._real;
        case Type::string:
            return _string == v._string;
        case Type::values:
        {
            if (_values->size() != v._values->size())
                return false;
            bool isEqual = true;
            for (uint32_t i = 0; i < _values->size(); ++i)
                isEqual &= (_values->at(i) == v._values->at(i));
            return isEqual;
        }
        case Type::buffer:
        {
            if (_buffer->size() != v._buffer->size())
                return false;
            for (uint32_t i = 0; i < _buffer->size(); ++i)
                if ((*_buffer)[i] != (*v._buffer)[i])
                    return false;
            return true;
        }
        }
    }

    bool operator==(const Values v) const
    {
        if (_type != Type::values)
            return false;
        if (_values->size() != v.size())
            return false;
        bool isEqual = true;
        for (uint32_t i = 0; i < _values->size(); ++i)
            isEqual &= (_values->at(i) == v.at(i));
        return isEqual;
    }

    bool operator!=(const Value& v) const { return !operator==(v); }

    Value& operator[](int index)
    {
        if (_type == Type::values)
        {
            assert(_values != nullptr);
            return _values->at(index);
        }
        else
        {
            return *this;
        }
    }

    template <class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
    T as() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return "";
        case Type::integer:
            return std::to_string(_integer);
        case Type::real:
            return std::to_string(_real);
        case Type::string:
            return _string;
        case Type::values:
        {
            std::string out = "[";
            for (uint32_t i = 0; i < _values->size(); ++i)
            {
                out += (*_values)[i].as<std::string>();
                if (_values->size() > 1 && i < _values->size() - 1)
                    out += ", ";
            }
            out += "]";
            return out;
        }
        case Type::buffer:
        {
            std::string out = "(";
            const static size_t maxBufferPrinted = 16;
            for (uint32_t i = 0; i < std::min(static_cast<size_t>(maxBufferPrinted), _buffer->size()); ++i)
                out += std::to_string((*_buffer)[i]);
            if (_buffer->size() > maxBufferPrinted)
                out += "...";
            out += ")";
            return out;
        }
        }
    }

    template <class T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
    T as() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return 0;
        case Type::integer:
            return _integer;
        case Type::real:
            return _real;
        case Type::string:
            try
            {
                return std::stof(_string);
            }
            catch (...)
            {
                return 0;
            }
        case Type::values:
            return 0;
        case Type::buffer:
            return 0;
        }
    }

    template <class T, typename std::enable_if<std::is_same<T, Values>::value>::type* = nullptr>
    T as() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return {};
        case Type::integer:
            return {_integer};
        case Type::real:
            return {_real};
        case Type::string:
            return {_string};
        case Type::values:
            return *_values;
        case Type::buffer:
            return {};
        }
    }

    template <class T, typename std::enable_if<std::is_same<T, Buffer>::value>::type* = nullptr>
    T as() const
    {
        if (_type != Type::buffer)
            return {};
        return *_buffer;
    }

    void* data() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return nullptr;
        case Type::integer:
            return (void*)&_integer;
        case Type::real:
            return (void*)&_real;
        case Type::string:
            return (void*)_string.c_str();
        case Type::values:
            return nullptr;
        case Type::buffer:
            return _buffer->data();
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

    size_t size() const
    {
        switch (_type)
        {
        default:
            assert(false);
            return 0;
        case Type::integer:
            return sizeof(_integer);
        case Type::real:
            return sizeof(_real);
        case Type::string:
            return _string.size();
        case Type::values:
            return _values->size();
        case Type::buffer:
            return _buffer->size();
        }
    }

  private:
    std::string _name{""};
    Type _type{Type::integer};
    int64_t _integer{0};
    double _real{0.0};
    std::string _string{""};
    std::unique_ptr<Values> _values{nullptr};
    std::unique_ptr<Buffer> _buffer{nullptr};
}; // namespace Splash

} // namespace Splash

#endif // SPLASH_VALUE_H
