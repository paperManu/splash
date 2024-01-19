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
 * @rendering_context.h
 * Class holding the OpenGL context
 */

#ifndef SPLASH_RENDERING_CONTEXT_H
#define SPLASH_RENDERING_CONTEXT_H

#include <array>
#include <memory>
#include <mutex>
#include <string_view>

#include "./core/constants.h"
#include "./utils/log.h"

namespace Splash
{

namespace gfx
{
struct PlatformVersion
{
    std::string name;
    uint32_t major{0};
    uint32_t minor{0};
    uint32_t patch{0};

    [[nodiscard]] const std::string toString() const { return name + " " + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch); }
};
} // namespace gfx

/*************/
class RenderingContext
{
  public:
    /**
     * Constructor
     * \param name Rendering context name
     * \param platformVersion Rendering platform informations
     */
    RenderingContext(std::string_view name, const gfx::PlatformVersion& platformVersion);

    /**
     * Constructor, to create a context with resources shared with another
     * \param name Rendering context name
     * \param context Rendering context to share resources with
     */
    RenderingContext(std::string_view name, RenderingContext* context);

    /**
     * Destructor
     */
    ~RenderingContext();

    /**
     * Create a shared rendering context from this one
     * \return Return a unique pointer to the newly created context
     */
    std::unique_ptr<RenderingContext> createSharedContext(std::string_view name);

    /**
     * Get the monitor this context is set full screen to
     * \return Return the index of the full screen monitor, or -1 if the context is windowed
     */
    int32_t getFullscreenMonitor() const;

    /**
     * Get the pointer to the GLFW window
     * \return Return the pointer to the GLFW window
     */
    GLFWwindow* getGLFWwindow() const { return _window; }

    /**
     * Get the pointer to the main rendering context
     * \return Return the main rendering context
     */
    RenderingContext* getMainContext() const { return _mainContext; }

    /**
     * Get the names of the connected monitors
     * \return Return a vector of the connected monitors
     */
    std::vector<std::string> getMonitorNames() const;

    /**
     * Get the position and size of the context
     * \return Return an array containing the x and y positions, width and height
     */
    std::array<int32_t, 4> getPositionAndSize() const;

    /**
     * Get whether this GLFW window is the current context
     * \return Return true if this window is the current context
     */
    bool isCurrentContext() const { return _window == glfwGetCurrentContext(); }

    /**
     * Check whether the context is valid and usable
     * \return Return true if the context is correctly initialized
     */
    bool isInitialized() const { return _window != nullptr; }

    /**
     * Return whether the underlying platform is Wayland
     * \return Return true if the platform is Wayland
     */
    bool isPlatformWayland() const { return _glfwPlatformWayland; }

    /**
     * Return whether the context is visible
     * \return Return true if the context is visible
     */
    bool isVisible() const { return glfwGetWindowAttrib(_window, GLFW_VISIBLE); }

    /**
     * Release the context
     */
    void releaseContext();

    /**
     * Set the context of this window as current
     */
    void setAsCurrentContext();

    /**
     * Set whether the cursor is visible or not
     * \param visible If true, the cursor is drawn
     */
    void setCursorVisible(bool visible);

    /**
     * Set the decorations status
     * \param active If true, activates the decoration
     */
    void setDecorations(bool active);

    /**
     * Set the keyboard, mouse, scroll, etc. events callbacks
     */
    void setEventsCallbacks();

    /**
     * Set the context as full screen, or not
     * \param index Monitor index, -1 to set as windowed
     * \return Return true if the full screen state has been changed successfully
     */
    bool setFullscreenMonitor(int32_t index);

    /**
     * Set the position and size of the context
     * \param posAndSize An array containg the x and y position, width and height
     */
    void setPositionAndSize(const std::array<int32_t, 4>& posAndSize);

    /**
     * Set the swap interval for the context.
     * The context must be activated before calling this.
     * \param interval Swap interval (in frames), 0 for no interval, -1 to try our best to match vsync
     */
    void setSwapInterval(int32_t interval);

    /**
     * Show the context
     */
    void show();

    /**
     * Swap front an back buffers
     */
    void swapBuffers();

  private:
    static bool _glfwInitialized;
    static bool _glfwPlatformWayland;

    std::string _name{};
    GLFWwindow* _previouslyActiveWindow{nullptr};
    GLFWwindow* _window{nullptr};
    RenderingContext* _mainContext{nullptr};

    static std::vector<GLFWmonitor*> _monitors;
    bool _isContextActive{false};

    /**
     * Get the list of monitors
     * \return Return the list of connected monitors
     */
    std::vector<GLFWmonitor*> getMonitorList();

    /**
     * Callback for GLFW errors
     * \param code Error code
     * \param msg Associated error message
     */
    static void glfwErrorCallback(int /*code*/, const char* msg) { Log::get() << Log::ERROR << "glfwErrorCallback - " << msg << Log::endl; }

    /**
     * Callback for connection/disconnection of monitors
     * \param monitor Monitor which changed state
     * \param even Event type
     */
    static void glfwMonitorCallback(GLFWmonitor* monitor, int event);
};

} // namespace Splash

#endif // SPLASH_GL_WINDOW_H
