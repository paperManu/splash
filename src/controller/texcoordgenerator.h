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
 * @texcoordgenerator.h
 * The TexCoordGenerator class
 */

#ifndef SPLASH_TEXCOORDGENERATOR_H
#define SPLASH_TEXCOORDGENERATOR_H

#include <calimiro/geometry.h>
#include <calimiro/texture_coordinates/texCoordUtils.h>

#include "./controller/controller.h"
#include "./utils/osutils.h"

namespace Splash
{

class TexCoordGenerator : public ControllerObject
{
  public:
    /**
     * Constructor
     */
    explicit TexCoordGenerator(RootObject* root);

    /**
     * Generate texture coordinates (to be used from within geometricCalibrator)
     * \param geometry a geometry that must at least include vertices
     * \return a new geometry with added texture coordinates
     */
    calimiro::Geometry generateTexCoordFromGeometry(calimiro::Geometry& geometry);

    /**
     * Generate texture coordinates (to be used on a mesh in a scene)
     * The mesh on which to generate the texture coordinates is defined by the attribute "meshName"
     */
    void generateTexCoordOnMesh();

  private:
    calimiro::TexCoordUtils::texCoordMethod _method{calimiro::TexCoordUtils::texCoordMethod::ORTHOGRAPHIC};
    glm::vec3 _eyePosition = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 _eyeOrientation = glm::vec3(0.f, 0.f, 1.f);
    float _sphericFov{180.f};
    bool _replaceMesh{true};
    float _horizonRotation{0.f};
    bool _flipHorizontal{false};
    bool _flipVertical{false};
    std::string _meshName;

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_TEXCOORDGENERATOR_H
