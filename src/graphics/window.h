/*
 * Copyright (C) 2013 Splash authors
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
 * @window.h
 * The Window class
 */

#ifndef SPLASH_WINDOW_H
#define SPLASH_WINDOW_H

#include <array>
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/graph_object.h"
#include "./graphics/api/window_gfx_impl.h"
#include "./graphics/object.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"

namespace Splash
{

class Gui;
class RenderingContext;

/*************/
//! Window class, holding the GL context
class Window final : public GraphObject
{
    friend RenderingContext;

  public:
    /**
     * Constructor
     * \param root Root object
     */
    Window(RootObject* root);

    /**
     * Destructor
     */
    ~Window() final;

    /**
     * Constructors/operators
     */
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    /**
     * Get grabbed character (not necesserily a specific key)
     * \param win GLFW window which grabbed the input
     * \param codepoint Character code
     * \return Return the number of characters in the queue before calling this method
     */
    static int getChar(GLFWwindow*& win, unsigned int& codepoint);

    /**
     * Get the next grabbed key in the queue
     * \param win GLFW window which grabbed the key
     * \param key Key code
     * \param action Action grabbed (press, release)
     * \param mods Key modifier
     * \return Return the number of actions in the queue before calling this method
     */
    static int getKey(GLFWwindow*& win, int& key, int& action, int& mods);

    /**
     * Get the grabbed mouse action
     * \param win GLFW window which grabbed the key
     * \param btn Mouse button number
     * \param action Mouse action detected (press, release)
     * \param mods Key modifier
     * \return Return the number of actions in the queue before calling this method
     */
    static int getMouseBtn(GLFWwindow*& win, int& btn, int& action, int& mods);

    /**
     * Get the mouse position
     * \param win GLFW window the mouse hovers
     * \param xpos X position
     * \param ypos Y position
     */
    static void getMousePos(GLFWwindow*& win, int& xpos, int& ypos);

    /**
     * Get the mouse wheel movement
     * \param win GLFW window the mouse hovers
     * \param xoffset X offset of the wheel
     * \param yoffset Y offset of the wheel
     * \return Return the number of actions in the queue before calling this method
     */
    static int getScroll(GLFWwindow*& win, double& xoffset, double& yoffset);

    /**
     * Get the list of paths dropped onto any window
     * \return Return the list of paths
     */
    static std::vector<std::string> getPathDropped();

    /**
     * Get the quit flag status
     * \return Return the quit flag status
     */
    static int getQuitFlag() { return _quitFlag; }

    /**
     * Get the timestamp
     * \return Return the timestamp in us
     */
    virtual int64_t getTimestamp() const final { return _frontBufferTimestamp; }

    /**
     * Check whether the window is initialized
     * \return Return true if the window is initialized
     */
    bool isInitialized() const { return _isInitialized; }

    /**
     * Check whether the given rendering context is related to this object
     * \return Return true if the rendering context is held by this object
     */
    bool isWindow(const GLFWwindow* glfwWindow) const { return (_gfxImpl->getRenderingContext()->getGLFWwindow() == glfwWindow ? true : false); }

    /**
     * Render this window to screen
     */
    void render() final;

    /**
     * Hide / show cursor
     * \param visibility Desired visibility
     */
    void showCursor(bool visibility);

    /**
     * Set a new texture to draw
     * \param tex Target texture
     */
    void setTexture(const std::shared_ptr<Texture>& tex);

    /**
     * Unset a new texture to draw
     * \param tex Target texture
     */
    void unsetTexture(const std::shared_ptr<Texture>& tex);

    /**
     * Swap the back and front buffers
     */
    void swapBuffers();

  protected:
    /**
     * Try to link the given GraphObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkIt(const std::shared_ptr<GraphObject>& obj) final;

    /**
     * Try to unlink the given GraphObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkIt(const std::shared_ptr<GraphObject>& obj) final;

    /**
     * Input callbacks
     */
    static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow* win, unsigned int codepoint);
    static void mouseBtnCallback(GLFWwindow* win, int button, int action, int mods);
    static void mousePosCallback(GLFWwindow* win, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* win, double xoffset, double yoffset);
    static void pathdropCallback(GLFWwindow* win, int count, const char** paths);
    static void closeCallback(GLFWwindow* win);

  private:
    std::unique_ptr<gfx::WindowGfxImpl> _gfxImpl;
    bool _isInitialized{false};

    int64_t _backBufferTimestamp{0};
    int64_t _frontBufferTimestamp{0};
    int64_t _presentationDelay{0};

    bool _withDecoration{true};
    std::array<int32_t, 4> _windowRect{};
    bool _resized{true};
    Values _layout{0, 1, 2, 3};
    int _swapInterval{1};
    bool _guiOnly{false};

    // Swap synchronization test
    bool _swapSynchronizationTesting{false};
    glm::vec4 _swapSynchronizationColor{0.0, 0.0, 0.0, 1.0};

    static int _swappableWindowsCount;

    // Offscreen rendering related objects
    bool _renderTextureUpdated{false};

    std::shared_ptr<Object> _screen;
    std::shared_ptr<Object> _screenGui;
    glm::dmat4 _viewProjectionMatrix;
    std::vector<std::weak_ptr<Texture>> _inTextures;
    std::shared_ptr<Gui> _gui{nullptr};
    std::shared_ptr<Texture> _guiTexture{nullptr}; // The gui has its own texture

    static std::mutex _callbackMutex;
    static std::deque<std::pair<GLFWwindow*, std::vector<int>>> _keys;      // Input keys queue
    static std::deque<std::pair<GLFWwindow*, unsigned int>> _chars;         // Input keys queue
    static std::deque<std::pair<GLFWwindow*, std::vector<int>>> _mouseBtn;  // Input mouse buttons queue
    static std::pair<GLFWwindow*, std::vector<double>> _mousePos;           // Input mouse position
    static std::deque<std::pair<GLFWwindow*, std::vector<double>>> _scroll; // Input mouse scroll queue
    static std::vector<std::string> _pathDropped;                           // Filepath drag&dropped
    static std::atomic_bool _quitFlag;                                      // Grabs close window events

    /**
     * Register new attributes
     */
    void registerAttributes();

    /**
     * Set up the user events callbacks
     */
    void setEventsCallbacks();

    /**
     * Set the monitor for full screen display
     * \param index Monitor index, -1 to set as windowed
     */
    void setFullscreenMonitor(int32_t index);

    /**
     * Set whether the window has decorations
     * \param hasDecoration Desired decoration status
     */
    void setWindowDecoration(bool hasDecoration);

    /**
     * Snap the window to the borders
     * \param distance Snap distance
     * \return Return true if the window has been snapped to a border
     */
    bool snapWindow(int distance);

    /**
     * Update the swap interval. Call this when the _swapInterval has been changed
     * \param swapInterval Desired swap interval
     */
    void updateSwapInterval(int swapInterval);

    /**
     * Update the window size and position to reflect its attributes
     */
    void updateWindowShape();
};

} // namespace Splash

#endif // SPLASH_WINDOW_H
