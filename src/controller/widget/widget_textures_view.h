/*
 * Copyright (C) 2018 Emmanuel Durand
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
 * @widget_textures_view.h
 * A widget to view all textures, for debugging purposes
 */

#ifndef SPLASH_WIDGET_TEXTURES_VIEW_H
#define SPLASH_WIDGET_TEXTURES_VIEW_H

#include "./widget.h"

namespace Splash
{

/*************/
class GuiTexturesView : public GuiWidget
{
  public:
    GuiTexturesView(Scene* scene, std::string name = "")
        : GuiWidget(scene, name)
    {
    }
    void render() final;
};

} // end of namespace

#endif // SPLASH_WIDGET_TEXTURES_VIEW_H
