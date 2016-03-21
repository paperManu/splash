/*
 * Copyright (C) 2016 Emmanuel Durand
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
 * @mesh_bezierPatch.h
 * The Mesh_BezierPatch_BezierPatch class
 */

#ifndef SPLASH_MESH_BEZIERPATCH_H
#define SPLASH_MESH_BEZIERPATCH_H

#include <chrono>
#include <memory>
#include <mutex>
#include <vector>
#include <glm/glm.hpp>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "mesh.h"

namespace Splash {

class Mesh_BezierPatch : public Mesh
{
    public:
        /**
         * Constructor
         */
        Mesh_BezierPatch();
        Mesh_BezierPatch(bool linkedToWorld); //< This constructor is used if the object is linked to a World counterpart

        /**
         * Destructor
         */
        virtual ~Mesh_BezierPatch();

        /**
         * No copy constructor, but a copy operator
         */
        Mesh_BezierPatch(const Mesh_BezierPatch&) = delete;
        Mesh_BezierPatch& operator=(const Mesh_BezierPatch&) = delete;
        Mesh_BezierPatch& operator=(Mesh_BezierPatch&&) = default;

        /**
         * Select the bezier mesh or the control points as the mesh to output
         */
        void switchMeshes(bool control);

        /**
         * Update the content of the mesh
         */
        virtual void update();

    private:
        struct Patch
        {
            std::vector<glm::vec2> vertices;
            std::vector<glm::vec2> uvs;
        };

        Patch _patch {};
        glm::vec2 _size {3, 3};
        int _patchResolution {8};

        bool _patchUpdated {true};
        MeshContainer _bezierControl;
        MeshContainer _bezierMesh;

        // Factorial
        inline uint32_t factorial(uint32_t i)
        {
            return (i == 0 || i == 1) ? 1 : factorial(i - 1) * i;
        }

        // Binomial coefficient
        inline uint32_t binomialCoeff(uint32_t n, uint32_t i)
        {
            if (n > i)
                return 0;
            return factorial(n) / (factorial(i) * factorial(n - i));
        }

        void init();

        /**
         * Create a patch
         */
        void createPatch(int width = 4, int height = 4);

        /**
         * Update the underlying mesh from the patch control points
         */
        void updatePatch();
        
        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

} // end of namespace

#endif // SPLASH_MESH_BEZIERPATCH_H
