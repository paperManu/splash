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
 * @userinput_dragndrop.h
 * The DragNDrop class, which handles drag and drop actions
 */

#ifndef SPLASH_USERINPUT_DRAGNDROP_H
#define SPLASH_USERINPUT_DRAGNDROP_H

#include "./userinput.h"

namespace Splash
{

class DragNDrop final : public UserInput
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    explicit DragNDrop(RootObject* root);

    /**
     * \brief Destructor
     */
    ~DragNDrop() final;

    /**
     * Constructors/operators
     */
    DragNDrop(const DragNDrop&) = delete;
    DragNDrop& operator=(const DragNDrop&) = delete;
    DragNDrop(DragNDrop&&) = delete;
    DragNDrop& operator=(DragNDrop&&) = delete;

  private:
    /**
     * \brief Input update method
     */
    void updateMethod() final;
};

} // end of namespace

#endif // SPLASH_USERINPUT_DRAGNDROP_H
