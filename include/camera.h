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

#ifndef CAMERA_H
#define CAMERA_H

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

#include "image_shmdata.h"
#include "object.h"
#include "texture.h"

namespace Splash {

class Camera : public BaseObject
{
    public:
        /**
         * Constructor
         */
        Camera(GlWindowPtr w);

        /**
         * Destructor
         */
        ~Camera();

        /**
         * No copy constructor, but a move one
         */
        Camera(const Camera&) = delete;
        Camera(Camera&& c)
        {
            _isInitialized = c._isInitialized;
            _window = c._window;
            _fbo = c._fbo;
            _outTextures = c._outTextures;
            _objects = c._objects;

            _fov = c._fov;
            _width = c._width;
            _height = c._height;
            _near = c._near;
            _far = c._far;
            _eye = c._eye;
            _target = c._target;
        }

        /**
         * Add an object to render
         */
        void addObject(ObjectPtr& obj);

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
         * Try to link the given BaseObject to this
         */
        bool linkTo(BaseObjectPtr obj);

        /**
         * Get the coordinates of the closest vertex to the given point
         */
        std::vector<Value> pickVertex(float x, float y);

        /**
         * Get the coordinates of the closest calibration point
         */
        std::vector<Value> pickCalibrationPoint(float x, float y);

        /**
         * Render this camera into its textures
         */
        bool render();

        /**
         * Set the given calibration point
         * Returns true if the point already existed
         */
        bool addCalibrationPoint(std::vector<Value> worldPoint);
        void deselectCalibrationPoint();
        void moveCalibrationPoint(float dx, float dy);
        void removeCalibrationPoint(std::vector<Value> worldPoint, bool unlessSet = false);
        bool setCalibrationPoint(std::vector<Value> screenPoint);

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

        GLuint _fbo;
        TexturePtr _depthTexture;
        std::vector<TexturePtr> _outTextures;
        std::vector<ObjectPtr> _objects;
        Image_ShmdataPtr _outShm;

        // Objects for copying to host
        bool _writeToShm {false};
        GLuint _pbos[2];
        int _pboReadIndex {0};

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
        float _near {0.1}, _far {100.0};
        float _cx {0.5}, _cy {0.5};
        glm::vec3 _eye {1.0, 0.0, 5.0};
        glm::vec3 _target {0.0, 0.0, 0.0};
        glm::vec3 _up {0.0, 0.0, 1.0};
        float _blendWidth {0.05f}; // Width of the blending, as a fraction of the width and height

        // Calibration parameters
        bool _displayCalibration {false};
        bool _showAllCalibrationPoints {false};
        struct CalibrationPoint
        {
            CalibrationPoint(glm::vec3 w) {world = w; screen = glm::vec2(0.f, 0.f);}
            CalibrationPoint(glm::vec3 w, glm::vec2 s) {world = w; screen = s;}
            glm::vec3 world;
            glm::vec2 screen;
            bool isSet {false};
        };
        std::vector<CalibrationPoint> _calibrationPoints;
        int _selectedCalibrationPoint {-1};
        
        // Type used by GSL for calibration
        struct GslParam
        {
            Camera* context;
            bool setExtrinsic {false};
        };
        // Function used for the calibration (camera parameters optimization)
        static double cameraCalibration_f(const gsl_vector* v, void* params);

        /**
         * Get the frustum matrix from the current camera parameters
         */
        glm::mat4x4 computeProjectionMatrix();

        /**
         * Get the view projection matrix from the camera parameters
         */
        glm::mat4x4 computeViewProjectionMatrix();

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

#endif // CAMERA_H
