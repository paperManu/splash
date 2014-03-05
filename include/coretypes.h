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

#ifndef CORETYPES_H
#define CORETYPES_H

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#define SPLASH
#define SPLASH_GL_CONTEXT_VERSION_MAJOR 3
#define SPLASH_GL_CONTEXT_VERSION_MINOR 3
#define SPLASH_GL_DEBUG false
#define SPLASH_SWAP_INTERVAL 1
#define SPLASH_SAMPLES 4
#define SPLASH_MAX_THREAD 4

#define SPLASH_ALL_PAIRS "__ALL__"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <json/reader.h>

#include "config.h"

namespace Splash
{

/*************/
typedef std::vector<unsigned char> SerializedObject;
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
            if (glfwGetCurrentContext() != NULL)
                return false;
            _mutex.lock();
            glfwMakeContextCurrent(_window);
            return true;
        }

        /**
         * Release the context
         */
        void releaseContext() const
        {
            if (glfwGetCurrentContext() == _window)
            {
                glfwMakeContextCurrent(NULL);
                _mutex.unlock();
            }
        }

    private:
        mutable std::mutex _mutex;
        GLFWwindow* _window {nullptr};
        GLFWwindow* _mainWindow {nullptr};
};

typedef std::shared_ptr<GlWindow> GlWindowPtr;

/*************/
struct Value
{
    public:
        enum Type
        {
            i = 0,
            f,
            s
        };

        Value(int v) {_i = v; _type = i;}
        Value(float v) {_f = v; _type = f;}
        Value(double v) {_f = (float)v; _type = f;}
        Value(std::string v) {_s = v; _type = s;}
        Value(const char* c) {_s = std::string(c); _type = s;}

        int asInt()
        {
            if (_type == i)
                return _i;
            else if (_type == f)
                return (int)_f;
            else if (_type == s)
                try {return std::stoi(_s);}
                catch (...) {return 0;}
        }

        float asFloat()
        {
            if (_type == i)
                return (float)_i;
            else if (_type == f)
                return _f;
            else if (_type == s)
                try {return std::stof(_s);}
                catch (...) {return 0.f;}
        }

        std::string asString()
        {
            if (_type == i)
                try {return std::to_string(_i);}
                catch (...) {return std::string();}
            else if (_type == f)
                try {return std::to_string(_f);}
                catch (...) {return std::string();}
            else if (_type == s)
                return _s;
        }

        void* data()
        {
            if (_type == i)
                return (void*)&_i;
            else if (_type == f)
                return (void*)&_f;
            else if (_type == s)
                return (void*)_s.c_str();
        }

        Type getType() {return _type;}
        
        int size()
        {
            if (_type == i)
                return sizeof(_i);
            else if (_type == f)
                return sizeof(_f);
            else if (_type == s)
                return _s.size();
        }

    private:
        Type _type;
        int _i;
        float _f;
        std::string _s;
};

/*************/
struct AttributeFunctor
{
    public:
        AttributeFunctor() {}
        AttributeFunctor(std::function<bool(std::vector<Value>)> setFunc) {_setFunc = setFunc;}
        AttributeFunctor(std::function<bool(std::vector<Value>)> setFunc,
                            std::function<std::vector<Value>()> getFunc) {_setFunc = setFunc; _getFunc = getFunc;}

        bool operator()(std::vector<Value> args)
        {
            if (!_setFunc)
                return false;
            return _setFunc(args);
        }
        std::vector<Value> operator()()
        {
            if (!_getFunc)
                return std::vector<Value>();
            return _getFunc();
        }

    private:
        std::function<bool(std::vector<Value>)> _setFunc;
        std::function<std::vector<Value>()> _getFunc;
};

class BaseObject;
typedef std::shared_ptr<BaseObject> BaseObjectPtr;

/*************/
class BaseObject
{
    public:
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
         * Register modifiable attributes
         */
        virtual void registerAttributes() {}

        /**
         * Set the specified attribute
         */
        bool setAttribute(std::string attrib, std::vector<Value> args)
        {
            if (_attribFunctions.find(attrib) == _attribFunctions.end())
                return false;
            return _attribFunctions[attrib](args);
        }

        /**
         * Get the specified attribute
         */
        bool getAttribute(std::string attrib, std::vector<Value>& args)
        {
            if (_attribFunctions.find(attrib) == _attribFunctions.end())
                return false;
            args = _attribFunctions[attrib]();
            return true;
        }
        
        /**
         * Update the content of the object
         */
        virtual void update() {}

        /**
         * Get the configuration as a json object
         */
        Json::Value getConfigurationAsJson()
        {
            Json::Value root;
            if (_remoteType == "")
                root["type"] = _type;
            else
                root["type"] = _remoteType;

            for (auto& attr : _attribFunctions)
            {
                std::vector<Value> values;
                if (getAttribute(attr.first, values) == false || values.size() == 0)
                    continue;

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
                    }
                }

                root[attr.first] = jsValue;
            }
            return root;
        }

    protected:
        unsigned long _id;
        std::string _type {"baseobject"};
        std::string _remoteType {""};
        std::string _name {""};
        std::map<std::string, AttributeFunctor> _attribFunctions;
};

typedef std::shared_ptr<BaseObject> BaseObjectPtr;

/*************/
class BufferObject : public BaseObject
{
    public:
        virtual ~BufferObject() {}

        /**
         * Returns true if the object has been updated
         */
        bool wasUpdated() {return _updatedBuffer;}

        /**
         * Set the updated buffer flag to false.
         */
        void setNotUpdated() {_updatedBuffer = false;}

        /**
         * Update the Image from a serialized representation
         * The second definition updates from the inner serialized object
         */
        virtual bool deserialize(const SerializedObjectPtr obj) = 0;
        bool deserialize()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_newSerializedObject == false)
                return true;

            bool _returnValue = deserialize(_serializedObject);
            _newSerializedObject = false;

            return _returnValue;
        }

        /**
         * Serialize the image
         */
        virtual SerializedObject serialize() const = 0;

        /**
         * Set the next serialized object to deserialize to buffer
         */
        void setSerializedObject(SerializedObjectPtr obj)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _serializedObject.swap(obj);
            _newSerializedObject = true;
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
        mutable std::mutex _mutex;
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
         * Set the attribute of the named object with the given args
         */
        bool set(std::string name, std::string attrib, std::vector<Value> args)
        {
            if (name == _name || name == SPLASH_ALL_PAIRS)
                setAttribute(attrib, args);
            else if (_objects.find(name) != _objects.end())
                _objects[name]->setAttribute(attrib, args);
        }

        /**
         * Set an object from its serialized form
         */
        void setFromSerializedObject(const std::string name, const SerializedObjectPtr obj)
        {
            if (_objects.find(name) != _objects.end() && std::dynamic_pointer_cast<BufferObject>(_objects[name]).get() != nullptr)
                std::dynamic_pointer_cast<BufferObject>(_objects[name])->setSerializedObject(obj);
        }

    protected:
        std::map<std::string, BaseObjectPtr> _objects;
};

typedef std::shared_ptr<RootObject> RootObjectPtr;

} // end of namespace

#endif // CORETYPES_H
