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

#include <memory>
#include <string>
#include <GLFW/glfw3.h>

#include "config.h"

namespace Splash
{

/*************/
class GlWindow
{
    public:
        /**
         * Constructors
         */
        GlWindow(GLFWwindow* w)
        {
            _window = w;
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

    private:
        GLFWwindow* _window {nullptr};
};

typedef std::shared_ptr<GlWindow> GlWindowPtr;

/*************/
class BaseObject
{
    public:
        unsigned long getId() const {return _id;}
        void setId(unsigned long id) {_id = id;}
        std::string getType() const {return _type;}

    protected:
        unsigned long _id;
        std::string _type {"baseobject"};
};

typedef std::shared_ptr<BaseObject> BaseObjectPtr;

} // end of namespace

#endif // CORETYPES_H
