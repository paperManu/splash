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
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @controller_gui.h
 * The Gui base class
 */

#ifndef SPLASH_GUI_H
#define SPLASH_GUI_H

#define SPLASH_GLV_TEXTCOLOR 1.0, 1.0, 1.0
#define SPLASH_GLV_FONTSIZE 8.0
#define SPLASH_GLV_FONTWIDTH 2.0

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#include <atomic>
#include <deque>
#include <functional>
#include <imgui.h>
#include <memory>

#include "./config.h"

#if HAVE_GPHOTO
#include "./controller/colorcalibrator.h"
#endif
#include "./controller/widget/widget.h"
#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./graphics/camera.h"
#include "./graphics/framebuffer.h"
#include "./userinput/userinput.h"

namespace Splash
{

class Scene;

/*************/
class Gui : public ControllerObject
{
  public:
    /**
     * \brief Constructor
     * \param w Window to display the gui
     * \param s Root scene
     */
    Gui(std::shared_ptr<GlWindow> w, RootObject* s);

    /**
     * \brief Destructor
     */
    ~Gui() final;

    /**
     * No copy constructor, but a move one
     */
    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;

    /**
     * \brief Get pointers to this gui textures
     * \return Return shared pointers to the rendered textures
     */
    std::shared_ptr<Texture> getTexture() const { return _fbo->getColorTexture(); }

    /**
     * \brief Check wether it is initialized
     * \return Return true if the gui is initialized
     */
    bool isInitialized() const { return _isInitialized; }

    /**
     * \brief Forward a unicode char event
     * \param unicodeChar Unicode character to forward to the gui
     */
    void unicodeChar(unsigned int unicodeChar);

    /**
     * \brief Forward joystick state
     * \param axes Axes state
     * \param buttons Buttons state
     */
    void setJoystick(const std::vector<float>& axes, const std::vector<uint8_t>& buttons);

    /**
     * \brief Forward a key event
     * \param key Key code
     * \param action Action to applied to the key
     * \param mods Modifier keys
     */
    void key(int key, int action, int mods);

    /**
     * \brief Forward mouse position
     * \param xpos X position
     * \param ypos Y position
     */
    void mousePosition(int xpos, int ypos);

    /**
     * \brief Forward mouse buttons
     * \param btn Button
     * \param action Action applied to the button
     * \param mods Key modifier
     */
    void mouseButton(int btn, int action, int mods);

    /**
     * \brief Forward mouse scroll
     * \param xoffset Scroll along X axis
     * \param yoffset Scroll along Y axis
     */
    void mouseScroll(double xoffset, double yoffset);

    /**
     * \brief Try to link the given BaseObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkTo(const std::shared_ptr<BaseObject>& obj) final;

    /**
     * \brief Try to unlink the given BaseObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkFrom(const std::shared_ptr<BaseObject>& obj) final;

    /**
     * \brief Render this gui
     */
    void render();

    /**
     * \brief Specify the configuration path (as loaded by World)
     * \param path Configuration path
     */
    void setConfigFilePath(const std::string& path) { _configurationPath = path; }

    /**
     * \brief Set joysticks state
     * \param state Joysticks state
     */
    void setJoystickState(const std::vector<UserInput::State>& state);

    /**
     * \brief Set the keyboard state
     * \param state Keyboard state
     */
    void setKeyboardState(const std::vector<UserInput::State>& state);

    /**
     * \brief Set the mouse state
     * \param state Mouse state
     */
    void setMouseState(const std::vector<UserInput::State>& state);

    /**
     * \brief Set the resolution of the gui
     * \param width Width
     * \param height Height
     */
    void setOutputSize(int width, int height);

  private:
    bool _isInitialized{false};
    std::shared_ptr<GlWindow> _window;
    Scene* _scene;

    std::unique_ptr<Framebuffer> _fbo{nullptr};
    float _width{512}, _height{512};
    bool _resized{false};
    int _initialGuiPos[2]{16, 16}; //!< Gui position at startup

    // GUI specific camera
    std::shared_ptr<Camera> _guiCamera{nullptr};
    std::shared_ptr<Texture_Image> _splashLogo{nullptr};

    // ImGUI related attributes
    static GLuint _imFontTextureId;
    static GLuint _imGuiShaderHandle, _imGuiVertHandle, _imGuiFragHandle;
    static GLint _imGuiTextureLocation;
    static GLint _imGuiProjMatrixLocation;
    static GLint _imGuiPositionLocation;
    static GLint _imGuiUVLocation;
    static GLint _imGuiColorLocation;
    static GLuint _imGuiVboHandle, _imGuiElementsHandle, _imGuiVaoHandle;
    static size_t _imGuiVboMaxSize;

    // ImGUI objects
    ImGuiWindowFlags _windowFlags{0};
    std::vector<std::shared_ptr<GuiWidget>> _guiWidgets;

    // Gui related attributes
    std::string _configurationPath;
    std::string _projectPath;
    bool _firstRender{true};
    bool _mouseHoveringWindow{false};
    bool _isVisible{false};
    bool _wasVisible{true};
    bool _wireframe{false};
    bool _blendingActive{false};
    bool _showAbout{false};

    /**
     * \brief Initialize ImGui
     * \brief width Default width
     * \brief height Default height
     */
    void initImGui(int width, int height);

    /**
     * \brief Initialize widgets
     */
    void initImWidgets();

    /**
     * \brief ImGui render function
     * \param draw_data Data sent by ImGui for drawing
     */
    static void imGuiRenderDrawLists(ImDrawData* draw_data);

    /**
     * Actions
     */

    /**
     * \brief Activate the cameras lookup tables
     */
    void activateLUT();

    /**
     * \brief Launch calibration of the camera response function
     */
    void calibrateColorResponseFunction();

    /**
     * \brief Calibrate projectors colors
     */
    void calibrateColors();

    /**
     * \brief Compute projectors blending
     */
    void computeBlending(bool once = false);

    /**
     * \brief Get the clipboard
     * \return Return a pointer to the text
     */
    static const char* getClipboardText(void* userData);

    /**
     * \brief Set the clipboard
     * \param text Text to set the clipboard to
     */
    static void setClipboardText(void* userData, const char* text);

    /**
     * \brief Copy camera parameters from the specified configuration file to the current configuration
     */
    void copyCameraParameters();

    /**
     * \brief Load the specified configuration
     */
    void loadConfiguration();

    /**
     * Load the icon image
     */
    void loadIcon();

    /**
     * \brief Load a partial configuration, which does not contain calibration or projection parameters
     */
    void loadProject();

    /**
     * Render the splash screen
     */
    void renderSplashScreen();

    /**
     * \brief Save the configuration to the specified file
     */
    void saveConfiguration();

    /**
     * \brief Save a partial configuration, which does not contain calibration or projection parameters
     */
    void saveProject();

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_GUI_H
