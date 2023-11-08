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
 * @object.h
 * The Object class
 */

#ifndef SPLASH_OBJECT_H
#define SPLASH_OBJECT_H

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/graph_object.h"
#include "./graphics/api/gpu_buffer.h"
#include "./graphics/geometry.h"
#include "./graphics/shader.h"
#include "./graphics/texture.h"

namespace Splash
{

class Object : public GraphObject
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    Object(RootObject* root);

    /**
     * Destructor
     */
    ~Object() override;

    /**
     * No copy constructor, but some moves
     */
    Object(const Object& o) = delete;
    Object& operator=(const Object& o) = delete;

    /**
     * Activate this object for rendering
     */
    void activate();

    /**
     * Compute the visibility for the mvp specified with setViewProjectionMatrix, for blending purposes
     * \param viewMatrix View matrix
     * \param projectionMatrix Projection matrix
     * \param blendWidth Width of the blending zone between projectors
     */
    void computeCameraContribution(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float blendWidth);

    /**
     * Deactivate this object for rendering
     */
    void deactivate();

    /**
     * Add a geometry to this object
     * \param geometry Geometry to add
     */
    void addGeometry(const std::shared_ptr<Geometry>& geometry) { _geometries.push_back(geometry); }

    /**
     * Add a texture to this object
     * \param texture Texture to add
     */
    void addTexture(const std::shared_ptr<Texture>& texture) { _textures.push_back(texture); }

    /**
     * Add a calibration point
     * \param point Point coordinates
     */
    void addCalibrationPoint(const glm::dvec3& point);

    /**
     * Get the update timestamp of the object, which is computed as the latest
     * timestamp from all its textures
     * \return Return the timestamp, in us
     */
    virtual int64_t getTimestamp() const final;

    /**
     * Remove a calibration point
     * \param point Point coordinates
     */
    void removeCalibrationPoint(const glm::dvec3& point);

    /**
     * Draw the object
     */
    void draw();

    /**
     * Get a reference to all the calibration points set
     * \return Return a reference to the vector containing all calibration points
     */
    inline std::vector<glm::dvec3>& getCalibrationPoints() { return _calibrationPoints; }

    /**
     * Get farthest visible vertex distance from last camera blending computation.
     * This has to be called after computeCameraContribution
     * \return Return the farthest visible vertex distance
     */
    inline float getFarthestVisibleVertexDistance() const { return _farthestVisibleVertexDistance; }

    /**
     * Get the model matrix
     * \return Return the model matrix
     */
    inline glm::dmat4 getModelMatrix() const { return computeModelMatrix(); }

    /**
     * Get the shader used for the object. This must be called while the object is active
     * \return Return the shader
     */
    inline std::shared_ptr<Shader> getShader() const { return _shader; }

    /**
     * Get the number of vertices for this object
     * \return Return the number of vertices
     */
    int getVerticesNumber() const;

    /**
     * Get the coordinates of the closest vertex to the given point
     * \param p Coordinates around which to look
     * \param v Found vertex coordinates
     * \return Return the distance between p and v
     */
    float pickVertex(glm::dvec3 p, glm::dvec3& v);

    /**
     * Remove a geometry from this object
     * \param geometry Geometry to remove
     */
    void removeGeometry(const std::shared_ptr<Geometry>& geometry);

    /**
     * Remove a texture from this object
     * \param texture Texture to remove
     */
    void removeTexture(const std::shared_ptr<Texture>& texture);

    /**
     * Reset tessellation of all linked objects
     */
    void resetTessellation();

    /**
     * Reset the visibility flag, as well as the faces ID
     * \param primitiveIdShift Shift for the ID of the vertices
     */
    void resetVisibility(int primitiveIdShift = 0);

    /**
     * Reset the attribute holding the number of camera and the blending value
     */
    void resetBlendingAttribute();

    /**
     * Set the view and projection matrices
     * \param mv View matrix
     * \[aram mp Projection matrix
     */
    void setViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp);

    /**
     * Set the model matrix. This overrides the position attribute
     * \param model Model matrix
     */
    void setModelMatrix(const glm::dmat4& model) { _modelMatrix = model; }

    /**
     * Subdivide the objects wrt the given camera limits (for blending purposes)
     * \param viewMatrix View matrix
     * \param projectionMatrix Projection matrix
     * \param fovX Horizontal FOV
     * \param fovY Vertical FOV
     * \param blendWidth Blending width
     * \param blendPrecision Blending precision
     */
    // TODO: use the matrices specified with setViewProjectionMatrix
    void tessellateForThisCamera(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float fovX, float fovY, float blendWidth, float blendPrecision);

    /**
     * This transfers the visibility from the texture active as GL_TEXTURE0 to the vertices attributes
     * \param width Width of the texture
     * \param height Height of the texture
     * \param primitiveIdShift Shift for the ID as rendered in the texture
     */
    void transferVisibilityFromTexToAttr(int width, int height, int primitiveIdShift);

    /**
     * Set the shader for this object
     */
    void setShader(const std::shared_ptr<Shader>& shader);

  protected:
    /**
     * Try to link the given GraphObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkIt(const std::shared_ptr<GraphObject>& obj) final;

    /**
     * Try to unlink the given GraphObject from this object
     * \param obj Shared pointer to the (supposed) child object
     */
    void unlinkIt(const std::shared_ptr<GraphObject>& obj) final;

  private:
    mutable std::mutex _mutex;

    std::shared_ptr<Shader> _shader{};
    std::shared_ptr<Shader> _computeShaderResetVisibility{};
    std::shared_ptr<Shader> _computeShaderResetBlendingAttributes{};
    std::shared_ptr<Shader> _computeShaderComputeBlending{};
    std::shared_ptr<Shader> _computeShaderTransferVisibilityToAttr{};
    std::shared_ptr<Shader> _feedbackShaderSubdivideCamera{};

    // A map for previously used graphics shaders
    std::map<std::string, std::shared_ptr<Shader>> _graphicsShaders;

    std::vector<std::shared_ptr<Texture>> _textures;
    std::vector<std::shared_ptr<Geometry>> _geometries;

    bool _vertexBlendingActive{false};

    glm::dvec3 _position{0.0, 0.0, 0.0};
    glm::dvec3 _rotation{0.0, 0.0, 0.0};
    glm::dvec3 _scale{1.0, 1.0, 1.0};
    glm::dmat4 _modelMatrix;

    std::string _fill{"texture"};
    std::vector<std::string> _fillParameters{};
    int _sideness{0};
    glm::dvec4 _color{0.0, 0.0, 0.0, 1.0};
    float _normalExponent{0.0};

    // Depending on the current rendering pass, this can be either:
    // - computed by computeCameraContribution, during blending computation
    // - set as a result of overall blending computation by the Blender object,
    //   and used to render the blending
    float _farthestVisibleVertexDistance{0.f};
    // If true, computes _farthestVisibleVertexDistance, otherwise sets it to 0.f
    // (when calling computeCameraContribution())
    bool _computeFarthestVisibleVertexDistance{false};

    // A copy of all the cameras' calibration points,
    // for display purposes. These are not saved
    std::vector<glm::dvec3> _calibrationPoints;

    /**
     * Init function called by constructor
     */
    void init();

    /**
     * Compute the matrix corresponding to the object position
     * \return Return the model matrix
     */
    glm::dmat4 computeModelMatrix() const;

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_OBJECT_H
