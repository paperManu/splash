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
 * @widget_graph.h
 * The performance graphs widget
 */

#ifndef SPLASH_WIDGET_GRAPH_H
#define SPLASH_WIDGET_GRAPH_H

#include "./widget.h"

namespace Splash
{

/*************/
class GuiGraph : public GuiWidget
{
  public:
    GuiGraph(Scene* scene, const std::string& name)
        : GuiWidget(scene, name)
    {
    }
    void render() final;

  private:
    unsigned int _maxHistoryLength{300};
    std::map<std::string, std::deque<unsigned long long>> _durationGraph;
};

} // namespace Splash

#endif
