/*
 * Copyright (C) 2014 Splash authors
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
 * @scope_guard.h
 * ScopeGuard, to use RAII to clean things up in a scope
 */

#ifndef SPLASH_SCOPE_GUARD_H
#define SPLASH_SCOPE_GUARD_H

#include <utility>

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

} // namespace Splash

#endif // SPLASH_SCOPE_GUARD_H
