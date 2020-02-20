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
 * @image_shmv.h
 * The Image_ShmV class
 */

#ifndef SPLASH_IMAGE_SHMV_H
#define SPLASH_IMAGE_SHMV_H

#include "./core/constants.h"

#include "./image/image.h"

namespace Splash
{

class Image_ShmV : public Image
{
  public:
    /**
     * Constructor
     */
    Image_ShmV(RootObject* root);

    /**
     * Destructor
     */
    ~Image_ShmV() final = default;

    /**
     * No copy constructor
     */
    Image_ShmV(const Image_ShmV&) = delete;
    Image_ShmV& operator=(const Image_ShmV&) = delete;

  private:
    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_IMAGE_SHMV_H
