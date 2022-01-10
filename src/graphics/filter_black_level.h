/*
 * Copyright (C) 2020 Emmanuel Durand
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
 * The FilterBlackLevel class
 */

#ifndef SPLASH_FILTER_BLACK_LEVEL_H
#define SPLASH_FILTER_BLACK_LEVEL_H

#include "./core/constants.h"
#include "./graphics/filter.h"

namespace Splash
{

/*************/
class FilterBlackLevel final : public Filter
{
  public:
    /**
     *  Constructor
     * \param root Root object
     */
    FilterBlackLevel(RootObject* root);

    /**
     * Destructor
     */
    ~FilterBlackLevel() final = default;

    /**
     * Constructors/operators
     */
    FilterBlackLevel(const FilterBlackLevel&) = delete;
    FilterBlackLevel& operator=(const FilterBlackLevel&) = delete;
    FilterBlackLevel(FilterBlackLevel&&) = delete;
    FilterBlackLevel& operator=(FilterBlackLevel&&) = delete;

    /**
     *  Render the filter
     */
    void render() override;

  private:
    float _autoBlackLevelTargetValue{0.f}; //!< If not zero, defines the target luminance value
    float _autoBlackLevelSpeed{1.f};       //!< Time to match the black level target value
    float _autoBlackLevel{0.f};
    int64_t _previousTime{0}; //!< Used for computing the current black value regarding black value speed

    /**
     * Register attributes related to the default shader
     */
    void registerDefaultShaderAttributes() override;
};

} // namespace Splash

#endif // SPLASH_FILTER_BLACK_LEVEL_H
