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
#define SPLASH_GL_CONTEXT_VERSION_MINOR 2
#define SPLASH_GL_DEBUG false
#define SPLASH_SWAP_INTERVAL 0

#include <map>
#include <memory>
#include <string>
#include <GLFW/glfw3.h>

#include "config.h"
#include "log.h"

namespace Splash
{

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
struct AttributeFunctor
{
    public:
        AttributeFunctor() {}
        AttributeFunctor(std::function<void(std::vector<float>)> func) {_func = func;}
        bool operator()(std::vector<float> args) {_func(args);}

    private:
        std::function<void(std::vector<float>)> _func;
};

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
         * Register modifiable attributes
         */
        virtual void registerAttributes() {}

        /**
         * Set the specified attribute
         */
        bool setAttribute(std::string attrib, std::vector<float> args)
        {
            if (_attribFunctions.find(attrib) == _attribFunctions.end())
                return false;
            return _attribFunctions[attrib](args);
        }

    protected:
        unsigned long _id;
        std::string _type {"baseobject"};
        std::map<std::string, AttributeFunctor> _attribFunctions;
};

typedef std::shared_ptr<BaseObject> BaseObjectPtr;

} // end of namespace

#endif // CORETYPES_H
