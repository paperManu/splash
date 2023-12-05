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
 * @gl_utils.h
 * OpenGL utilities
 */

#ifndef SPLASH_OPENGL_GL_UTILS_H
#define SPLASH_OPENGL_GL_UTILS_H

#include "./core/constants.h"

namespace Splash::gfx::opengl
{

#ifdef DEBUG
class OpenGLErrorGuard
{
  public:
    explicit OpenGLErrorGuard(std::string_view scope);

    ~OpenGLErrorGuard();

  private:
    const std::string _scope;
};

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
#define OnOpenGLScopeExit auto CONCATENATE(on_opengl_scope_exit_var, __LINE__) = OpenGLErrorGuard
#else
#define OnOpenGLScopeExit
#endif

} // namespace Splash::gfx::opengl

#endif
