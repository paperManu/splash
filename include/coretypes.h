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

#define SPLASH_GL_CONTEXT_VERSION_MAJOR 3
#define SPLASH_GL_CONTEXT_VERSION_MINOR 3
#define SPLASH_GL_DEBUG false
#define SPLASH_SWAP_INTERVAL 1
#define SPLASH_SAMPLES 4
#define SPLASH_MAX_THREAD 4

#include <map>
#include <memory>
#include <string>
#include <GLFW/glfw3.h>

#include "config.h"
#include "log.h"

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

    private:
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
        AttributeFunctor(std::function<void(std::vector<Value>)> func) {_func = func;}
        bool operator()(std::vector<Value> args) {_func(args);}

    private:
        std::function<void(std::vector<Value>)> _func;
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
         * Update the content of the object
         */
        virtual void update() {}

    protected:
        unsigned long _id;
        std::string _type {"baseobject"};
        std::map<std::string, AttributeFunctor> _attribFunctions;
};

typedef std::shared_ptr<BaseObject> BaseObjectPtr;

} // end of namespace

#endif // CORETYPES_H
