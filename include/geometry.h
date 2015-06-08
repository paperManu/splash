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
 * @texture.h
 * The Geometry base class
 */

#ifndef SPLASH_GEOMETRY_H
#define SPLASH_GEOMETRY_H

#include <chrono>
#include <map>
#include <vector>
#include <glm/glm.hpp>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "gpuBuffer.h"
#include "mesh.h"

namespace Splash {

class Geometry : public BaseObject
{
    public:
        /**
         * Constructor
         */
        Geometry();

        /**
         * Destructor
         */
        ~Geometry();

        /**
         * No copy constructor, but a move one
         */
        Geometry(const Geometry&) = delete;
        Geometry& operator=(const Geometry&) = delete;

        /**
         * Activate the geometry for rendering
         */
        void activate();
        void activateAsSharedBuffer();

        /**
         * Activate the geomtry for feedback into the alternative buffers
         */
        void activateForFeedback();

        /**
         * Deactivate the geometry for rendering
         */
        void deactivate() const;

        /**
         * Deactivate for feedback
         */
        void deactivateFeedback();

        /**
         * Get the number of vertices for this geometry
         */
        int getVerticesNumber() const {return _useAlternativeBuffers ? _alternativeVerticesNumber : _verticesNumber;}

        /**
         * Try to link the given BaseObject to this
         */
        bool linkTo(BaseObjectPtr obj);

        /**
         * Get the coordinates of the closest vertex to the given point
         */
        float pickVertex(glm::dvec3 p, glm::dvec3& v);

        /**
         * Replace one of the GL buffers with an alternative one
         */
        void setAlternativeBuffer(std::shared_ptr<GpuBuffer> buffer, int index);

        /**
         * Specify the number of vertices to draw
         */
        void setAlternativeVerticesNumber(unsigned int nbr) {_alternativeVerticesNumber = nbr;}

        /**
         * Deactivate the specified alternative buffer (and re-use the default one
         */
        void resetAlternativebuffer(int index = -1);

        /**
         * Set the mesh for this object
         */
        void setMesh(MeshPtr mesh) {_mesh = mesh;}

        /**
         * Updates the object
         */
        void update();

        /**
         * Activate alternative buffers for draw
         */
        void useAlternativeBuffers(bool isActive);

    private:
        mutable std::mutex _mutex;

        MeshPtr _mesh;
        std::chrono::high_resolution_clock::time_point _timestamp;

        std::map<GLFWwindow*, GLuint> _vertexArray;
        std::vector<std::shared_ptr<GpuBuffer>> _glBuffers {};
        std::vector<std::shared_ptr<GpuBuffer>> _glAlternativeBuffers {};
        bool _buffersDirty {false};
        bool _useAlternativeBuffers {false};

        int _verticesNumber {0};
        int _alternativeVerticesNumber {0};
        int _alternativeBufferSize {0};

        // Transform feedback
        GLuint _feedbackQuery;
        int _feedbackNbrPrimitives {0};

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Geometry> GeometryPtr;

} // end of namespace

#endif // SPLASH_GEOMETRY_H
