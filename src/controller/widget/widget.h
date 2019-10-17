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

#define SPLASH_GLV_TEXTCOLOR 1.0, 1.0, 1.0
#define SPLASH_GLV_FONTSIZE 8.0
#define SPLASH_GLV_FONTWIDTH 2.0

#include <deque>
#include <functional>
#include <imgui.h>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>

#include "./config.h"

#include "./core/attribute.h"
#include "./controller/controller.h"
#include "./core/coretypes.h"
#include "./image/image.h"

namespace Splash
{
class Gui;
class Scene;

namespace SplashImGui
{
struct FilesystemFile
{
    std::string filename{""};
    bool isDir{false};
};

bool FileSelectorParseDir(const std::string& path, std::vector<FilesystemFile>& list, const std::vector<std::string>& extensions, bool showNormalFiles);
bool FileSelector(const std::string& label, std::string& path, bool& cancelled, const std::vector<std::string>& extensions, bool showNormalFiles = true);

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
    virtual ~GuiWidget() override {}

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
    std::list<std::string> _hiddenAttributes{"savable"};

    /**
     * Draws the widgets for the attributes of the given object
     * and sends the appriorate messages to the World
     */
    void drawAttributes(const std::string& objName, const std::unordered_map<std::string, Values>& attributes);
};

} // end of namespace

#endif // SPLASH_WIDGETS_H
