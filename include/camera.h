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
#include <glm/glm.hpp>
#include <gsl/gsl_deriv.h>
#include <gsl/gsl_multimin.h>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "./config.h"

#include "./attribute.h"
#include "./coretypes.h"
#include "./geometry.h"
#include "./image.h"
#include "./object.h"
#include "./texture_image.h"

namespace Splash
{

/*************/
class Camera : public BaseObject
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Camera(RootObject* root);

    /**
     * \brief Destructor
     */
    ~Camera() override;

    /**
     * No copy constructor, but a move one
     */
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    /**
     * \brief Tessellate the objects for this camera
     */
    void blendingTessellateForCurrentCamera();

    /**
     * \brief Compute the blending for all objects seen by this camera
     */
    void computeBlendingContribution();

    /**
     * \brief Compute the vertex visibility for all objects visible by this camera
     */
    void computeVertexVisibility();

    /**
     * \brief Get the projection matrix
     * \return Return the projection matrix
     */
    glm::dmat4 computeProjectionMatrix();

    /**
     * \brief Get the projection matrix given the fov and shift (0.5 meaning no shift, 0.0 and 1.0 meaning 100% on one direction or the other)
     * \param fov Field of view, in degrees
     * \param cx Center shift along X
     * \param cy Center shift along Y
     * \return Return the projection matrix
     */
    glm::dmat4 computeProjectionMatrix(float fov, float cx, float cy);

    /**
     * \brief Get the view matrix
     * \return Return the view matrix
     */
    glm::dmat4 computeViewMatrix();

    /**
     * \brief Compute the calibration given the calibration points
     * \return Return true if all went well
     */
    bool doCalibration();

    /**
     * \brief Add one of the core models to the next redraw, with the given transformation matrix
     * \param modelName Name of the model, as known in the _models map
     * \param rtMatrix Model matrix
     */
    void drawModelOnce(const std::string& modelName, const glm::dmat4& rtMatrix);

    /**
     * \brief Get the output textures for this camera
     * \return Return a vector of pointers to the output textures
     */
    std::vector<std::shared_ptr<Texture_Image>> getTextures() const { return _outTextures; }

    /**
     * \brief Check whether the camera is initialized
     * \return Return true if the camera is initialized
     */
    bool isInitialized() const { return _isInitialized; }

    /**
     * \brief Try to link the given BaseObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkTo(const std::shared_ptr<BaseObject>& obj) override;

    /**
     * \brief Try to unlink the given BaseObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkFrom(const std::shared_ptr<BaseObject>& obj) override;

    /**
     * \brief Get the coordinates of the closest vertex to the given point
     * \param x Target x coordinate
     * \param y Target y coordinate
     * \return Return the coordinates of the closest vertex, or an empty Values if no vertex is close enough
     */
    Values pickVertex(float x, float y);

    /**
     * \brief Get the coordinates of the given fragment (world coordinates). Also returns its depth in camera space
     * \param x Target x coordinate
     * \param y Target y coordinate
     * \param fragDepth Fragment depth
     * \return Return the world coordinate of the point represented by the fragment
     */
    Values pickFragment(float x, float y, float& fragDepth);

    /**
     * \brief Get the coordinates of the closest calibration point
     * \param x Target x coordinate
     * \param y Target y coordinate
     * \return Return the closest calibration point, or an empty Values if no point is close enough
     */
    Values pickCalibrationPoint(float x, float y);

    /**
     * \brief Pick the closest calibration point or vertex
     * \param x Target x coordinate
     * \param y Target y coordinate
     * \return Return the closest point or vertex, or an empty Values if none is close enough
     */
    Values pickVertexOrCalibrationPoint(float x, float y);

    /**
     * \brief Render this camera into its textures
     */
    void render() override;

    /**
     * \brief Set the given calibration point. This point is then selected
     * \return Return true if the point has been added or if it already existed
     */
    bool addCalibrationPoint(const Values& worldPoint);

    /**
     * \brief Deselect the current calibration point
     */
    void deselectCalibrationPoint();

    /**
     * \brief Move the selected calibration point
     * \param dx Displacement along X
     * \param dy Displacement along Y
     */
    void moveCalibrationPoint(float dx, float dy);

    /**
     * \brief Remove the given calibration point
     * \param point Point to remove
     * \param unlessSet If true, does not remove the point if it is set
     */
    void removeCalibrationPoint(const Values& point, bool unlessSet = false);

