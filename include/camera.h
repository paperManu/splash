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
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @camera.h
 * The Camera base class
 */

#ifndef SPLASH_CAMERA_H
#define SPLASH_CAMERA_H

#include <functional>
#include <unordered_map>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <gsl/gsl_deriv.h>
#include <gsl/gsl_multimin.h>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "image.h"
#include "geometry.h"
#include "object.h"
#include "texture_image.h"

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

        /**
         * Computes the blending map for this camera
         */
        void computeBlendingMap(ImagePtr& map);

        /**
         * Compute the blending for all objects seen by this camera
         */
        void computeBlendingContribution();

        /**
         * Compute the vertex visibility for all objects in front of this camera
         */
        void computeVertexVisibility();

        /**
         * Tessellate the objects for the given camera
         */
        void blendingTessellateForCurrentCamera();

        /**
         * Compute the calibration given the calibration points
         */
        bool doCalibration();

        /**
         * Get pointers to this camera textures
         */
        std::vector<Texture_ImagePtr> getTextures() const {return _outTextures;}

        /**
         * Check wether it is initialized
         */
        bool isInitialized() const {return _isInitialized;}

        /**
         * Try to link / unlink the given BaseObject to this
         */
        bool linkTo(std::shared_ptr<BaseObject> obj);
        bool unlinkFrom(std::shared_ptr<BaseObject> obj);

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
         * Pick the closest calibration point or vertex
         */
        Values pickVertexOrCalibrationPoint(float x, float y);

        /**
         * Render this camera into its textures
         */
        bool render();

        /**
         * Set the given calibration point
         * Returns true if the point already existed
         */
        bool addCalibrationPoint(const Values& worldPoint);
        void deselectCalibrationPoint();
        void moveCalibrationPoint(float dx, float dy);
        void removeCalibrationPoint(const Values& point, bool unlessSet = false);
        bool setCalibrationPoint(const Values& screenPoint);

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
        Texture_ImagePtr _depthTexture;
        std::vector<Texture_ImagePtr> _outTextures;
        std::vector<std::weak_ptr<Object>> _objects;

        // Rendering parameters
        bool _drawFrame {false};
        bool _wireframe {false};
        bool _hidden {false};
        bool _flashBG {false};
        bool _automaticResize {true};
        glm::dvec4 _clearColor {0.6, 0.6, 0.6, 1.0};

        // Color correction
        Values _colorLUT {0};
        bool _isColorLUTActivated {false};
        glm::mat3 _colorMixMatrix;

        // Some default models use in various situations
        std::list<std::shared_ptr<Mesh>> _modelMeshes;
        std::list<std::shared_ptr<Geometry>> _modelGeometries;
        std::unordered_map<std::string, std::shared_ptr<Object>> _models;

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
        float _blendPrecision {0.1f}; // Controls the tessellation level for the blending
        float _blackLevel {0.f};
        float _brightness {1.f};
        float _colorTemperature {6500.f};

        // Calibration parameters
        bool _calibrationCalledOnce {false};
        bool _displayCalibration {false};
        bool _displayAllCalibrations {false};
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
         * Send calibration points to the model
         */
        void sendCalibrationPointsToObjects();

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Camera> CameraPtr;

} // end of namespace

#endif // SPLASH_CAMERA_H
