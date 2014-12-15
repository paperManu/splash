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
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @coretypes.h
 * A few, mostly basic, types
 */

#ifndef SPLASH_CORETYPES_H
#define SPLASH_CORETYPES_H

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES

#define SPLASH
#define SPLASH_GL_CONTEXT_VERSION_MAJOR 3
#define SPLASH_GL_CONTEXT_VERSION_MINOR 3
#define SPLASH_GL_DEBUG false
#define SPLASH_SAMPLES 4

#define SPLASH_ALL_PAIRS "__ALL__"

#include <atomic>
#include <chrono>
#include <ostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <json/reader.h>

#include "config.h"
#include "threadpool.h"

namespace Splash
{

/*************/
// All object types are specified here,
// so that we don't have to do it everywhere

class Camera;
class Geometry;
class Gui;
class GlvGlobalView;
class GlvControl;
class Image;
class Image_Shmdata;
class Link;
class Mesh;
class Mesh_Shmdata;
class Object;
class Scene;
class Shader;
class Texture;
class Window;

typedef std::shared_ptr<Camera> CameraPtr;
typedef std::shared_ptr<Geometry> GeometryPtr;
typedef std::shared_ptr<Gui> GuiPtr;
typedef std::shared_ptr<Image> ImagePtr;
typedef std::shared_ptr<Image_Shmdata> Image_ShmdataPtr;
typedef std::shared_ptr<Link> LinkPtr;
typedef std::shared_ptr<Mesh> MeshPtr;
typedef std::shared_ptr<Mesh_Shmdata> Mesh_ShmdataPtr;
typedef std::shared_ptr<Object> ObjectPtr;
typedef std::shared_ptr<Scene> ScenePtr;
typedef std::shared_ptr<Shader> ShaderPtr;
typedef std::shared_ptr<Texture> TexturePtr;
typedef std::shared_ptr<Window> WindowPtr;

/*************/
struct SerializedObject
{
    /**
     * Constructors
     */
    SerializedObject() {}

    SerializedObject(int size)
    {
        _data.resize(size);
    }

    SerializedObject(char* start, char* end)
    {
        _data = std::vector<char>(start, end);
    }

    SerializedObject(const SerializedObject& obj)
    {
        _data = obj._data;
    }

    SerializedObject(SerializedObject&& obj)
    {
        *this = std::move(obj);
    }

    /**
     * Operators
     */
    SerializedObject& operator=(const SerializedObject& obj)
    {
        if (this != &obj)
        {
            _data = obj._data;
        }
        return *this;
    }

    SerializedObject& operator=(SerializedObject&& obj)
    {
        if (this != &obj)
        {
            _data = std::move(obj._data);
        }
        return *this;
    }

    /**
     * Get the pointer to the data
     */
    char* data()
    {
        return _data.data();
    }

    /**
     * Return the size of the data
     */
    std::size_t size()
    {
        return _data.size();
    }

    /**
     * Modify the size of the data
     */
    void resize(size_t s)
    {
        _data.resize(s);
    }

    /**
     * Attributes
     */
    std::mutex _mutex;
    std::vector<char> _data;
};

//typedef std::vector<char> SerializedObject;
typedef std::shared_ptr<SerializedObject> SerializedObjectPtr;

/*************/
class GlWindow
{
    public:
        /**
         * Constructors
         */
        GlWindow(GLFWwindow* w, GLFWwindow* mainWindow)
        {
            _window = w;
            _mainWindow = mainWindow;
        }

        /**
         * Destructor
         */
        ~GlWindow()
        {
            if (_window != nullptr)
                glfwDestroyWindow(_window);
        }

        /**
         * Get the pointer to the GLFW window
         */
        GLFWwindow* get() const {return _window;}

        /**
         * Get the pointer to the main GLFW window
         */
        GLFWwindow* getMainWindow() const {return _mainWindow;}

