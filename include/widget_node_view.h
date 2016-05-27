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

#ifndef SPLASH_WIDGET_NODE_VIEW_H
#define SPLASH_WIDGET_NODE_VIEW_H

#include "./widgets.h"

namespace Splash
{

/*************/
class GuiNodeView : public GuiWidget
{
    public:
        GuiNodeView(std::string name) : GuiWidget(name) {}
        void render();
        std::string getClickedNode() {return _clickedNode;}
        int updateWindowFlags();

    private:
        bool _isHovered {false};
        std::string _clickedNode {""};
        std::string _sourceNode {""};

        // Node render settings
        std::vector<int> _nodeSize {160, 30};
        std::vector<int> _viewSize {640, 320};
        std::vector<int> _viewShift {0, 0};
        std::map<std::string, std::vector<float>> _nodePositions;
        
        std::map<std::string, std::vector<std::string>> getObjectLinks();
        std::map<std::string, std::string> getObjectTypes();
        void renderNode(std::string name);
};

} // end of namespace

#endif
