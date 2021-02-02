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
 * @widget_node_view.h
 * The scene graph widget
 */

#ifndef SPLASH_WIDGET_NODE_VIEW_H
#define SPLASH_WIDGET_NODE_VIEW_H

#include "./widget.h"

#include <array>
#include <string>
#include <vector>

#include <imgui.h>

namespace Splash
{

/*************/
class GuiNodeView : public GuiWidget
{
  public:
    GuiNodeView(Scene* scene, const std::string& name);
    void render() final;
    std::string getClickedNode() { return _clickedNode; }
    int updateWindowFlags() final;

  private:
    bool _isHovered{false};
    std::string _clickedNode{""};
    std::string _sourceNode{""};
    std::string _capturedNode{""};
    std::vector<std::string> _objectTypes{};

    // Node render settings
    bool _viewNonSavableObjects{false};
    std::vector<int> _nodeSize{160, 32};
    std::vector<int> _viewShift{0, 0};
    std::map<std::string, std::vector<float>> _nodePositions;

    // Temporary variables used for ImGui
    bool _firstRender{true};
    int _comboObjectIndex{0};
    ImVec2 _graphSize{0, 0};

    /**
     * Render the given node
     * \param name Node name
     */
    void renderNode(const std::string& name);

    /**
     * Initialize node positions
     * \param objectNames Names of the objects nodes to initialize
     */
    void initializeNodePositions(const std::vector<std::string>& objectNames);
};

} // end of namespace

#endif
