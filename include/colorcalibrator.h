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
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @colorcalibrator.h
 * The ColorCalibrator class
 */

#ifndef SPLASH_COLORCALIBRATOR_H
#define SPLASH_COLORCALIBRATOR_H

#include <utility>
#include <glm/glm.hpp>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "cgUtils.h"
#include "image_gphoto.h"

namespace pic {
class Image;
class CameraResponseFunction;
}

namespace Splash {

class World;
class Scene;

/*************/
class ColorCalibrator : public BaseObject
{
    public:
        /**
         * Constructor
         */
        ColorCalibrator(std::weak_ptr<Scene> scene);

        /**
         * Destructor
         */
        ~ColorCalibrator();

        /**
         * No copy constructor
         */
        ColorCalibrator(const ColorCalibrator&) = delete;
        ColorCalibrator& operator=(const ColorCalibrator&) = delete;

        /**
         * Update the color calibration of all cameras
         */
        void update();

        /**
         * Update the color response function of the physical camera
         */
        void updateCRF();

    private:
        //
        // Some internal types
        //
        typedef std::pair<float, RgbValue> Point;
        typedef std::vector<Point> Curve;

        struct CalibrationParams
        {
            std::string camName {};
            std::vector<int> camROI {0, 0};
            std::vector<bool> maskROI;
            RgbValue whitePoint;
            RgbValue whiteBalance;
            RgbValue minValues;
            RgbValue maxValues;
            std::vector<Curve> curves {3};
            std::vector<Curve> projectorCurves;
            glm::mat3 mixRGB;
        };

        //
        // Attributes
        //
        std::weak_ptr<Scene> _scene;
        Image_GPhotoPtr _gcamera;
        std::shared_ptr<pic::CameraResponseFunction> _crf {nullptr};

        unsigned int _colorCurveSamples {5}; // Number of samples for each channels to create the color curves
        double _displayDetectionThreshold {1.f}; // Coefficient applied while detecting displays / projectors, increase to get rid of ambiant lights
        double _minimumROIArea {0.005}; // Minimum area size for projection detection, as a fraction of the image size
        int _imagePerHDR {1}; // Number of images taken for each color-measuring HDR
        double _hdrStep {1.0}; // Stops between images taken for color-measuring HDR
        int _equalizationMethod {2}; //

        std::vector<CalibrationParams> _calibrationParams;

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
        std::vector<int> getMaxRegionROI(std::shared_ptr<pic::Image> image);

        /**
         * Get a mask of the projectors surface
         */
        std::vector<bool> getMaskROI(std::shared_ptr<pic::Image> image);

        /**
         * Get the mean value of the area around the given coords
         */
        std::vector<float> getMeanValue(std::shared_ptr<pic::Image> image, std::vector<int> coords = std::vector<int>(), int boxSize = 32);
        std::vector<float> getMeanValue(std::shared_ptr<pic::Image> image, std::vector<bool> mask);

        /**
         * White balance equalization strategies
         */
        std::function<RgbValue()> equalizeWhiteBalances;
        RgbValue equalizeWhiteBalancesOnly(); // Only equalize WB without caring about luminance
        RgbValue equalizeWhiteBalancesFromWeakestLum(); // Match all WB with the one of the weakest projector
        RgbValue equalizeWhiteBalancesMaximizeMinLum(); // Match WB so as to maximize minimum luminance

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<ColorCalibrator> ColorCalibratorPtr;

} // end of namespace

#endif
