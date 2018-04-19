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

bool FileSelectorParseDir(std::string& path, std::vector<FilesystemFile>& list, const std::vector<std::string>& extensions, bool showNormalFiles);
bool FileSelector(const std::string& label, std::string& path, bool& cancelled, const std::vector<std::string>& extensions, bool showNormalFiles = true);
}

/*************/
class GuiWidget : public ControllerObject
{
  public:
    GuiWidget(Scene* scene, std::string name = "");
    virtual ~GuiWidget() override {}
    virtual void render() override {}
    virtual int updateWindowFlags() { return 0; }
    virtual void setJoystick(const std::vector<float>& /*axes*/, const std::vector<uint8_t>& /*buttons*/) {}
    void setScene(Scene* scene) { _scene = scene; }

  protected:
    std::string _name{""};
    Scene* _scene;
    std::string _fileSelectorTarget{""};

    /**
     * Draws the widgets for the attributes of the given object
     * and sends the appriorate messages to the World
     */
    void drawAttributes(const std::string& objName, const std::unordered_map<std::string, Values>& attributes);
};

} // end of namespace

#endif // SPLASH_WIDGETS_H
