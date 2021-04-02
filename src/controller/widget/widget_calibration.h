/*
 * Copyright (C) 2021 Frédéric Lestage
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
 * @widget_calibration.h
 * The Calibrator (Calimiro) widget
 */

#ifndef SPLASH_WIDGET_CALIBRATION_H
#define SPLASH_WIDGET_CALIBRATION_H

#include "./config.h" // Must be included before HAVE_CALIMIRO is used, even if its also indirectly added by widget.h

#if HAVE_CALIMIRO
#include <calimiro/texture_coordinates/texCoordUtils.h>
#endif

#include "./controller/widget/widget.h"

namespace Splash
{

/*************/
class GuiCalibration : public GuiWidget
{
  public:
    GuiCalibration(Scene* scene, const std::string& name);
    void render() final;
    int updateWindowFlags() final;

  private:
#if HAVE_CALIMIRO
    std::vector<std::string> _texCoordMethodsList;
    std::map<std::string, calimiro::TexCoordUtils::texCoordMethod> _stringToMethod;

    void renderGeometricCalibration(ImVec2& availableSize);
    void renderTexCoordCalibration(ImVec2& availableSize);
#endif

    /**
     * Getter for the meshes in the scene
     */
    const std::list<std::shared_ptr<Mesh>> getSceneMeshes();
};

} // namespace Splash

#endif
