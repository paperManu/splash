/*
 * Copyright (C) 2020 Splash authors
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
 * @filter_black_level.h
 * The FilterColorCurves class
 */

#ifndef SPLASH_FILTER_COLOR_CURVES_H
#define SPLASH_FILTER_COLOR_CURVES_H

#include "./core/constants.h"
#include "./graphics/filter.h"

namespace Splash
{

/*************/
class FilterColorCurves final : public Filter
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    FilterColorCurves(RootObject* root);

    /**
     * Destructor
     */
    ~FilterColorCurves() final = default;

    /**
     * Constructors/operators
     */
    FilterColorCurves(const FilterColorCurves&) = delete;
    FilterColorCurves& operator=(const FilterColorCurves&) = delete;
    FilterColorCurves(FilterColorCurves&&) = delete;
    FilterColorCurves& operator=(FilterColorCurves&&) = delete;

  private:
    Values _colorCurves{}; //!< RGB points for the color curves, active if at least 3 points are set

    /**
     * Register attributes related to the default shader
     */
    void registerDefaultShaderAttributes() override;

    /**
     * Update the shader parameters, if it is the default shader
     */
    void updateShaderParameters();

    /**
     * Updates the shader uniforms according to the textures and images the filter is connected to.
     */
    void updateUniforms() override;
};

} // namespace Splash

#endif // SPLASH_FILTER_COLOR_CURVES_H
