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
 * @widget_global_view.h
 * The global view widget, to calibrate cameras
 */

#ifndef SPLASH_WIDGET_WARP_H
#define SPLASH_WIDGET_WARP_H

#include "./warp.h"
#include "./widget.h"

namespace Splash
{

/*************/
class GuiWarp : public GuiWidget
{
  public:
    GuiWarp(Scene* scene, const std::string& name)
        : GuiWidget(scene, name)
    {
    }
    void render() final;
    void setScene(Scene* scene) { _scene = scene; }
    int updateWindowFlags() final;

  private:
    bool _noMove{false};

    int _currentWarp{0};
    std::string _currentWarpName{""};
    int _currentControlPointIndex{0};
    glm::vec2 _previousMousePos;

    void processKeyEvents(const std::shared_ptr<Warp>& warp);
    void processMouseEvents(const std::shared_ptr<Warp>& warp, int warpWidth, int warpHeight);

    void updateControlPoints(const std::string& warpName, const Values& controlPoints);
};

} // end of namespace

#endif
