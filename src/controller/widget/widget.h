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
 * @widget.h
 * The splash main widget class
 */

#ifndef SPLASH_WIDGETS_H
#define SPLASH_WIDGETS_H

#include <deque>
#include <functional>
#include <imgui.h>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>

#include "./core/constants.h"

#include "./controller/controller.h"
#include "./core/attribute.h"
#include "./image/image.h"

namespace Splash
{
class Gui;
class Scene;

namespace SplashImGui
{

/**
 * Displays a file selector
 * \param label Label for the file selector window
 * \param path Path to the directory to parse
 * \param cancelled If the selection is cancelled, this will be set to false
 * \param extensions Filter out all extensions except these ones. Full extensions should be specified, i.e {{".bmp"}}
 * \param showNormalFiles If false, only show directories
 * \param newFile If true, a new file will be created based on the entered file name
 * \return Return true if a selection was done or cancelled, false if the window should remain open
 */
bool FileSelector(const std::string& label, std::string& path, bool& cancelled, const std::vector<std::string>& extensions, bool showNormalFiles = true, bool newFile = false);

/**
 * Utility function used by FileSelector
 * \param path Path to parse
 * \param list File and directory list will be put in there
 * \param extensions Filter out all extensions except these ones. Full extensions should be specified, i.e {{".bmp"}}
 * \param showNormalFiles If false, only show directories
 * \return Return true if the path is a directory
 */
bool FileSelectorParseDir(const std::string& path, std::vector<std::string>& list, const std::vector<std::string>& extensions, bool showNormalFiles);

/**
 * Utility method to call InputText on a std::string
 * \param label Label for the InputText
 * \param str String to fill
 * \param flags ImGui flags
 * \return Return true if the input has been validated
 */
bool InputText(const char* label, std::string& str, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
}

/*************/
class GuiWidget : public ControllerObject
{
  public:
    GuiWidget(Scene* scene, const std::string& name = "");
    virtual ~GuiWidget() override = default;

    /**
     * Render the widget
     */
    virtual void render() override {}

    /**
     * Get the window flags as updated by the widget
     */
    virtual int updateWindowFlags() { return 0; }

    /**
     * Send joystick information to the widget
     * \param axes Vector containing axes values
     * \param buttons Vector containingi buttons statuses
     */
    virtual void setJoystick(const std::vector<float>& /*axes*/, const std::vector<uint8_t>& /*buttons*/) {}

  protected:
    Scene* _scene;
    std::string _fileSelectorTarget{""};
    std::list<std::string> _hiddenAttributes{"savable", "lockedAttributes", "buffer"};

    /**
     * Draws the widgets for the attributes of the given object
     * and sends the appriorate messages to the World
     */
    void drawAttributes(const std::string& objName, const std::unordered_map<std::string, Values>& attributes);

    /**
     * Get the corresponding UTF-8 character for a given keyboard key,
     * based on the US layout. Basically it converts US layout keys to
     * the layout currently in use.
     * \param key Key to look for, as the ASCII character from the US layout
     * \return Return the keyboard key in the current locale
     */
    const char* getLocalKeyName(char key);
};

} // end of namespace

#endif // SPLASH_WIDGETS_H
