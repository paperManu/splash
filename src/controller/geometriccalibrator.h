/*
 * Copyright (C) 2019 Splash authors
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

#include <calimiro/texture_coordinates/texCoordUtils.h>

#include "./controller/controller.h"
#include "./core/constants.h"
#include "./utils/osutils.h"

namespace Splash
{

class GeometricCalibrator final : public ControllerObject
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
     * Constructors/operators
     */
    GeometricCalibrator(const GeometricCalibrator&) = delete;
    GeometricCalibrator& operator=(const GeometricCalibrator&) = delete;
    GeometricCalibrator(GeometricCalibrator&&) = delete;
    GeometricCalibrator& operator=(GeometricCalibrator&&) = delete;

    /**
     * Run calibration
     */
    void calibrate();

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

  private:
    enum CameraModel
    {
        Pinhole = 0,
        Fisheye = 1
    };

    /**
     * Structure holding the configuration state before calibration
     */
    struct ConfigurationState
    {
        std::vector<std::string> cameraList{};
        std::vector<std::string> windowList{};
        std::map<std::string, std::string> objectTypes{};
        std::unordered_map<std::string, std::vector<std::string>> objectLinks{};
        std::unordered_map<std::string, std::vector<std::string>> objectReversedLinks{};
        std::vector<Values> windowLayouts{};
        std::vector<uint8_t> windowTextureCount{};
    };

    /**
     * Structure holding the computed calibration parameters for a given Camera
     */
    struct CalibrationParams
    {
        std::string cameraName;
        double fov{0.0};
        double cx{0.0};
        double cy{0.0};
        glm::dvec4 eye{};
        glm::dvec4 target{};
        glm::dvec4 up{};
    };

    /**
     * Overall calibration, including the path to the 3D mesh
     */
    struct Calibration
    {
        std::string meshPath{};
        std::vector<CalibrationParams> params;
    };

    static inline const std::string _worldImageName{"__pattern_image"};
    static inline const std::string _worldBlackImage{"__black_image"};
    static inline const std::string _worldGreyImage{"__grey_image"};
    static inline const std::string _worldFilterPrefix{"__pattern_filter_"};
    static inline const std::string _finalMeshName{"final_mesh.obj"};

    Utils::CalimiroLogger _logger;
    bool _running{false};             //!< True if calibration is currently running
    bool _nextPosition{false};        //!< Set to true to capture from next camera position
    bool _finalizeCalibration{false}; //!< Set to true to finalize calibration
    bool _abortCalibration{false};
    uint32_t _positionCount{0};
    bool _computeTexCoord{true};

    float _cameraFocal{5000.f};
    CameraModel _cameraModel{CameraModel::Pinhole};
    float _structuredLightScale{1.0 / 16.0};
    std::chrono::milliseconds _captureDelay{std::chrono::milliseconds(250)};

    std::shared_ptr<Image> _grabber{nullptr};
    std::future<bool> _calibrationFuture{};

    /**
     * Calibration function
     * \return Return the generated calibration
     */
    std::optional<Calibration> calibrationFunc(const ConfigurationState& state);

    /**
     * Save the configuration state at the beginning of the calibration
     * \return Return the configuration state
     */
    ConfigurationState saveCurrentState();

    /**
     * Setup the calibration state
     * \param state Initial configuration state
     */
    void setupCalibrationState(const ConfigurationState& state);

    /**
     * Restore the configuration state to what it was before calibration
     * \param state Configuration state
     */
    void restoreState(const ConfigurationState& state);

    /**
     * Apply calibration, after the calibration process has been ran
     * \param calibration Calibration parameters
     */
    void applyCalibration(const ConfigurationState& state, const Calibration& calibration);

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_GEOMETRICCALIBRATOR_H
