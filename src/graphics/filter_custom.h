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
 * The FilterCustom class
 */

#ifndef SPLASH_FILTER_CUSTOM_H
#define SPLASH_FILTER_CUSTOM_H

#include <filesystem>

#include "./core/constants.h"
#include "./graphics/filter.h"

namespace Splash
{

/*************/
class FilterCustom : public Filter
{
  public:
    /**
     *  Constructor
     * \param root Root object
     */
    FilterCustom(RootObject* root);

    /**
     * No copy constructor, but a move one
     */
    FilterCustom(const FilterCustom&) = delete;
    FilterCustom(FilterCustom&&) = default;
    FilterCustom& operator=(const FilterCustom&) = delete;

  private:
    std::string _shaderSource{""};                            //!< User defined fragment shader filter
    std::string _shaderSourceFile{""};                        //!< User defined fragment shader filter source file
    bool _watchShaderFile{false};                             //!< If true, updates shader automatically if source file changes
    std::filesystem::file_time_type _lastShaderSourceWrite{}; //!< Last time the shader source has been updated

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes() override;

    /**
     * Register attributes related to the default shader
     */
    void registerDefaultShaderAttributes() override{};

    /**
     * Set the filter fragment shader. Automatically adds attributes corresponding to the uniforms
     * \param source Source fragment shader
     * \return Return true if the shader is valid
     */
    bool setFilterSource(const std::string& source);
};

} // namespace Splash

#endif // SPLASH_FILTER_CUSTOM_H