        /**
         * Set the context of this window as current
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
         * Release the context
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
        mutable GLFWwindow* _previousWindow {nullptr};
        GLFWwindow* _window {nullptr};
        GLFWwindow* _mainWindow {nullptr};
};

typedef std::shared_ptr<GlWindow> GlWindowPtr;

struct Value;
typedef std::vector<Value> Values;

/*************/
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

        Value() {_i = 0; _type = Type::i;}
        Value(int v) {_i = v; _type = Type::i;}
        Value(float v) {_f = v; _type = Type::f;}
        Value(double v) {_f = (float)v; _type = Type::f;}
        Value(std::string v) {_s = v; _type = Type::s;}
        Value(const char* c) {_s = std::string(c); _type = Type::s;}
        Value(Values v) {_v = v; _type = Type::v;}

        Value(const Value& v) noexcept
        {
            _i = v._i;
            _f = v._f;
            _s = v._s;
            _v = v._v;
            _type = v._type;
        }

        Value& operator=(const Value& v) noexcept
        {
            if (this != &v)
            {
                _i = v._i;
                _f = v._f;
                _s = v._s;
                _v = v._v;
                _type = v._type;
            }
            return *this;
        }

        Value(Value&& v) noexcept
        {
            *this = std::move(v);
        }

        Value& operator=(Value&& v) noexcept
        {
            if (this != &v)
            {
                _i = v._i;
                _f = v._f;
                _s = v._s;
                _v = v._v;
                _type = v._type;
            }
            return *this;
        }

        bool operator==(Value v)
        {
            if (_type != v._type)
                return false;
            else if (_type == Type::i)
                return _i == v._i;
            else if (_type == Type::f)
                return _f == v._f;
            else if (_type == Type::s)
                return _s == v._s;
            else if (_type == Type::v)
            {
                if (_v.size() != v._v.size())
                    return false;
                bool isEqual = true;
                for (int i = 0; i < _v.size(); ++i)
                    isEqual &= (_v[i] == v._v[i]);
                return isEqual;
            }
        }

        int asInt()
        {
            if (_type == Type::i)
                return _i;
            else if (_type == Type::f)
                return (int)_f;
            else if (_type == Type::s)
                try {return std::stoi(_s);}
                catch (...) {return 0;}
            else
                return 0;
        }

        float asFloat()
        {
            if (_type == Type::i)
                return (float)_i;
            else if (_type == Type::f)
                return _f;
            else if (_type == Type::s)
                try {return std::stof(_s);}
                catch (...) {return 0.f;}
            else
                return 0.f;
        }

        std::string asString()
        {
            if (_type == Type::i)
                try {return std::to_string(_i);}
                catch (...) {return std::string();}
            else if (_type == Type::f)
                try {return std::to_string(_f);}
                catch (...) {return std::string();}
            else if (_type == Type::s)
                return _s;
            else
                return "";
        }

        Values asValues()
        {
            if (_type == Type::i)
                return Values({_i});
            else if (_type == Type::f)
                return Values({_f});
            else if (_type == Type::s)
                return Values({_s});
            else if (_type == Type::v)
                return _v;
        }

        void* data()
        {
            if (_type == Type::i)
                return (void*)&_i;
            else if (_type == Type::f)
                return (void*)&_f;
            else if (_type == Type::s)
                return (void*)_s.c_str();
            else
                return nullptr;
        }

        Type getType() {return _type;}
        
        int size()
        {
            if (_type == Type::i)
                return sizeof(_i);
            else if (_type == Type::f)
                return sizeof(_f);
            else if (_type == Type::s)
                return _s.size();
            else
                return 0;
        }

    private:
        Type _type;
        int _i {0};
        float _f {0.f};
        std::string _s {""};
        Values _v {};
};

/*************/
struct AttributeFunctor
{
    public:
        AttributeFunctor() {}
        AttributeFunctor(std::function<bool(Values)> setFunc) {_setFunc = setFunc;}
        AttributeFunctor(std::function<bool(Values)> setFunc,
                            std::function<Values()> getFunc) {_setFunc = setFunc; _getFunc = getFunc;}

