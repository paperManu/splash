/*
 * Copyright (C) 2015 Splash authors
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
 * @gl_window.h
 * Class holding the OpenGL context
 */

#ifndef SPLASH_GL_WINDOW_H
#define SPLASH_GL_WINDOW_H

#include <mutex>

#include "./core/constants.h"

namespace Splash
{

/*************/
class GlWindow
{
  public:
    /**
     * Constructor
     * \param w Pointer to an existing GLFWwindow
     * \param mainWindow Pointer to an existing GLFWwindow which is shared with w
     */
    GlWindow() = delete;
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
     * \return Return the pointer to the GLFW window
     */
    GLFWwindow* get() const { return _window; }

    /**
     * Get the pointer to the main GLFW window
     * \return Return the main GLFW window
     */
    GLFWwindow* getMainWindow() const { return _mainWindow; }

    /**
     * Get whether this GLFW window is the current context
     * \return Return true if this window is the current context
     */
    bool isCurrentContext() const { return _window == glfwGetCurrentContext(); }

    /**
     * Set the context of this window as current
     */
    void setAsCurrentContext()
    {
        _previousWindow = glfwGetCurrentContext();
        if (_previousWindow == _window)
            return;
        _mutex.lock();
        glfwMakeContextCurrent(_window);
    }

    /**
     * Release the context
     */
    void releaseContext()
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
    std::mutex _mutex;
    GLFWwindow* _previousWindow{nullptr};
    GLFWwindow* _window{nullptr};
    GLFWwindow* _mainWindow{nullptr};
};

} // namespace Splash

#endif // SPLASH_GL_WINDOW_H
