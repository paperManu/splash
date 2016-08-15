/*
 * Copyright (C) 2014 Emmanuel Durand
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
 * @widget_global_view.h
 * The global view widget, to calibrate cameras
 */

#ifndef SPLASH_WIDGET_GLOBAL_VIEW_H
#define SPLASH_WIDGET_GLOBAL_VIEW_H

#include "./camera.h"
#include "./widget.h"

namespace Splash
{

/*************/
class GuiGlobalView : public GuiWidget
{
    public:
        GuiGlobalView(std::weak_ptr<Scene> scene, std::string name = "")
            : GuiWidget(scene, name) {}
        void render();
        int updateWindowFlags();
        void setCamera(const std::shared_ptr<Camera>& cam);
        void setJoystick(const std::vector<float>& axes, const std::vector<uint8_t>& buttons);
        void setScene(std::weak_ptr<Scene> scene) {_scene = scene;}

    protected:
        std::shared_ptr<Camera> _camera, _guiCamera;
        std::weak_ptr<Scene> _scene;
        bool _camerasHidden {false};
        bool _beginDrag {true};
        bool _noMove {false};

        bool _hideCameras {false};
        bool _camerasColorized {false};

        // Size of the view
        int _camWidth {0}, _camHeight {0};

        // Joystick state
        std::vector<float> _joyAxes {};
        std::vector<uint8_t> _joyButtons {};
        std::vector<uint8_t> _joyButtonsPrevious {};

        // Store the previous camera values
        struct CameraParameters
        {
            Values eye, target, up, fov, principalPoint;
        };
        std::vector<CameraParameters> _previousCameraParameters;
        Values _newTarget;
        float _newTargetDistance {1.f};

        // Previous point added
        Values _previousPointAdded;

        void processJoystickState();
        void processKeyEvents();
        void processMouseEvents();

        // Actions
        /**
         * \brief Activate the colorization of the wireframe rendering
         * \param colorize If true, activate the colorization
         */
        void colorizeCameraWireframes(bool colorize);

        void doCalibration();
        void propagateCalibration(); // Propagates calibration to other Scenes if needed

        /**
         * \brief Hide all cameras except for the selected one
         * \param hide Hide cameras if true
         */
        void hideOtherCameras(bool hide);

        void nextCamera();
        void revertCalibration();
        void showAllCalibrationPoints();
        void showAllCamerasCalibrationPoints();

        // Other
        std::vector<glm::dmat4> getCamerasRTMatrices();
        std::vector<std::shared_ptr<Camera>> getCameras();
};

} // end of namespace

#endif