        bool operator()(Values args)
        {
            if (!_setFunc)
                return false;
            return _setFunc(args);
        }
        Values operator()()
        {
            if (!_getFunc)
                return Values();
            return _getFunc();
        }

    private:
        std::function<bool(Values)> _setFunc;
        std::function<Values()> _getFunc;
};

class BaseObject;
typedef std::shared_ptr<BaseObject> BaseObjectPtr;
class RootObject;
typedef std::weak_ptr<RootObject> RootObjectWeakPtr;

/*************/
class BaseObject
{
    public:
        BaseObject() {}
        BaseObject(RootObjectWeakPtr root) {_root = root;}
        virtual ~BaseObject() {}

        std::string getType() const {return _type;}

        /**
         * Set and get the id of the object
         */
        unsigned long getId() const {return _id;}
        void setId(unsigned long id) {_id = id;}

        /**
         * Set and get the name of the object
         */
        std::string getName() const {return _name;}
        void setName(std::string name) {_name = name;}

        /**
         * Set and get the remote type of the object
         */
        std::string getRemoteType() const {return _remoteType;}
        void setRemoteType(std::string type) {_remoteType = type;}

        /**
         * Try to link the given BaseObject to this
         */
        virtual bool linkTo(BaseObjectPtr obj) {return false;}

        /**
         * Set the specified attribute
         */
        bool setAttribute(std::string attrib, Values args)
        {
            if (_attribFunctions.find(attrib) == _attribFunctions.end())
                return false;
            _updatedParams = true;
            return _attribFunctions[attrib](args);
        }

        /**
         * Get the specified attribute
         */
        bool getAttribute(std::string attrib, Values& args)
        {
            if (_attribFunctions.find(attrib) == _attribFunctions.end())
                return false;
            args = _attribFunctions[attrib]();
            return true;
        }

        /**
         * Get all the attributes as a map
         */
        std::map<std::string, Values> getAttributes()
        {
            std::map<std::string, Values> attribs;
            for (auto& attr : _attribFunctions)
            {
                Values values;
                if (getAttribute(attr.first, values) == false || values.size() == 0)
                    continue;
                attribs[attr.first] = values;
            }

            return attribs;
        }

        /**
         * Check whether the objects needs to be updated
         */
        virtual bool wasUpdated() {return _updatedParams;}

        /**
         * Reset the "was updated" status, if needed
         */
        virtual void setNotUpdated() {_updatedParams = false;}
        
        /**
         * Update the content of the object
         */
        virtual void update() {}

        /**
         * Get the configuration as a json object
         */
        Json::Value getValuesAsJson(Values values)
        {
            Json::Value jsValue;
            for (auto& v : values)
            {
                switch (v.getType())
                {
                default:
                    continue;
                case Value::i:
                    jsValue.append(v.asInt());
                    break;
                case Value::f:
                    jsValue.append(v.asFloat());
                    break;
                case Value::s:
                    jsValue.append(v.asString());
                    break;
                case Value::v:
                    jsValue.append(getValuesAsJson(v.asValues()));
                    break;
                }
            }
            return jsValue;
        }

        Json::Value getConfigurationAsJson()
        {
            Json::Value root;
            if (_remoteType == "")
                root["type"] = _type;
            else
                root["type"] = _remoteType;

            for (auto& attr : _attribFunctions)
            {
                Values values;
                if (getAttribute(attr.first, values) == false || values.size() == 0)
                    continue;

                Json::Value jsValue;
                jsValue = getValuesAsJson(values);
                root[attr.first] = jsValue;
            }
            return root;
        }

