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

#include <mutex>
#include <future>
#include <utility>

#include <glm/glm.hpp>
#include <opencv2/photo.hpp>

#include "./core/constants.h"

#include "./controller/controller.h"
#include "./core/attribute.h"
#include "./image/image_gphoto.h"
#include "./utils/cgutils.h"

namespace Splash
{

/*************/
class ColorCalibrator : public ControllerObject
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    explicit ColorCalibrator(RootObject* root);

    /**
     * Destructor
     */
    ~ColorCalibrator() = default;

    /**
     * Constructors/operators
     */
    ColorCalibrator(const ColorCalibrator&) = delete;
    ColorCalibrator& operator=(const ColorCalibrator&) = delete;
    ColorCalibrator(ColorCalibrator&&) = delete;
    ColorCalibrator& operator=(ColorCalibrator&&) = delete;

    /**
     * Update the color calibration of all cameras
     */
    void update() final;

    /**
     * Update the color response function of the physical camera
     */
    void updateCRF();

  protected:
    /**
     * Try linking the object to another one
     * \param obj GraphObject to link to
     * \return Return true if linking was successful
     */
    bool linkIt(const std::shared_ptr<GraphObject>& obj) final;

    /**
     * Try unlinking the given GraphObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkIt(const std::shared_ptr<GraphObject>& obj) final;

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
    std::shared_ptr<Image> _grabber{nullptr};
    bool _grabberAuto {false};
    cv::Mat _crf{};

    unsigned int _colorCurveSamples{5};     //!< Number of samples for each channels to create the color curves
    double _displayDetectionThreshold{1.f}; //!< Coefficient applied while detecting displays / projectors, increase to get rid of ambiant lights
    double _minimumROIArea{0.005};          //!< Minimum area size for projection detection, as a fraction of the image size
    int _imagePerHDR{1};                    //!< Number of images taken for each color-measuring HDR
    double _hdrStep{1.0};                   //!< Stops between images taken for color-measuring HDR
    int _equalizationMethod{2};
    uint32_t _colorLUTSize{16}; //!< Color LUT size for each channels

    std::vector<CalibrationParams> _calibrationParams{};

    std::future<void> _calibrationThread{};
    std::mutex _calibrationMutex{};

    /**
     * Capture an image synchronously
     * This basically calls the "capture" attribute of the Image_GPhoto and waits
     * for its timestamp to be updated
     */
    void captureSynchronously();

    /**
     * Capture an HDR image from the gcamera
     * \param nbrLDR Low dynamic ranger images count to use to create the HDR
     * \param step Stops between successive LDR images
     * \param computeResponseOnly If true, stop after the computation of the camera response function
     * \return Return the HDR image
     */
    cv::Mat3f captureHDR(unsigned int nbrLDR = 3, double step = 1.0, bool computeResponseOnly = false);

    /**
     * Compute the inverse projection transformation function, typically correcting the projector non linearity for all three channels
     * \param rgbCurves Projection transformation function as a vector of Curves
     * \return Return the inverted transformation function
     */
    std::vector<Curve> computeProjectorFunctionInverse(std::vector<Curve> rgbCurves);

    /**
     * Find the exposure which gives correctly exposed photos
     * \return Return the found exposure duration
     */
    float findCorrectExposure();

    /**
     * Find the center of region with max values
     * \return Return a vector<float> containing the coordinates (x, y) of the ROI as well as the side length
     */
    std::vector<int> getMaxRegionROI(const cv::Mat3f& image);

    /**
     * Get a mask of the projectors surface
     * \param image The image to compute the mask for
     * \return Return a vector<bool> the same size as image
     */
    std::vector<bool> getMaskROI(const cv::Mat3f& image);

    /**
     * Get the mean value of the area around the given coords
     * \param image Input image
     * \param coords Box center
     * \param boxSize Box size
     * \return Return the mean value for each channel
     */
    std::vector<float> getMeanValue(const cv::Mat3f& image, std::vector<int> coords = std::vector<int>(), int boxSize = 32);

    /*
     * Get the mean value of the area defined by the mask
     * \param image Input image
     * \param mask Vector holding the coordinates (x, y) and the side length
     * \return Return the mean value for each channel
     */
    std::vector<float> getMeanValue(const cv::Mat3f& image, std::vector<bool> mask);

    /*
     * Update equalization strategies without capturing again images
     * Allow to switch equalization method quickly
     */
    void computeCalibration();

    /*
     * Load calibration parameters if a calibration was already computed and saved
     */
    bool loadCalibrationParams();

    /**
     * White balance equalization strategies
     */
    std::function<RgbValue()> equalizeWhiteBalances;
    RgbValue equalizeWhiteBalancesOnly();           //!< Only equalize WB without caring about luminance
    RgbValue equalizeWhiteBalancesFromWeakestLum(); //!< Match all WB with the one of the weakest projector
    RgbValue equalizeWhiteBalancesMaximizeMinLum(); //!< Match WB so as to maximize minimum luminance

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif
