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
 * @userInput_joystick.h
 * The Joystick class, which grabs joystick events
 */

#ifndef SPLASH_USERINPUT_JOYSTICK_H
#define SPLASH_USERINPUT_JOYSTICK_H

#include "./userInput.h"

namespace Splash
{

class Joystick : public UserInput
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Joystick(std::weak_ptr<RootObject> root);

    /**
     * \brief Destructor
     */
    ~Joystick();

  private:
    struct Stick
    {
        std::vector<float> axes;
        std::vector<uint8_t> buttons;
    };

    std::vector<Stick> _joysticks; //!< Current joysticks state

    /**
     * \brief Update the joystick list
     */
    void detectJoysticks();

    /**
     * \brief Input update method
     */
    void updateMethod();

    /**
     * \brief Callbacks update method
     */
    void updateCallbacks() final;

    /**
     * \brief State update method
     */
    void readState() final;
};

} // end of namespace

#endif // SPLASH_USERINPUT_JOYSTICK_H
