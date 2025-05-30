/*
 * Copyright (C) 2014 Splash authors
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
 * @widget_warp.h
 * The warp view widget
 */

#ifndef SPLASH_WIDGET_WARP_H
#define SPLASH_WIDGET_WARP_H

#include "./graphics/warp.h"
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
    void update() final;
    int updateWindowFlags() final;
    std::optional<std::string> getActiveObjectName() const final { return _currentWarpName.empty() ? std::optional<std::string>() : _currentWarpName; }

  private:
    bool _noMove{false};

    bool _rendered{false};
    uint32_t _currentWarp{0};
    std::string _currentWarpName{""};
    int _currentControlPointIndex{0};
    glm::vec2 _deltaAtPicking;

    void processKeyEvents(const std::shared_ptr<Warp>& warp);
    void processMouseEvents(const std::shared_ptr<Warp>& warp, int warpWidth, int warpHeight);
};

} // namespace Splash

#endif
