/*
 * Copyright (C) 2014 Emmanuel Durand
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
 * @coretypes.h
 * A few, mostly basic, types
 */

//#define GL_GLEXT_PROTOTYPES
//#define GLX_GLXEXT_PROTOTYPES

#include "config.h"

#define SPLASH
#define SPLASH_GL_DEBUG true
#define SPLASH_SAMPLES 0

#define SPLASH_ALL_PEERS "__ALL__"
#define SPLASH_DEFAULTS_FILE_ENV "SPLASH_DEFAULTS"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <deque>
#include <execinfo.h>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include "./threadpool.h"

#ifndef SPLASH_CORETYPES_H
#define SPLASH_CORETYPES_H

#define PRINT_FUNCTION_LINE std::cout << "------> " << __FUNCTION__ << "::" << __LINE__ << std::endl;
#define PRINT_CALL_STACK                                                                                                                                                           \
    {                                                                                                                                                                              \
        int j, nptrs;                                                                                                                                                              \
        int size = 100;                                                                                                                                                            \
        void* buffers[size];                                                                                                                                                       \
        char** strings;                                                                                                                                                            \
                                                                                                                                                                                   \
        nptrs = backtrace(buffers, size);                                                                                                                                          \
        strings = backtrace_symbols(buffers, nptrs);                                                                                                                               \
        for (j = 0; j < nptrs; ++j)                                                                                                                                                \
            std::cout << strings[j] << endl;                                                                                                                                       \
                                                                                                                                                                                   \
        free(strings);                                                                                                                                                             \
    }

namespace Splash
{

/*************/
//! Spinlock, when mutexes have too much overhead
class Spinlock
{
  public:
    void lock()
    {
        while (_lock.test_and_set(std::memory_order_acquire))
        {
        }
    }

    void unlock() { _lock.clear(std::memory_order_release); }

    bool try_lock() { return !_lock.test_and_set(std::memory_order_acquire); }

  private:
    std::atomic_flag _lock = ATOMIC_FLAG_INIT;
};

/*************/
//! Resizable array, used to hold big buffers (like raw images)
template <typename T>
class ResizableArray
{
  public:
    /**
     * \brief Constructor
     */
    ResizableArray(){};

