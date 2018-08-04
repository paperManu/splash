/*
 * Copyright (C) 2014 Emmanuel Durand
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
 * @coretypes.h
 * A few, mostly basic, types
 */

#include "config.h"

#define SPLASH
#define SPLASH_GL_DEBUG true

#define SPLASH_ALL_PEERS "__ALL__"
#define SPLASH_DEFAULTS_FILE_ENV "SPLASH_DEFAULTS"

#define SPLASH_FILE_CONFIGURATION "splashConfiguration"
#define SPLASH_FILE_PROJECT "splashProject"

#include <ostream>
#include <execinfo.h>

#include "./graphics/gl_window.h"
#include "./utils/log.h"
#include "./core/resizable_array.h"
#include "./core/serialized_object.h"
#include "./core/spinlock.h"
#include "./utils/timer.h"
#include "./core/value.h"

#ifndef SPLASH_CORETYPES_H
#define SPLASH_CORETYPES_H

#define PRINT_FUNCTION_LINE std::cout << "------> " << __PRETTY_FUNCTION__ << "::" << __LINE__ << std::endl;

#define PRINT_CALL_STACK                                                                                                                                                           \
    {                                                                                                                                                                              \
        int j, nptrs;                                                                                                                                                              \
        int size = 100;                                                                                                                                                            \
        void* buffers[size];                                                                                                                                                       \
        char** strings;                                                                                                                                                            \
                                                                                                                                                                                   \
        nptrs = backtrace(buffers, size);                                                                                                                                          \
        strings = backtrace_symbols(buffers, nptrs);                                                                                                                               \
        for (j = 0; j < nptrs; ++j)                                                                                                                                                \
            std::cout << strings[j] << endl;                                                                                                                                       \
                                                                                                                                                                                   \
        free(strings);                                                                                                                                                             \
    }

namespace Splash
{

/*************/
//! OnScopeExit, taken from Switcher (https://github.com/nicobou/switcher)
template <typename F>
class ScopeGuard
{
  public:
    explicit ScopeGuard(F&& f)
        : f_(std::move(f))
    {
    }
    ~ScopeGuard() { f_(); }
  private:
    F f_;
};

enum class ScopeGuardOnExit
{
};
template <typename F>
ScopeGuard<F> operator+(ScopeGuardOnExit, F&& f)
{
    return ScopeGuard<F>(std::forward<F>(f));
}

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
#define OnScopeExit auto CONCATENATE(on_scope_exit_var, __LINE__) = ScopeGuardOnExit() + [&]()

} // end of namespace

#endif // SPLASH_CORETYPES_H
