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

#include <glm/glm.hpp>
#include <utility>

#include "./config.h"

#include "./core/attribute.h"
#include "./utils/cgutils.h"
#include "./core/coretypes.h"
#include "./image/image_gphoto.h"

namespace pic
{
class Image;
class CameraResponseFunction;
}

namespace Splash
{

class Scene;

/*************/
class ColorCalibrator : public BaseObject
{
  public:
    /**
     * \brief Constructor
     * \param scene Root scene
     */
    ColorCalibrator(RootObject* scene);

    /**
     * \brief Destructor
     */
    ~ColorCalibrator() override;

    /**
     * No copy constructor
     */
    ColorCalibrator(const ColorCalibrator&) = delete;
    ColorCalibrator& operator=(const ColorCalibrator&) = delete;

    /**
     * \brief Update the color calibration of all cameras
     */
    void update() override;

    /**
     * \brief Update the color response function of the physical camera
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
        std::string camName{};
        std::vector<int> camROI{0, 0};
        std::vector<bool> maskROI;
        RgbValue whitePoint;
        RgbValue whiteBalance;
        RgbValue minValues;
        RgbValue maxValues;
        std::vector<Curve> curves{3};
        std::vector<Curve> projectorCurves;
        glm::mat3 mixRGB;
    };

    //
    // Attributes
    //
    RootObject* _scene; // TODO: use _root instead
    std::shared_ptr<Image_GPhoto> _gcamera;
    std::shared_ptr<pic::CameraResponseFunction> _crf{nullptr};

    unsigned int _colorCurveSamples{5};     //!< Number of samples for each channels to create the color curves
    double _displayDetectionThreshold{1.f}; //!< Coefficient applied while detecting displays / projectors, increase to get rid of ambiant lights
    double _minimumROIArea{0.005};          //!< Minimum area size for projection detection, as a fraction of the image size
    int _imagePerHDR{1};                    //!< Number of images taken for each color-measuring HDR
    double _hdrStep{1.0};                   //!< Stops between images taken for color-measuring HDR
    int _equalizationMethod{2};

    std::vector<CalibrationParams> _calibrationParams;

    /**
     * \brief Capture an HDR image from the gcamera
     * \param nbrLDR Low dynamic ranger images count to use to create the HDR
     * \param step Stops between successive LDR images
     */
    std::shared_ptr<pic::Image> captureHDR(unsigned int nbrLDR = 3, double step = 1.0);

    /**
     * \brief Compute the inverse projection transformation function, typically correcting the projector non linearity for all three channels
     * \param rgbCurves Projection transformation function as a vector of Curves
     * \return Return the inverted transformation function
     */
    std::vector<Curve> computeProjectorFunctionInverse(std::vector<Curve> rgbCurves);

    /**
     * \brief Find the exposure which gives correctly exposed photos
     * \return Return the found exposure duration
     */
    float findCorrectExposure();

    /**
     * \brief Find the center of region with max values
     * \return Return a vector<float> containing the coordinates (x, y) of the ROI as well as the side length
     */
    std::vector<int> getMaxRegionROI(std::shared_ptr<pic::Image> image);

    /**
     * \brief Get a mask of the projectors surface
     * \param image The image to compute the mask for
     * \return Return a vector<bool> the same size as image
     */
    std::vector<bool> getMaskROI(std::shared_ptr<pic::Image> image);

    /**
     * \brief Get the mean value of the area around the given coords
     * \param image Input image
     * \param coords Box center
     * \param boxSize Box size
     * \return Return the mean value for each channel
     */
    std::vector<float> getMeanValue(std::shared_ptr<pic::Image> image, std::vector<int> coords = std::vector<int>(), int boxSize = 32);

    /*
     * \brief Get the mean value of the area defined by the mask
     * \param image Input image
     * \param mask Vector holding the coordinates (x, y) and the side length
     * \return Return the mean value for each channel
     */
    std::vector<float> getMeanValue(std::shared_ptr<pic::Image> image, std::vector<bool> mask);

    /**
     * \brief White balance equalization strategies
     */
    std::function<RgbValue()> equalizeWhiteBalances;
    RgbValue equalizeWhiteBalancesOnly();           //!< Only equalize WB without caring about luminance
    RgbValue equalizeWhiteBalancesFromWeakestLum(); //!< Match all WB with the one of the weakest projector
    RgbValue equalizeWhiteBalancesMaximizeMinLum(); //!< Match WB so as to maximize minimum luminance

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif
