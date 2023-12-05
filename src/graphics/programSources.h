/*
 * Copyright (C) 2023 Splash authors
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
 * @shaderSources.h
 * Contains the sources for all shaders used in Splash
 */

#ifndef SPLASH_PROGRAMSOURCES_H
#define SPLASH_PROGRAMSOURCES_H

#include <map>
#include <string>
#include <string_view>

#include "./graphics/api/shader_gfx_impl.h"
#include "./graphics/shaderSources.h"

namespace Splash::ProgramSources
{
/**
 * Struct for full shader sources
 */
struct ProgramSources
{
    const std::string_view vertex{};
    const std::string_view tess_ctrl{};
    const std::string_view tess_eval{};
    const std::string_view geometry{};
    const std::string_view fragment{};
    const std::string_view compute{};

    const std::string_view operator[](gfx::ShaderType type) const
    {
        switch (type)
        {
        default:
            break;
        case gfx::ShaderType::vertex:
            return vertex;
        case gfx::ShaderType::tess_ctrl:
            return tess_ctrl;
        case gfx::ShaderType::tess_eval:
            return tess_eval;
        case gfx::ShaderType::geometry:
            return geometry;
        case gfx::ShaderType::fragment:
            return fragment;
        case gfx::ShaderType::compute:
            return compute;
        }

        assert(false);
        return {};
    }
};

// clang-format off
    const std::map<std::string_view, const ProgramSources> Sources = {
        {"texture", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_TEXTURE,
            .fragment = ShaderSources::FRAGMENT_SHADER_TEXTURE}},
        {"object_cubemap", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_OBJECT_CUBEMAP,
            .geometry = ShaderSources::GEOMETRY_SHADER_OBJECT_CUBEMAP,
            .fragment = ShaderSources::FRAGMENT_SHADER_OBJECT_CUBEMAP}},
        {"cubemap_projection", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_CUBEMAP_PROJECTION,
            .fragment = ShaderSources::FRAGMENT_SHADER_CUBEMAP_PROJECTION}},
        {"image_filter", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_FILTER,
            .fragment = ShaderSources::FRAGMENT_SHADER_IMAGE_FILTER}},
        {"blacklevel_filter", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_FILTER,
            .fragment = ShaderSources::FRAGMENT_SHADER_BLACKLEVEL_FILTER}},
        {"color_curves_filter", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_FILTER,
            .fragment = ShaderSources::FRAGMENT_SHADER_COLOR_CURVES_FILTER}},
        {"color", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_MODELVIEW,
            .fragment = ShaderSources::FRAGMENT_SHADER_COLOR}},
        {"primitiveId", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_MODELVIEW,
            .fragment = ShaderSources::FRAGMENT_SHADER_PRIMITIVEID}},
        {"uv", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_MODELVIEW,
            .fragment = ShaderSources::FRAGMENT_SHADER_UV}},
        {"warp", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_WARP,
            .fragment = ShaderSources::FRAGMENT_SHADER_WARP}},
        {"warp_control", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_WARP_WIREFRAME,
            .geometry = ShaderSources::GEOMETRY_SHADER_WARP_WIREFRAME,
            .fragment = ShaderSources::FRAGMENT_SHADER_WARP_WIREFRAME}},
        {"wireframe", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_WIREFRAME,
            .geometry = ShaderSources::GEOMETRY_SHADER_WIREFRAME,
            .fragment = ShaderSources::FRAGMENT_SHADER_WIREFRAME}},
        {"window", ProgramSources {
            .vertex = ShaderSources::VERTEX_SHADER_WINDOW,
            .fragment = ShaderSources::FRAGMENT_SHADER_WINDOW}}
    };
// clang-format on
} // namespace Splash::ProgramSources

#endif
