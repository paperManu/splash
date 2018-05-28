/*
 * Copyright (C) 2013 Emmanuel Durand
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

#include <atomic>
#include <deque>
#include <list>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "./config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./core/graph_object.h"
#include "./graphics/object.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"

namespace Splash
{

/*************/
//! Window class, holding the GL context
class Window : public GraphObject
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Window(RootObject* root);

    /**
     * \brief Destructor
     */
    ~Window() final;

    /**
     * No copy constructor, but a move one
     */
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /**
     * \brief Get grabbed character (not necesserily a specific key)
     * \param win GLFW window which grabbed the input
     * \param codepoint Character code
     * \return Return the number of characters in the queue before calling this method
     */
    static int getChars(GLFWwindow*& win, unsigned int& codepoint);

    /**
     * \brief Get the next grabbed key in the queue
     * \param win GLFW window which grabbed the key
     * \param key Key code
     * \param action Action grabbed (press, release)
     * \param mods Key modifier
     * \return Return the number of actions in the queue before calling this method
     */
    static int getKeys(GLFWwindow*& win, int& key, int& action, int& mods);

    /**
     * \brief Get the grabbed mouse action
     * \param win GLFW window which grabbed the key
     * \param btn Mouse button number
     * \param action Mouse action detected (press, release)
     * \param mods Key modifier
     * \return Return the number of actions in the queue before calling this method
     */
    static int getMouseBtn(GLFWwindow*& win, int& btn, int& action, int& mods);

    /**
     * \brief Get the mouse position
     * \param win GLFW window the mouse hovers
     * \param xpos X position
     * \param ypos Y position
     */
    static void getMousePos(GLFWwindow*& win, int& xpos, int& ypos);

    /**
     * \brief Get the mouse wheel movement
     * \param win GLFW window the mouse hovers
     * \param xoffset X offset of the wheel
     * \param yoffset Y offset of the wheel
     * \return Return the number of actions in the queue before calling this method
     */
    static int getScroll(GLFWwindow*& win, double& xoffset, double& yoffset);

    /**
     * \brief Get the list of paths dropped onto any window
     * \return Return the list of paths
     */
    static std::vector<std::string> getPathDropped();

    /**
     * \brief Get the quit flag status
     * \return Return the quit flag status
     */
    static int getQuitFlag() { return _quitFlag; }

    /**
     * \brief Check whether the window is initialized
     * \return Return true if the window is initialized
     */
    bool isInitialized() const { return _isInitialized; }

    /**
     * \brief Check whether the given GLFW window is related to this object
     * \return Return true if the GLFW window is held by this object
     */
    bool isWindow(GLFWwindow* w) const { return (w == _window->get() ? true : false); }

    /**
     * \brief Try to link the given GraphObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkTo(const std::shared_ptr<GraphObject>& obj) final;

    /**
     * \brief Try to unlink the given GraphObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkFrom(const std::shared_ptr<GraphObject>& obj) final;

    /**
     * \brief Render this window to screen
     */
    void render() final;

    /**
     * \brief Hide / show cursor
     * \param visibility Desired visibility
     */
    void showCursor(bool visibility);

    /**
     * \brief Set a new texture to draw
     * \param tex Target texture
     */
    void setTexture(const std::shared_ptr<Texture>& tex);

    /**
     * \brief Unset a new texture to draw
     * \param tex Target texture
     */
    void unsetTexture(const std::shared_ptr<Texture>& tex);

    /**
     * \brief Swap the back and front buffers
     */
    void swapBuffers();

  private:
    bool _isInitialized{false};
    std::shared_ptr<GlWindow> _window;
    int _screenId{-1};
    bool _withDecoration{true};
    int _windowRect[4];
    bool _resized{true};
    bool _srgb{true};
    float _gammaCorrection{2.2f};
    Values _layout{0, 0, 0, 0};
    int _swapInterval{1};

    // Swap synchronization test
    bool _swapSynchronizationTesting{false};
    glm::vec4 _swapSynchronizationColor{0.0, 0.0, 0.0, 1.0};

    static std::atomic_int _swappableWindowsCount;

    // Offscreen rendering related objects
    GLuint _renderFbo{0};
    GLuint _readFbo{0};
    std::shared_ptr<Texture_Image> _depthTexture{nullptr};
    std::shared_ptr<Texture_Image> _colorTexture{nullptr};
    GLsync _renderFence{nullptr};

    std::shared_ptr<Object> _screen;
    std::shared_ptr<Object> _screenGui;
    glm::dmat4 _viewProjectionMatrix;
    std::list<std::weak_ptr<Texture>> _inTextures;
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
     * \brief Input callbacks
     */
    static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow* win, unsigned int codepoint);
    static void mouseBtnCallback(GLFWwindow* win, int button, int action, int mods);
    static void mousePosCallback(GLFWwindow* win, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* win, double xoffset, double yoffset);
    static void pathdropCallback(GLFWwindow* win, int count, const char** paths);
    static void closeCallback(GLFWwindow* win);

    /**
     * \brief Set FBOs up
     */
    void setupFBOs();
    void setupReadFBO();

    /**
     * \brief Register new attributes
     */
    void registerAttributes();

    /**
     * \brief Set up the user events callbacks
     */
    void setEventsCallbacks();

    /**
     * \brief Set up the projection surface
     * \return Return true if all is well
     */
    bool setProjectionSurface();

    /**
     * \brief Set whether the window has decorations
     * \param hasDecoration Desired decoration status
     */
    void setWindowDecoration(bool hasDecoration);

    /**
     * \brief Update the swap interval. Call this when the _swapInterval has been changed
     * \param swapInterval Desired swap interval
     */
    void updateSwapInterval(int swapInterval);

    /**
     * \brief Update the window size and position to reflect its attributes
     */
    void updateWindowShape();
};

} // end of namespace

#endif // SPLASH_WINDOW_H
