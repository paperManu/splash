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

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#include <atomic>
#include <deque>
#include <functional>
#include <imgui.h>
#include <memory>

#include "./core/constants.h"

#if HAVE_GPHOTO and HAVE_OPENCV
#include "./controller/colorcalibrator.h"
#endif
#include "./controller/widget/widget.h"
#include "./core/attribute.h"
#include "./graphics/camera.h"
#include "./graphics/framebuffer.h"
#include "./userinput/userinput.h"

namespace Splash
{

/*************/
class Gui final : public ControllerObject
{
  public:
    enum class FontType : uint8_t
    {
        Default,
        Clock
    };

    enum class MenuAction : uint8_t
    {
        None,
        OpenConfiguration,
        OpenProject,
        CopyCalibration,
        SaveConfigurationAs,
        SaveProjectAs
    };

  public:
    /**
     * Constructor
     * \param w Window to display the gui
     * \param s Root scene
     */
    Gui(std::shared_ptr<GlWindow> w, RootObject* s);

    /**
     * Destructor
     */
    ~Gui() final;

    /**
     * Constructors/operators
     */
    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;
    Gui(Gui&&) = delete;
    Gui& operator=(Gui&&) = delete;

    /**
     * Get pointers to this gui textures
     * \return Return shared pointers to the rendered textures
     */
    std::shared_ptr<Texture> getTexture() const { return _fbo->getColorTexture(); }

    /**
     * Check wether it is initialized
     * \return Return true if the gui is initialized
     */
    bool isInitialized() const { return _isInitialized; }

    /**
     * Forward a unicode char event
     * \param unicodeChar Unicode character to forward to the gui
     */
    void unicodeChar(unsigned int unicodeChar);

    /**
     * Forward joystick state
     * \param axes Axes state
     * \param buttons Buttons state
     */
    void setJoystick(const std::vector<float>& axes, const std::vector<uint8_t>& buttons);

    /**
     * Forward a key event
     * \param key Key code
     * \param action Action to applied to the key
     * \param mods Modifier keys
     */
    void key(int key, int action, int mods);

    /**
     * Forward mouse position
     * \param xpos X position
     * \param ypos Y position
     */
    void mousePosition(int xpos, int ypos);

    /**
     * Forward mouse buttons
     * \param btn Button
     * \param action Action applied to the button
     * \param mods Key modifier
     */
    void mouseButton(int btn, int action, int mods);

    /**
     * Forward mouse scroll
     * \param xoffset Scroll along X axis
     * \param yoffset Scroll along Y axis
     */
    void mouseScroll(double xoffset, double yoffset);

    /**
     * Render this gui
     */
    void render();

    /**
     * Specify the configuration path (as loaded by World)
     * \param path Configuration path
     */
    void setConfigFilePath(const std::string& path) { _configurationPath = path; }

    /**
     * Set joysticks state
     * \param state Joysticks state
     */
    void setJoystickState(const std::vector<UserInput::State>& state);

    /**
     * Set the keyboard state
     * \param state Keyboard state
     */
    void setKeyboardState(const std::vector<UserInput::State>& state);

    /**
     * Set the mouse state
     * \param state Mouse state
     */
    void setMouseState(const std::vector<UserInput::State>& state);

    /**
     * Set the resolution of the gui
     * \param width Width
     * \param height Height
     */
    void setOutputSize(int width, int height);

    /**
     * Store the window this GUI will be shown onto
     * \param window Raw pointer to the target Window
     */
    void setOutputWindow(Window* window) { _window = window; }

    /**
     * Get the window this GUI is shown onto
     * \return Return a raw pointer of the window
     */
    Window* getOutputWindow() const { return _window; }

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

  private:
    struct FontDefinition
    {
        FontType type;
        std::string filename;
        uint32_t size;
    };

    const int32_t _defaultWidth{900};
    const int32_t _defaultHeight{900};
    const float _backgroundAlpha{0.97f};

    bool _isInitialized{false};
    std::shared_ptr<GlWindow> _glWindow;
    Window* _window{nullptr};

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
    bool _showFileSelector{false};
    MenuAction _menuAction{MenuAction::None};
    ImGuiWindowFlags _windowFlags{0};
    std::map<FontType, ImFont*> _guiFonts{};
    std::vector<std::shared_ptr<GuiWidget>> _guiWidgets;
    std::vector<std::shared_ptr<GuiWidget>> _guiBottomWidgets;

    // Gui related attributes
    std::string _configurationPath;
    std::string _projectPath;
    bool _mouseHoveringWindow{false};
    bool _isVisible{false};
    bool _wasVisible{true};
    bool _wireframe{false};
    bool _blendingActive{false};
    bool _showAbout{false};
    bool _showHelp{false};
    bool _fullscreen{false};

    /**
     * Initialize ImGui
     * \param width Default width
     * \param height Default height
     */
    void initImGui(int width, int height);

    /**
     * Initialize widgets
     */
    void initImWidgets();

    /**
     * Get the corresponding UTF-8 character for a given keyboard key,
     * based on the US layout. Basically it converts US layout keys to
     * the layout currently in use.
     * \param key Key to look for, as the ASCII character from the US layout
     * \return Return the keyboard key in the current locale
     */
    const char* getLocalKeyName(char key);

    /**
     * ImGui render function
     * \param draw_data Data sent by ImGui for drawing
     */
    static void imGuiRenderDrawLists(ImDrawData* draw_data);

    /**
     * Actions
     */

    /**
     * Draw the main tabulation
     */
    void drawMainTab();

    /**
     * Draw the menu bar
     */
    void drawMenuBar();

    /**
     * Launch calibration of the camera response function
     */
    void calibrateColorResponseFunction();

    /**
     * Calibrate projectors colors
     */
    void calibrateColors();

    /**
     * Compute projectors blending
     */
    void computeBlending(bool once = false);

    /**
     * Get the clipboard
     * \return Return a pointer to the text
     */
    static const char* getClipboardText(void* userData);

    /**
     * Set the clipboard
     * \param text Text to set the clipboard to
     */
    static void setClipboardText(void* userData, const char* text);

    /**
     * Copy camera parameters from the specified configuration file to the current configuration
     * \param path Path to the configuration file to copy from
     */
    void copyCameraParameters(const std::string& path);

    /**
     * Load the icon image
     */
    void loadIcon();

    /**
     * Render the splash screen
     */
    void renderSplashScreen();

    /**
     * Render the help
     */
    void renderHelp();

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_GUI_H