    /**
     * \brief Set the selected calibration point
     * \param screenPoint Desired projected position of this calibration point
     * \return Return true if all went well
     */
    bool setCalibrationPoint(const Values& screenPoint);

    /**
     * \brief Set the number of output buffers for this camera
     * \param nbr Number of outputs
     */
    void setOutputNbr(int nbr);

    /**
     * \brief Set the resolution of this camera
     * \param width Width of the output textures
     * \param height Height of the output textures
     */
    void setOutputSize(int width, int height);

  private:
    bool _isInitialized{false};
    std::shared_ptr<GlWindow> _window;

    GLuint _fbo{0};
    std::shared_ptr<Texture_Image> _depthTexture;
    std::vector<std::shared_ptr<Texture_Image>> _outTextures;
    std::vector<std::weak_ptr<Object>> _objects;

    // Rendering parameters
    bool _drawFrame{false};
    bool _wireframe{false};
    bool _showCameraCount{false};
    bool _hidden{false};
    bool _flashBG{false};
    bool _automaticResize{true};
    bool _render16bits{false};
    bool _updateColorDepth{false}; // Set to true if the _render16bits has been updated
    glm::dvec4 _clearColor{0.6, 0.6, 0.6, 1.0};
    glm::dvec4 _wireframeColor{1.0, 1.0, 1.0, 1.0};

    // Color correction
    Values _colorLUT{0};
    bool _isColorLUTActivated{false};
    glm::mat3 _colorMixMatrix;

    // Some default models use in various situations
    std::list<std::shared_ptr<Mesh>> _modelMeshes;
    std::unordered_map<std::string, std::shared_ptr<Object>> _models;

    // Camera parameters
    float _fov{35.f};                      //!< Vertical FOV
    float _width{512.f}, _height{512.f};   //!< Current width and height
    float _newWidth{0.f}, _newHeight{0.f}; //!< New width and height
    float _near{0.1f}, _far{100.0f};       //!< Near and far parameters
    float _cx{0.5f}, _cy{0.5f};            //!< Relative position of the lens center
    glm::dvec3 _eye{1.0, 0.0, 5.0};        //!< Camera position
    glm::dvec3 _target{0.0, 0.0, 0.0};     //!< Camera target
    glm::dvec3 _up{0.0, 0.0, 1.0};         //!< Camera up vector
    float _blendWidth{0.05f};              //!< Width of the blending, as a fraction of the width and height
    float _blendPrecision{0.1f};           //!< Controls the tessellation level for the blending
    float _brightness{1.f};                //!< Brightness correction
    float _colorTemperature{6500.f};       //!< Color temperature correction
    bool _weightedCalibrationPoints{true}; //!< If true, calibration points closer to the borders have a higher influence on the calibration

    // Calibration parameters
    bool _calibrationCalledOnce{false};
    bool _displayCalibration{false};
    bool _displayAllCalibrations{false};
    bool _showAllCalibrationPoints{false};
    struct CalibrationPoint
    {
        CalibrationPoint() {}
        CalibrationPoint(glm::dvec3 w)
        {
            world = w;
            screen = glm::dvec2(0.f, 0.f);
        }
        CalibrationPoint(glm::dvec3 w, glm::dvec2 s)
        {
            world = w;
            screen = s;
        }
        glm::dvec3 world;
        glm::dvec2 screen;
        bool isSet{false};
        float weight{1.f};
    };
    std::vector<CalibrationPoint> _calibrationPoints;
    int _selectedCalibrationPoint{-1};

    //! List of additional objects to draw
    struct Drawable
    {
        Drawable(const std::string& name, glm::dmat4 mat)
            : model(name)
            , rtMatrix(mat)
        {
        }

        std::string model;
        glm::dmat4 rtMatrix;
    };
    std::list<Drawable> _drawables;

    // Function used for the calibration (camera parameters optimization)
    static double cameraCalibration_f(const gsl_vector* v, void* params);

    /**
     * \brief Init function called in constructors
     */
    void init();

    /**
     * \brief Load some defaults models, like the locator for calibration
     */
    void loadDefaultModels();

    /**
     * \brief Send calibration points to the model
     */
    void sendCalibrationPointsToObjects();

    /**
     * \brief Update the color depth for all textures
     */
    void updateColorDepth();

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_CAMERA_H
