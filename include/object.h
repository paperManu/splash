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
        Object(RootObjectWeakPtr root);

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
        void computeVisibility(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float blendWidth);

        /**
         * Deactivate this object for rendering
         */
        void deactivate();

        /**
         * Add a geometry to this object
         */
        void addGeometry(const GeometryPtr geometry) {_geometries.push_back(geometry);}

        /**
         * Add a texture to this object
         */
        void addTexture(const TexturePtr texture) {_textures.push_back(texture);}

        /**
         * Draw the object
         */
        void draw();

        /**
         * Get the model matrix
         */
        glm::dmat4 getModelMatrix() const {return computeModelMatrix();}

        /**
         * Get the shader
         */
        ShaderPtr getShader() const {return _shader;}

        /**
         * Read back the raw geometries
         * (either the original or the alternative, tessellated version)
         */
        std::vector<std::vector<std::vector<char>>> getRawGeometries(bool alternative);
        std::map<std::string, std::unique_ptr<SerializedObject>> getGeometriesAsSerializedMeshes(bool alternative);

        /**
         * Try to link the given BaseObject to this
         */
        bool linkTo(BaseObjectPtr obj);

        /**
         * Get the coordinates of the closest vertex to the given point
         */
        float pickVertex(glm::dvec3 p, glm::dvec3& v);

        /**
         * Remove a texture from this object
         */
        void removeTexture(const TexturePtr texture);

        /**
         * Reset the blending to no blending at all
         */
        void resetBlendingMap();

        /**
         * Reset tessellation of all linked objects
         */
        void resetTessellation();

        /**
         * Reset computed visibility from any camera
         */
        void resetVisibility();

        /**
         * Set the blending map for the object
         */
        void setBlendingMap(TexturePtr map);

        /**
         * Set the geometries from the given buffers
         */
        void setRawGeometries(const std::vector<std::vector<std::vector<char>>>& buffers);

        /**
         * Set the shader
         */
        void setShader(const ShaderPtr shader) {_shader = shader;}

        /**
         * Set the view projection matrix
         */
        void setViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp);

        /**
         * Subdivide the objects wrt the given camera limits (for blending purposes)
         */
        void tessellateForThisCamera(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float blendWidth, float blendPrecision);

    private:
        mutable std::mutex _mutex;

        ShaderPtr _shader;
        ShaderPtr _computeShaderResetBlending;
        ShaderPtr _computeShaderComputeBlending;
        ShaderPtr _feedbackShaderSubdivideCamera;

        std::vector<TexturePtr> _textures;
        std::vector<GeometryPtr> _geometries;
        std::vector<TexturePtr> _blendMaps;

        bool _vertexBlendingActive {false};

        glm::dvec3 _position {0.0, 0.0, 0.0};
        glm::dvec3 _scale {1.0, 1.0, 1.0};

        std::string _fill {"texture"};
        int _sideness {0};

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

typedef std::shared_ptr<Object> ObjectPtr;

} // end of namespace

#endif // SPLASH_OBJECT_H
