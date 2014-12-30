/*
 * Copyright (C) 2013 Emmanuel Durand
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
 * @camera.h
 * The Camera base class
 */

#ifndef SPLASH_CAMERA_H
#define SPLASH_CAMERA_H

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#include "config.h"
#include "coretypes.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <gsl/gsl_deriv.h>
#include <gsl/gsl_multimin.h>
#include <GLFW/glfw3.h>

namespace Splash {

/*************/
class Camera : public BaseObject
{
    public:
        /**
         * Constructor
         */
        Camera(RootObjectWeakPtr root);

        /**
         * Destructor
         */
        ~Camera();

        /**
         * No copy constructor, but a move one
         */
        Camera(const Camera&) = delete;
        Camera& operator=(const Camera&) = delete;

        Camera(Camera&& c)
        {
            *this = std::move(c);
        }

        Camera& operator=(Camera&& c)
        {
            if (this != &c)
            {
                _isInitialized = c._isInitialized;
                _window = c._window;

                _fbo = c._fbo;
                _outTextures = c._outTextures;
                _objects = c._objects;

                _drawFrame = c._drawFrame;
                _wireframe = c._wireframe;
                _hidden = c._hidden;
                _flashBG = c._flashBG;

                _models = c._models;

                _fov = c._fov;
                _width = c._width;
                _height = c._height;
                _near = c._near;
                _far = c._far;
                _cx = c._cx;
                _cy = c._cy;
                _eye = c._eye;
                _target = c._target;
                _up = c._up;
                _blendWidth = c._blendWidth;
                _blackLevel = c._blackLevel;
                _brightness = c._brightness;

                _displayCalibration = c._displayCalibration;
                _showAllCalibrationPoints = c._showAllCalibrationPoints;

                _calibrationPoints = c._calibrationPoints;
                _selectedCalibrationPoint = c._selectedCalibrationPoint;
            }
            return *this;
        }

        /**
         * Computes the blending map for this camera
         */
        void computeBlendingMap(ImagePtr& map);

        /**
         * Compute the calibration given the calibration points
         */
        bool doCalibration();

        /**
         * Get pointers to this camera textures
         */
        std::vector<TexturePtr> getTextures() const {return _outTextures;}

        /**
         * Check wether it is initialized
         */
        bool isInitialized() const {return _isInitialized;}

        /**
         * Try to link / unlink the given BaseObject to this
         */
        bool linkTo(BaseObjectPtr obj);
        bool unlinkFrom(BaseObjectPtr obj);

        /**
         * Get the coordinates of the closest vertex to the given point
         */
        Values pickVertex(float x, float y);

        /**
         * Get the coordinates of the given fragment (world coordinates)
         */
        Values pickFragment(float x, float y);

        /**
         * Get the coordinates of the closest calibration point
         */
        Values pickCalibrationPoint(float x, float y);

        /**
         * Render this camera into its textures
         */
        bool render();

        /**
         * Set the given calibration point
         * Returns true if the point already existed
         */
        bool addCalibrationPoint(Values worldPoint);
        void deselectCalibrationPoint();
        void moveCalibrationPoint(float dx, float dy);
        void removeCalibrationPoint(Values worldPoint, bool unlessSet = false);
        bool setCalibrationPoint(Values screenPoint);

        /**
         * Set the number of output buffers for this camera
         */
        void setOutputNbr(int nbr);

        /**
         * Set the resolution of this camera
         */
        void setOutputSize(int width, int height);

    private:
        bool _isInitialized {false};
        GlWindowPtr _window;

        GLuint _fbo {0};
        TexturePtr _depthTexture;
        std::vector<TexturePtr> _outTextures;
        std::vector<ObjectPtr> _objects;

        // Rendering parameters
        bool _drawFrame {false};
        bool _wireframe {false};
        bool _hidden {false};
        bool _flashBG {false};

        // Some default models use in various situations
        std::map<std::string, ObjectPtr> _models;

        // Camera parameters
        float _fov {35}; // This is the vertical FOV
        float _width {512}, _height {512};
        float _newWidth {0}, _newHeight {0};
        float _near {0.1}, _far {100.0};
        float _cx {0.5}, _cy {0.5};
        glm::dvec3 _eye {1.0, 0.0, 5.0};
        glm::dvec3 _target {0.0, 0.0, 0.0};
        glm::dvec3 _up {0.0, 0.0, 1.0};
        float _blendWidth {0.05f}; // Width of the blending, as a fraction of the width and height
        float _blackLevel {0.f};
        float _brightness {1.f};
        float _colorTemperature {6500.f};

        // Calibration parameters
        bool _calibrationCalledOnce {false};
        bool _displayCalibration {false};
        bool _showAllCalibrationPoints {false};
        struct CalibrationPoint
        {
            CalibrationPoint() {}
            CalibrationPoint(glm::dvec3 w) {world = w; screen = glm::dvec2(0.f, 0.f);}
            CalibrationPoint(glm::dvec3 w, glm::dvec2 s) {world = w; screen = s;}
            glm::dvec3 world;
            glm::dvec2 screen;
            bool isSet {false};
        };
        std::vector<CalibrationPoint> _calibrationPoints;
        int _selectedCalibrationPoint {-1};
        
        // Function used for the calibration (camera parameters optimization)
        static double cameraCalibration_f(const gsl_vector* v, void* params);

        /**
         * Get the color balance (r/g and b/g) from a black body temperature
         */
        glm::vec2 colorBalanceFromTemperature(float temp);

        /**
         * Get the frustum matrix from the current camera parameters
         */
        glm::dmat4 computeProjectionMatrix();
        glm::dmat4 computeProjectionMatrix(float fov, float cx, float cy);

        /**
         * Get the view matrix from the camera parameters
         */
        glm::dmat4 computeViewMatrix();

        /**
         * Init function called in constructors
         */
        void init();

        /**
         * Load some defaults models, like the locator for calibration
         */
        void loadDefaultModels();

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Camera> CameraPtr;

} // end of namespace

#endif // SPLASH_CAMERA_H
