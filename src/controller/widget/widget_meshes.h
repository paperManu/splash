/*
 * Copyright (C) 2017 Splash authors
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

class Camera;
class Image;
class Object;
class Shader;

/*************/
class GuiMeshes : public GuiWidget
{
  public:
    GuiMeshes(Scene* scene, const std::string& name);
    void render() final;
    int updateWindowFlags() final;
    std::optional<std::string> getActiveObjectName() const final { return _selectedMeshName.empty() ? std::optional<std::string>() : _selectedMeshName; }

  private:
    std::map<std::string, int> _meshTypeIndex;
    std::map<std::string, std::string> _meshType;
    std::map<std::string, std::string> _meshTypeReversed{}; // Created from the previous map

    std::shared_ptr<Camera> _previewCamera{nullptr};
    std::shared_ptr<Object> _previewObject{nullptr};
    std::shared_ptr<Shader> _previewShader{nullptr};
    std::shared_ptr<Image> _previewImage{nullptr};
    std::shared_ptr<Mesh> _currentMesh{nullptr};
    int _camWidth{0}, _camHeight{0}; //!< Size of the view

    Values _cameraTarget;
    float _cameraTargetDistance{1.f};

    Values _newMedia{"image", "", 0.f, 0.f};
    std::string _selectedMeshName;

    const std::list<std::shared_ptr<Mesh>> getSceneMeshes();
    void processMouseEvents();
    void replaceMesh(const std::string& previousMedia, const std::string& media, const std::string& type);
};

} // namespace Splash

#endif
