/*
 * Copyright (C) 2016 Emmanuel Durand
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
 * @userinput_keyboard.h
 * The Keyboard class, which grabs keyboard events
 */

#ifndef SPLASH_USERINPUT_KEYBOARD_H
#define SPLASH_USERINPUT_KEYBOARD_H

#include "./userinput.h"

namespace Splash
{

class Keyboard final : public UserInput
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    explicit Keyboard(RootObject* root);

    /**
     * Destructor
     */
    ~Keyboard() final;

    /**
     * Constructors/operators
     */
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;
    Keyboard(Keyboard&&) = delete;
    Keyboard& operator=(Keyboard&&) = delete;

  private:
    /**
     * Input update method
     */
    void updateMethod() final;
};

} // end of namespace

#endif // SPLASH_USERINPUT_KEYBOARD_H
