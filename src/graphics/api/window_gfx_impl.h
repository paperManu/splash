/*
 * Copyright (C) 2023 Tarek Yasser
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

#ifndef SPLASH_GFX_WINDOW_GFX_IMPL
#define SPLASH_GFX_WINDOW_GFX_IMPL

#include "glm/vec4.hpp"

struct GLFWwindow;

namespace Splash
{

class Scene;
class RootObject;

namespace gfx
{

class WindowGfxImpl
{
  public:
    /**
     * Constructor
     */
    WindowGfxImpl() = default;

    /**
     * Destructor
     */
    virtual ~WindowGfxImpl() = default;

    /**
     * Setup the FBOs for this window
     * \param scene Pointer to the Scene
     * \param width FBO width
     * \param height FBO height
     */
    virtual void setupFBOs(Scene* _scene, uint32_t width, uint32_t height) = 0;

    /**
     * Clear the screen with the given color
     * \param color Clear color
     * \param clearDepth If true, also clears the depth buffer
     */
    virtual void clearScreen(glm::vec4 color, bool clearDepth = false) = 0;

    /**
     * Reset the render sync fence
     */
    virtual void resetSyncFence() = 0;

    /**
     * Setup the window for beginning rendering
     * \param windowRect Rendering position and size
     */
    virtual void beginRender(int windowRect[4]) = 0;

    /**
     * End rendering for this window
     */
    virtual void endRender() = 0;

    /**
     * Setup user data for debug messages
     * \param userData Pointer to the user data
     */
    virtual void setDebugData(const void* userData) = 0;

    /**
     * Get the GLFW window
     * \return Return a pointer to the GLFW window
     */
    virtual inline GLFWwindow* getGlfwWindow() = 0;

    /**
     * Get the main window, corresponding to the main rendering context
     * \return Return a pointer to the  main GLFW window
     */
    virtual inline GLFWwindow* getMainWindow() = 0;

    /**
     * Initialize the window
     * \param scene Root Scene
     */
    virtual void init(Scene* scene) = 0;

    /**
     * Set as current rendering context
     */
    virtual inline void setAsCurrentContext() = 0;

    /**
     * Release from begin the current rendering context
     */
    virtual inline void releaseContext() = 0;

    /**
     * Check whether this window is the current rendering context
     * \return Return true if this window is the current rendering context
     */
    virtual inline bool isCurrentContext() const = 0;

    /**
     * Update the GLFW window from another one, both having shared resources
     * \param window The window to share resources with
     */
    virtual void updateGlfwWindow(GLFWwindow* window) = 0;

    /**
     * Check whether the GLFW window exists
     * \return Return true if the contained GLFW window exists
     */
    virtual bool windowExists() const = 0;

    /**
     * Copy the given portion of rendered FBO up to the front buffer, respecting vertical sync if active
     * \param windowIndex Window index, among all of Splash windows
     * \param srgb True if the window is rendered in the sRGB color space
     * \param renderTextureUpdated True if the render textures have been updated
     * \param windowRect Position and size of the region to copy
     */
    virtual void swapBuffers(int windowIndex, bool _srgb, bool& _renderTextureUpdated, int _windowRect[4]) = 0;
};

} // namespace gfx

} // namespace Splash

#endif
