/*
 * Copyright (C) 2015 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @colorcalibrator.h
 * The ColorCalibrator class
 */

#ifndef SPLASH_COLORCALIBRATOR_H
#define SPLASH_COLORCALIBRATOR_H

#include "config.h"
#include "coretypes.h"
#include "basetypes.h"

namespace Splash {

class World;
typedef std::weak_ptr<World> WorldWeakPtr;

/*************/
class ColorCalibrator : public BaseObject
{
    public:
        /**
         * Constructor
         */
        ColorCalibrator(WorldWeakPtr world);

        /**
         * Destructor
         */
        ~ColorCalibrator();

        /**
         * No copy constructor
         */
        ColorCalibrator(const ColorCalibrator&) = delete;
        ColorCalibrator& operator=(const ColorCalibrator&) = delete;

        ColorCalibrator(ColorCalibrator&& c)
        {
            *this = std::move(c);
        }

        ColorCalibrator& operator=(ColorCalibrator&& c)
        {
            if (this != &c)
            {
            }
            return *this;
        }

        /**
         * Update the color calibration of all cameras
         */
        void update();

    private:
        WorldWeakPtr _world;
        Image_GPhotoPtr _gcamera;
        
        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<ColorCalibrator> ColorCalibratorPtr;

} // end of namespace

#endif
