/*
 * Copyright (C) 2019 Emmanuel Durand
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
 * @geometriccalibrator.h
 * The GeometricCalibrator class
 */

#ifndef SPLASH_GEOMETRICCALIBRATOR_H
#define SPLASH_GEOMETRICCALIBRATOR_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>

#include "./config.h"

#include "./controller/controller.h"

namespace Splash
{

class GeometricCalibrator : public ControllerObject
{
  public:
    /**
     * Constructor
     */
    explicit GeometricCalibrator(RootObject* root);

    /**
     * Destructor
     */
    ~GeometricCalibrator() final;

    /**
     * Run calibration
     */
    void calibrate();

    /**
     * Try linking the object to another one
     * \param obj GraphObject to link to
     * \return Return true if linking was successful
     */
    bool linkTo(const std::shared_ptr<GraphObject>& obj) final;

    /**
     * Try unlinking the given GraphObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkFrom(const std::shared_ptr<GraphObject>& obj) final;

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();

  private:
    enum CameraModel
    {
        Pinhole = 0,
        Fisheye = 1
    };

    bool _running{false};             //!< True if calibration is currently running
    bool _nextPosition{false};        //!< Set to true to capture from next camera position
    bool _finalizeCalibration{false}; //!< Set to true to finalize calibration
    bool _abortCalibration{false};

    float _cameraFocal{5000.f};
    CameraModel _cameraModel{CameraModel::Fisheye};
    float _structuredLightScale{1.0 / 16.0};
    std::chrono::milliseconds _captureDelay{250ms};

    std::shared_ptr<Image> _grabber{nullptr};

    std::future<bool> _calibrationFuture{};

    /**
     * Calibration function, ran in a separate thread through _calibrationFuture
     * \return Return true if all went well during the calibration process
     */
    bool calibrationFunc();
};

} // namespace Splash

#endif // SPLASH_GEOMETRICCALIBRATOR_H
