/*
 * Copyright (C) 2017 Emmanuel Durand
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
 * @widget_meshes.h
 * The Meshes widget, to update a mesh geometry source
 */

#ifndef SPLASH_WIDGET_MESHES_H
#define SPLASH_WIDGET_MESHES_H

#include "./controller/widget/widget.h"

namespace Splash
{

/*************/
class GuiMeshes : public GuiWidget
{
  public:
    GuiMeshes(Scene* scene, const std::string& name);
    void render() final;
    int updateWindowFlags() final;

  private:
    std::map<std::string, int> _meshTypeIndex;
    std::map<std::string, std::string> _meshType;
    std::map<std::string, std::string> _meshTypeReversed{}; // Created from the previous map

    Values _newMedia{"image", "", 0.f, 0.f};

    std::list<std::shared_ptr<GraphObject>> getSceneMeshes();
    void replaceMesh(const std::string& previousMedia, const std::string& media, const std::string& type);
};

} // end of namespace

#endif