    protected:
        unsigned long _id;
        std::string _type {"baseobject"};
        std::string _remoteType {""};
        std::string _name {""};
        RootObjectWeakPtr _root;
        std::map<std::string, AttributeFunctor> _attribFunctions;
        bool _updatedParams {true};

        /**
         * Register new functors to modify attributes
         */
        virtual void registerAttributes() = 0;
};

/*************/
class BufferObject : public BaseObject
{
    public:
        BufferObject() {}
        BufferObject(RootObjectWeakPtr root) : BaseObject(root) {}

        virtual ~BufferObject() {}

        /**
         * Returns true if the object has been updated
         */
        bool wasUpdated() {return _updatedBuffer | BaseObject::wasUpdated();}

        /**
         * Set the updated buffer flag to false.
         */
        void setNotUpdated() {BaseObject::setNotUpdated(); _updatedBuffer = false;}

        /**
         * Update the Image from a serialized representation
         * The second definition updates from the inner serialized object
         */
        virtual bool deserialize(const SerializedObjectPtr obj) = 0;
        bool deserialize()
        {
            if (_newSerializedObject == false)
                return true;

            bool _returnValue = deserialize(_serializedObject);
            _newSerializedObject = false;

            return _returnValue;
        }

        /**
         * Serialize the image
         */
        virtual SerializedObjectPtr serialize() const = 0;

        /**
         * Set the next serialized object to deserialize to buffer
         */
        void setSerializedObject(SerializedObjectPtr obj)
        {
            {
                std::lock_guard<std::mutex> lock(_writeMutex);
                _serializedObject = move(obj);
                _newSerializedObject = true;
            }

            // Deserialize it right away, in a separate thread
            SThread::pool.enqueueWithoutId([&]() {
                deserialize();
            });
        }

        /**
         * Updates the timestamp of the object. Also, set the update flag to true
         */
        void updateTimestamp()
        {
            _timestamp = std::chrono::high_resolution_clock::now();
            _updatedBuffer = true;
        }

    protected:
        mutable std::mutex _readMutex;
        mutable std::mutex _writeMutex;
        std::chrono::high_resolution_clock::time_point _timestamp;
        bool _updatedBuffer {false};

        SerializedObjectPtr _serializedObject;
        bool _newSerializedObject {false};
};

typedef std::shared_ptr<BufferObject> BufferObjectPtr;

/*************/
class RootObject : public BaseObject
{
    public:
        virtual ~RootObject() {}

        /**
         * Register an object which was created elsewhere
         * If an object was the same name exists, it is replaced
         */
        void registerObject(BaseObjectPtr object)
        {
            if (object.get() != nullptr)
                _objects[object->getName()] = object;
        }

        /**
         * Set the attribute of the named object with the given args
         */
        bool set(std::string name, std::string attrib, Values args)
        {
            std::lock_guard<std::mutex> lock(_setMutex);
            if (name == _name || name == SPLASH_ALL_PAIRS)
                return setAttribute(attrib, args);
            else if (_objects.find(name) != _objects.end())
                return _objects[name]->setAttribute(attrib, args);
        }

        /**
         * Set an object from its serialized form
         * If non existant, it is handled by the handleSerializedObject method
         */
        void setFromSerializedObject(const std::string name, const SerializedObjectPtr obj)
        {
            std::lock_guard<std::mutex> lock(_setMutex);
            if (_objects.find(name) != _objects.end() && std::dynamic_pointer_cast<BufferObject>(_objects[name]).get() != nullptr)
                std::dynamic_pointer_cast<BufferObject>(_objects[name])->setSerializedObject(obj);
            else
                handleSerializedObject(name, obj);
        }

    protected:
        mutable std::mutex _setMutex;
        std::map<std::string, BaseObjectPtr> _objects;

        virtual void handleSerializedObject(const std::string name, const SerializedObjectPtr obj) {}
};

typedef std::shared_ptr<RootObject> RootObjectPtr;

} // end of namespace

#endif // SPLASH_CORETYPES_H
