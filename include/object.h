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

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "geometry.h"
#include "gpuBuffer.h"
#include "shader.h"
#include "texture.h"

namespace Splash {

class Object : public BaseObject
{
    public:
        /**
         * Constructor
         */
        Object();
        Object(std::weak_ptr<RootObject> root);

        /**
         * Destructor
         */
        ~Object();

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
         */
        void computeCameraContribution(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float blendWidth);

        /**
         * Deactivate this object for rendering
         */
        void deactivate();

        /**
         * Add a geometry to this object
         */
        void addGeometry(const std::shared_ptr<Geometry>& geometry) {_geometries.push_back(geometry);}

        /**
         * Add a texture to this object
         */
        void addTexture(const std::shared_ptr<Texture>& texture) {_textures.push_back(texture);}

        /**
         * Add and remove a calibration point
         */
        void addCalibrationPoint(glm::dvec3 point);
        void removeCalibrationPoint(glm::dvec3 point);

        /**
         * Draw the object
         */
        void draw();

        /**
         * Get a reference to all the calibration points set
         */
        std::vector<glm::dvec3>& getCalibrationPoints() {return _calibrationPoints;}

        /**
         * Get the model matrix
         */
        glm::dmat4 getModelMatrix() const {return computeModelMatrix();}

        /**
         * Get the shader
         */
        std::shared_ptr<Shader> getShader() const {return _shader;}

        /**
         * Try to link the given BaseObject to this
         */
        bool linkTo(std::shared_ptr<BaseObject> obj);
        void unlinkFrom(std::shared_ptr<BaseObject> obj);

        /**
         * Get the coordinates of the closest vertex to the given point
         */
        float pickVertex(glm::dvec3 p, glm::dvec3& v);

        /**
         * Remove a geometry from this object
         */
        void removeGeometry(const std::shared_ptr<Geometry>& geometry);

        /**
         * Remove a texture from this object
         */
        void removeTexture(const std::shared_ptr<Texture>& texture);

        /**
         * Reset the blending to no blending at all
         */
        void resetBlendingMap();

        /**
         * Reset tessellation of all linked objects
         */
        void resetTessellation();

        /**
         * Reset the visibility flag, as well as the faces ID
         */
        void resetVisibility();

        /**
         * Reset the attribute holding the number of camera and the blending value
         */
        void resetBlendingAttribute();

        /**
         * Set the blending map for the object
         */
        void setBlendingMap(const std::shared_ptr<Texture>& map);

        /**
         * Set the shader
         */
        void setShader(const std::shared_ptr<Shader>& shader) {_shader = shader;}

        /**
         * Set the view projection matrix
         */
        void setViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp);

        /**
         * Set the model matrix. This overrides the position attribute
         */
        void setModelMatrix(const glm::dmat4& model) {_modelMatrix = model;}

        /**
         * Subdivide the objects wrt the given camera limits (for blending purposes)
         */
        void tessellateForThisCamera(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float fovX, float fovY, float blendWidth, float blendPrecision);

        /**
         * This transfers the visibility from the texture active as GL_TEXTURE0 to the vertices attributes
         */
        void transferVisibilityFromTexToAttr(int width, int height);

    private:
        mutable std::mutex _mutex;

        std::shared_ptr<Shader> _shader {};
        std::shared_ptr<Shader> _computeShaderResetVisibility {};
        std::shared_ptr<Shader> _computeShaderResetBlendingAttributes {};
        std::shared_ptr<Shader> _computeShaderComputeBlending {};
        std::shared_ptr<Shader> _computeShaderTransferVisibilityToAttr {};
        std::shared_ptr<Shader> _feedbackShaderSubdivideCamera {};

        // A map for previously used graphics shaders
        std::map<std::string, std::shared_ptr<Shader>> _graphicsShaders;

        std::vector<std::shared_ptr<Texture>> _textures;
        std::vector<std::shared_ptr<Geometry>> _geometries;
        std::vector<std::shared_ptr<Texture>> _blendMaps;

        bool _vertexBlendingActive {false};

        glm::dvec3 _position {0.0, 0.0, 0.0};
        glm::dvec3 _rotation {0.0, 0.0, 0.0};
        glm::dvec3 _scale {1.0, 1.0, 1.0};
        glm::dmat4 _modelMatrix;

        std::string _fill {"texture"};
        int _sideness {0};
        glm::dvec4 _color {0.0, 1.0, 0.0, 1.0};
        float _normalExponent {0.0};

        // A copy of all the cameras' calibration points,
        // for display purposes. These are not saved
        std::vector<glm::dvec3> _calibrationPoints;

        /**
         * Init function called by constructor
         */
        void init();

        /**
         * Compute the matrix corresponding to the object position
         */
        glm::dmat4 computeModelMatrix() const;

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

} // end of namespace

#endif // SPLASH_OBJECT_H
