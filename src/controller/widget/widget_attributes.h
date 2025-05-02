/*
 * Copyright (C) 2025 Splash authors
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
 * @widget_attribute.h
 * A widget to show all of an object's attributes
 */

#ifndef SPLASH_WIDGET_ATTRIBUTES_H
#define SPLASH_WIDGET_ATTRIBUTES_H

#include "./widget.h"

namespace Splash
{

/*************/
class GuiAttributes : public GuiWidget
{
  public:
    GuiAttributes(Scene* scene, const std::string& name)
        : GuiWidget(scene, name)
    {
    }
    void render() final;
    void setTargetObjectName(const std::string& name) { _targetObjectName = name; }

  private:
    std::string _targetObjectName{};
};

} // namespace Splash

#endif
