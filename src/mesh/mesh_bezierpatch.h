/*
 * Copyright (C) 2016 Splash authors
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
 * @mesh_bezierpatch.h
 * The Mesh_BezierPatch_BezierPatch class
 */

#ifndef SPLASH_MESH_BEZIERPATCH_H
#define SPLASH_MESH_BEZIERPATCH_H

#include <chrono>
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <vector>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./mesh/mesh.h"

namespace Splash
{

class Mesh_BezierPatch final : public Mesh
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    explicit Mesh_BezierPatch(RootObject* root);

    /**
     * Destructor
     */
    ~Mesh_BezierPatch() final = default;

    /**
     * Constructors/operators
     */
    Mesh_BezierPatch(const Mesh_BezierPatch&) = delete;
    Mesh_BezierPatch& operator=(const Mesh_BezierPatch&) = delete;
    Mesh_BezierPatch(Mesh_BezierPatch&&) = delete;
    Mesh_BezierPatch& operator=(Mesh_BezierPatch&&) = delete;

    /**
     * Get the list of the control points
     * \return Returns the list
     */
    std::vector<glm::vec2> getControlPoints() const { return _patch.vertices; }

    /**
     * Get whether the Bezier patch is the default one (meaning no deformation/warping),
     * or if it has been modified
     * \return Return true if the Bezier patch has been modified
     */
    inline bool isPatchModified() const { return _patchModified; }

    /**
     * Select the bezier mesh or the control points as the mesh to output
     * \param control If true, selects the control points
     */
    void switchMeshes(bool control);

    /**
     * Update the content of the mesh
     */
    void update() final;

  private:
    struct Patch
    {
        glm::ivec2 size{0, 0};
        std::vector<glm::vec2> vertices{};
        std::vector<glm::vec2> uvs{};
    };

    Patch _patch{};
    int _patchResolution{64};
    std::mutex _patchMutex{};

    bool _patchModified{false}; //!< False if the patch control points are not the default ones anymore
    bool _patchUpdated{true};   //!< True if the patch control points have been changed and the mesh needs updating
    MeshContainer _bezierControl;
    MeshContainer _bezierMesh;

    std::vector<float> _binomialCoeffsX{};
    std::vector<float> _binomialCoeffsY{};
    glm::ivec2 _binomialDimensions{0, 0};

    // Factorial
    inline int32_t factorial(int32_t i) { return (i == 0 || i == 1) ? 1 : factorial(i - 1) * i; }

    // "Semi" factorial
    inline int32_t factorial(int32_t n, int32_t k)
    {
        int32_t res = 1;
        for (int32_t i = n - k + 1; i <= n; ++i)
            res *= i;
        return res;
    }

    // Binomial coefficient
    inline int32_t binomialCoeff(int32_t n, int32_t i)
    {
        if (n < i)
            return 0;
        return factorial(n, i) / factorial(i);
    }

    /**
     * Create a patch
     * \param width Horizontal control point count
     * \param height Vertical control point count
     * \return Return the patch
     */
    Patch createPatch(int width = 4, int height = 4);

    /**
     * Create Bezier control points from a patch
     * \param patch Patch description
     */
    void updateBezierFromPatch(Patch& patch);

    /**
     * Update the underlying mesh from the patch control points
     */
    void updatePatch();

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_MESH_BEZIERPATCH_H
