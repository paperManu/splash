/*
 * Copyright (C) 2016 Splash authors
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
 * @userinput_joystick.h
 * The Joystick class, which grabs joystick events
 */

#ifndef SPLASH_USERINPUT_JOYSTICK_H
#define SPLASH_USERINPUT_JOYSTICK_H

#include "./userinput.h"

namespace Splash
{

class Joystick final : public UserInput
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    explicit Joystick(RootObject* root);

    /**
     * Destructor
     */
    ~Joystick() final;

    /**
     * Constructors/operators
     */
    Joystick(const Joystick&) = delete;
    Joystick& operator=(const Joystick&) = delete;
    Joystick(Joystick&&) = delete;
    Joystick& operator=(Joystick&&) = delete;

  private:
    struct Stick
    {
        std::vector<float> axes;
        std::vector<uint8_t> buttons;
    };

    std::vector<Stick> _joysticks; //!< Current joysticks state

    /**
     * Update the joystick list
     */
    void detectJoysticks();

    /**
     * Input update method
     */
    void updateMethod() final;

    /**
     * Callbacks update method
     */
    void updateCallbacks() final;

    /**
     * State update method
     */
    void readState() final;
};

} // namespace Splash

#endif // SPLASH_USERINPUT_JOYSTICK_H
