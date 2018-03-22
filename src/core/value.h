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

namespace Splash
{

struct Value;
typedef std::deque<Value> Values;

/*************/
struct Value
{
  public:
    enum Type
    {
        i = 0,  // integer
        f,      // float
        s,      // string
        v       // values
    };

    Value() {}

    template <class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    Value(T v, std::string name = "")
        : _i(v)
        , _type(Type::i)
        , _name(name)
    {
    }

    template <class T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    Value(T v, std::string name = "")
        : _f(v)
        , _type(Type::f)
        , _name(name)
    {
    }

    template <class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
    Value(T v, std::string name = "")
        : _s(v)
        , _type(Type::s)
        , _name(name)
    {
    }

    template <class T, typename std::enable_if<std::is_same<T, const char*>::value>::type* = nullptr>
    Value(T c, std::string name = "")
        : _s(std::string(c))
        , _type(Type::s)
        , _name(name)
    {
    }

    template <class T, typename std::enable_if<std::is_same<T, Values>::value>::type* = nullptr>
    Value(T v, std::string name = "")
        : _v(std::unique_ptr<Values>(new Values(v)))
        , _type(Type::v)
        , _name(name)
    {
    }

    Value(const Value& v) { operator=(v); }

    Value& operator=(const Value& v)
    {
        if (this != &v)
        {
            _type = v._type;
            switch (_type)
            {
            case Type::i:
                _i = v._i;
                break;
            case Type::f:
                _f = v._f;
                break;
            case Type::s:
            case Type::v:
                break;
            }
            _name = v._name;
            _s = v._s;
            _v = std::unique_ptr<Values>(new Values());
            if (v._v)
                *_v = *(v._v);
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
        : _v(std::unique_ptr<Values>(new Values()))
        , _type(Type::v)
    {
        auto it = first;
        while (it != last)
        {
            _v->push_back(Value(*it));
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
        case Type::i:
            return _i == v._i;
        case Type::f:
            return _f == v._f;
        case Type::s:
            return _s == v._s;
        case Type::v:
            if (_v->size() != v._v->size())
                return false;
            bool isEqual = true;
            for (uint32_t i = 0; i < _v->size(); ++i)
                isEqual &= (_v->at(i) == v._v->at(i));
            return isEqual;
        }
    }

    bool operator!=(const Value& v) const { return !operator==(v); }

    Value& operator[](int index)
    {
        if (_type != Type::v)
            return *this;
        else
            return _v->at(index);
    }

    template <class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
    T as() const
    {
        switch (_type)
        {
        default:
            return "";
        case Type::i:
            return std::to_string(_i);
        case Type::f:
            return std::to_string(_f);
        case Type::s:
            return _s;
        }
    }

    template <class T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
    T as() const
    {
        switch (_type)
        {
        default:
            return 0;
        case Type::i:
            return _i;
        case Type::f:
            return _f;
        case Type::s:
            try
            {
                return std::stof(_s);
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
        case Type::i:
            return {_i};
        case Type::f:
            return {_f};
        case Type::s:
            return {_s};
        case Type::v:
            return *_v;
        }
    }

    void* data()
    {
        switch (_type)
        {
        default:
            return nullptr;
        case Type::i:
            return (void*)&_i;
        case Type::f:
            return (void*)&_f;
        case Type::s:
            return (void*)_s.c_str();
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
        case Type::i:
            return 'n';
        case Type::f:
            return 'n';
        case Type::s:
            return 's';
        case Type::v:
            return 'v';
        }
    }

    uint32_t size() const
    {
        switch (_type)
        {
        default:
            return 0;
        case Type::i:
            return sizeof(_i);
        case Type::f:
            return sizeof(_f);
        case Type::s:
            return _s.size();
        case Type::v:
            return _v->size();
        }
    }

  private:
    union {
        int64_t _i{0};
        double _f;
    };
    std::string _s{""};
    std::unique_ptr<Values> _v{nullptr};
    Type _type{Type::i};
    std::string _name{""};
};

} // end of namespace

#endif // SPLASH_VALUE_H
