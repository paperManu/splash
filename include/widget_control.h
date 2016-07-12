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

#ifndef SPLASH_WIDGET_CONTROL_H
#define SPLASH_WIDGET_CONTROL_H

#include "./widget.h"

namespace Splash
{

/*************/
class GuiControl : public GuiWidget
{
    public:
        GuiControl(std::weak_ptr<Scene> scene, std::string name)
            : GuiWidget(scene, name) {}
        void render();
        int updateWindowFlags();

    private:
        std::shared_ptr<GuiWidget> _nodeView;
        int _targetIndex {-1};
        std::string _targetObjectName {};
};

} // end of namespace

#endif