    /**
     * \brief Constructor with an initial size
     * \param size Initial array size
     */
    ResizableArray(size_t size) { resize(size); }

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
    {
        _size = a._size;
        _shift = a._shift;
        _buffer = std::move(a._buffer);
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

/*************/
//! Object holding the serialized form of a BufferObject
struct SerializedObject
{
    /**
     * \brief Constructor
     */
    SerializedObject() {}

    /**
     * \brief Constructor with an initial size
     * \param size Initial array size
     */
    SerializedObject(int size) { _data.resize(size); }

    /**
     * \brief Constructor from two iterators
     * \param start Begin iterator
     * \param end End iterator
     */
    SerializedObject(char* start, char* end) { _data = ResizableArray<char>(start, end); }

    /**
     * \brief Get the pointer to the data
     * \return Return a pointer to the data
     */
    char* data() { return _data.data(); }

    /**
     * \brief Get ownership over the inner buffer. Use with caution, as it invalidates the SerializedObject
     * \return Return the inner buffer as a rvalue
     */
    ResizableArray<char>&& grabData() { return std::move(_data); }

    /**
     * \brief Get the size of the data
     * \return Return the size
     */
    std::size_t size() { return _data.size(); }

    /**
     * \brief Modify the size of the data
     * \param s New size
     */
    void resize(size_t s) { _data.resize(s); }

    //! Inner buffer
    ResizableArray<char> _data;
};

/*************/
//! Class holding the OpenGL context
class GlWindow
{
  public:
    /**
     * \brief Constructor
     * \param w Pointer to an existing GLFWwindow
     * \param mainWindow Pointer to an existing GLFWwindow which is shared with w
     */
    GlWindow(GLFWwindow* w, GLFWwindow* mainWindow)
    {
        _window = w;
        _mainWindow = mainWindow;
    }

    /**
     * \brief Destructor
     */
    ~GlWindow()
    {
        if (_window != nullptr)
            glfwDestroyWindow(_window);
    }

    /**
     * \brief Get the pointer to the GLFW window
     * \return Return the pointer to the GLFW window
     */
    GLFWwindow* get() const { return _window; }

    /**
     * \brief Get the pointer to the main GLFW window
     * \return Return the main GLFW window
     */
    GLFWwindow* getMainWindow() const { return _mainWindow; }

    /**
     * \brief Set the context of this window as current
     * \return Return true if everything went well
     */
    bool setAsCurrentContext() const
    {
        _previousWindow = glfwGetCurrentContext();
        if (_previousWindow == _window)
            return true;
        _mutex.lock();
        glfwMakeContextCurrent(_window);
        return true;
    }

    /**
     * \brief Release the context
     */
    void releaseContext() const
    {
        if (_window == _previousWindow)
            _previousWindow = nullptr;
        else if (glfwGetCurrentContext() == _window)
        {
            if (_previousWindow == nullptr)
                glfwMakeContextCurrent(NULL);
            else
            {
                glfwMakeContextCurrent(_previousWindow);
                _previousWindow = nullptr;
            }
            _mutex.unlock();
        }
    }

  private:
    mutable std::mutex _mutex;
    mutable GLFWwindow* _previousWindow{nullptr};
    GLFWwindow* _window{nullptr};
    GLFWwindow* _mainWindow{nullptr};
};

struct Value;
typedef std::deque<Value> Values;

/*************/
//! Class which can hold different data types, and convert between them
struct Value
{
  public:
    enum Type
    {
        i = 0,
        f,
        s,
        v
    };

    Value() {}

    template <class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    Value(T v)
    {
        _i = v;
        _type = Type::i;
    }

    template <class T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    Value(T v)
    {
        _f = v;
        _type = Type::f;
    }

    template <class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
    Value(T v)
    {
        _s = v;
        _type = Type::s;
    }

    template <class T, typename std::enable_if<std::is_same<T, const char*>::value>::type* = nullptr>
    Value(T c)
    {
        _s = std::string(c);
        _type = Type::s;
    }

    template <class T, typename std::enable_if<std::is_same<T, Values>::value>::type* = nullptr>
    Value(T v)
    {
        _v = std::unique_ptr<Values>(new Values());
        *_v = v;
        _type = Type::v;
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
            _s = v._s;
            _v = std::unique_ptr<Values>(new Values());
            if (v._v)
                *_v = *(v._v);
        }

        return *this;
    }

    template <class InputIt>
    Value(InputIt first, InputIt last)
    {
        _type = Type::v;
        _v = std::unique_ptr<Values>(new Values());

        auto it = first;
        while (it != last)
        {
            _v->push_back(Value(*it));
            ++it;
        }
    }

    bool operator==(Value v) const
    {
        if (_type != v._type)
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
            for (int i = 0; i < _v->size(); ++i)
                isEqual &= (_v->at(i) == v._v->at(i));
            return isEqual;
        }
    }

    bool operator!=(Value v) const { return !operator==(v); }

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

    Type getType() const { return _type; }
    char getTypeAsChar() const
    {
        switch (_type)
        {
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

    int size() const
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
    Type _type{Type::i};
    union {
        int64_t _i{0};
        double _f;
    };
    std::string _s;
    std::unique_ptr<Values> _v{nullptr};
};

/*************/
//! OnScopeExit, taken from Switcher (https://github.com/nicobou/switcher)
template <typename F>
class ScopeGuard
{
  public:
    explicit ScopeGuard(F&& f)
        : f_(std::move(f))
    {
    }
    ~ScopeGuard() { f_(); }
  private:
    F f_;
};

enum class ScopeGuardOnExit
{
};
template <typename F>
ScopeGuard<F> operator+(ScopeGuardOnExit, F&& f)
{
    return ScopeGuard<F>(std::forward<F>(f));
}

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
#define OnScopeExit auto CONCATENATE(on_scope_exit_var, __LINE__) = ScopeGuardOnExit() + [&]()

} // end of namespace

#endif // SPLASH_CORETYPES_H
