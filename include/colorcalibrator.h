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

#include <utility>
#include <glm/glm.hpp>

namespace pic {
class Image;
class CameraResponseFunction;
}

namespace Splash {

class World;

/*************/
class ColorCalibrator : public BaseObject
{
    public:
        /**
         * Constructor
         */
        ColorCalibrator(std::weak_ptr<World> world);

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

        /**
         * Update the color response function of the physical camera
         */
        void updateCRF();

    private:
        // Some internal types
        struct RgbValue
        {
            RgbValue() {};
            RgbValue(Values v)
            {
                if (v.size() != 3)
                    return;
                r = v[0].asFloat();
                g = v[1].asFloat();
                b = v[2].asFloat();
            }
            RgbValue(std::vector<float> v)
            {
                if (v.size() != 3)
                    return;
                r = v[0];
                g = v[1];
                b = v[2];
            }

            float operator[](int i)
            {
                if (i == 0)
                    return r;
                else if (i == 1)
                    return g;
                else if (i == 2)
                    return b;
                else
                    return 0.f;
            }

            void set(int i, float v)
            {
                if (i == 0)
                    r = v;
                else if (i == 1)
                    g = v;
                else if (i == 2)
                    b = v;
            }

            float r {0.f};
            float g {0.f};
            float b {0.f};
        };
        typedef std::pair<float, RgbValue> Point;
        typedef std::vector<Point> Curve;

        struct CalibrationParams
        {
            std::vector<int> camPos {0, 0};
            std::vector<float> blackValues {0.f, 0.f, 0.f};
            std::vector<float> minValues {0.f, 0.f, 0.f};
            std::vector<float> maxValues {0.f, 0.f, 0.f};
            std::vector<Curve> curves {3};
            std::vector<Curve> projectorCurves;
            glm::mat3 mixRGB;
        };

        // Attributes
        std::weak_ptr<World> _world;
        Image_GPhotoPtr _gcamera;
        std::shared_ptr<pic::CameraResponseFunction> _crf {nullptr};

        unsigned int _colorCurveSamples {4}; // Number of samples for each channels to create the color curves

        /**
         * Capture an HDR image from the gcamera
         */
        std::shared_ptr<pic::Image> captureHDR(unsigned int nbrLDR = 3, double step = 1.0);

        /**
         * Compute the inverse projection transformation function, typically
         * correcting the projector non linearity for all three channels
         */
        std::vector<Curve> computeProjectorFunctionInverse(std::vector<Curve> rgbCurves);

        /**
         * Find the exposure which gives correctly exposed photos
         */
        float findCorrectExposure();

        /**
         * Find the center of region with max values
         */
        std::vector<int> getMaxRegionCenter(std::shared_ptr<pic::Image> image);

        /**
         * Get the mean value of the area around the given coords
         */
        std::vector<float> getMeanValue(std::shared_ptr<pic::Image> image, std::vector<int> coords = std::vector<int>(), int boxSize = 32);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<ColorCalibrator> ColorCalibratorPtr;

} // end of namespace

#endif
