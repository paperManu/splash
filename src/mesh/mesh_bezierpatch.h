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

#include "./config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./mesh/mesh.h"

namespace Splash
{

class Mesh_BezierPatch : public Mesh
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Mesh_BezierPatch(RootObject* root);

    /**
     * \brief Destructor
     */
    virtual ~Mesh_BezierPatch() final;

    /**
     * No copy constructor, but a copy operator
     */
    Mesh_BezierPatch(const Mesh_BezierPatch&) = delete;
    Mesh_BezierPatch& operator=(const Mesh_BezierPatch&) = delete;
    Mesh_BezierPatch& operator=(Mesh_BezierPatch&&) = default;

    /**
     * \brief Get the list of the control points
     * \return Returns the list
     */
    std::vector<glm::vec2> getControlPoints() const { return _patch.vertices; }

    /**
     * \brief Select the bezier mesh or the control points as the mesh to output
     * \param control If true, selects the control points
     */
    void switchMeshes(bool control);

    /**
     * \brief Update the content of the mesh
     */
    virtual void update() final;

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

    bool _patchUpdated{true};
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
     * \brief Initialization
     */
    void init();

    /**
     * \brief Create a patch
     * \param width Horizontal control point count
     * \param height Vertical control point count
     * \param patch Patch description
     */
    void createPatch(int width = 4, int height = 4);
    void createPatch(Patch& patch);

    /**
     * \brief Update the underlying mesh from the patch control points
     */
    void updatePatch();

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_MESH_BEZIERPATCH_H
