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

#include <deque>
#include <memory>
#include <string>

#include "./core/serializer.h"

namespace Splash
{

struct Value;
using Values = std::deque<Value>;

/*************/
struct Value
{
  public:
    enum Type : uint8_t
    {
        integer = 0, // integer
        real,        // float
        string,      // string
        values       // values
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
                if (v._values)
                    *_values = *(v._values);
            }
            else
            {
                _values.reset(nullptr);
            }
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
            return false;
        case Type::integer:
            return _integer == v._integer;
        case Type::real:
            return _real == v._real;
        case Type::string:
            return _string == v._string;
        case Type::values:
            if (_values->size() != v._values->size())
                return false;
            bool isEqual = true;
            for (uint32_t i = 0; i < _values->size(); ++i)
                isEqual &= (_values->at(i) == v._values->at(i));
            return isEqual;
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
        if (_type != Type::values)
            return *this;
        else
            return _values->at(index);
    }

    template <class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
    T as() const
    {
        switch (_type)
        {
        default:
            return "";
        case Type::integer:
            return std::to_string(_integer);
        case Type::real:
            return std::to_string(_real);
        case Type::string:
            return _string;
        }
    }

    template <class T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
    T as() const
    {
        switch (_type)
        {
        default:
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
        }
    }

    template <class T, typename std::enable_if<std::is_same<T, Values>::value>::type* = nullptr>
    T as() const
    {
        switch (_type)
        {
        default:
            return {};
        case Type::integer:
            return {_integer};
        case Type::real:
            return {_real};
        case Type::string:
            return {_string};
        case Type::values:
            return *_values;
        }
    }

    void* data() const
    {
        switch (_type)
        {
        default:
            return nullptr;
        case Type::integer:
            return (void*)&_integer;
        case Type::real:
            return (void*)&_real;
        case Type::string:
            return (void*)_string.c_str();
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
            return ' ';
        case Type::integer:
            return 'n';
        case Type::real:
            return 'n';
        case Type::string:
            return 's';
        case Type::values:
            return 'v';
        }
    }

    uint32_t size() const
    {
        switch (_type)
        {
        default:
            return 0;
        case Type::integer:
            return sizeof(_integer);
        case Type::real:
            return sizeof(_real);
        case Type::string:
            return _string.size();
        case Type::values:
            return _values->size();
        }
    }

  private:
    std::string _name{""};
    Type _type{Type::integer};
    int64_t _integer{0};
    double _real{0.0};
    std::string _string{""};
    std::unique_ptr<Values> _values{nullptr};
};

// Specialisation of serialization for Splash::Value
namespace Serial
{

namespace detail
{

template <class T>
struct getSizeHelper<T, typename std::enable_if<std::is_same<T, Value>::value>::type>
{
    static size_t value(const Value& obj)
    {
        size_t acc = sizeof(Value::Type);
        auto objType = obj.getType();

        if (objType == Value::Type::values)
            return acc + getSize(obj.as<Values>());
        else if (objType == Value::Type::string)
            return acc + getSize(obj.as<std::string>());
        else
            return acc + obj.size();
    }
};

template <class T>
struct serializeHelper<T, typename std::enable_if<std::is_same<T, Value>::value>::type>
{
    static void apply(const Value& obj, std::vector<uint8_t>::iterator& it)
    {
        auto objType = obj.getType();
        serializer(static_cast<typename std::underlying_type<Value::Type>::type>(objType), it);

        if (objType == Value::Type::values)
        {
            serializer(obj.as<Values>(), it);
        }
        else if (objType == Value::Type::string)
        {
            serializer(obj.as<std::string>(), it);
        }
        else
        {
            auto ptr = reinterpret_cast<uint8_t*>(obj.data());
            std::copy(ptr, ptr + obj.size(), it);
            it += obj.size();
        }
    }
};

template <class T>
struct deserializeHelper<T, typename std::enable_if<std::is_same<T, Value>::value>::type>
{
    static Value apply(std::vector<uint8_t>::const_iterator& it)
    {
        T obj;
        Value::Type type;
        std::copy(it, it + sizeof(Value::Type), reinterpret_cast<char*>(&type));
        it += sizeof(Value::Type);

        switch (type)
        {
        case Value::Type::integer:
            obj = Value(deserializer<int64_t>(it));
            break;
        case Value::Type::real:
            obj = Value(deserializer<double>(it));
            break;
        case Value::Type::string:
            obj = Value(deserializer<std::string>(it));
            break;
        case Value::Type::values:
            obj = Value(deserializer<Values>(it));
            break;
        }

        return obj;
    }
};

} // namespace detail

} // namespace Serial

} // namespace Splash

#endif // SPLASH_VALUE_H
