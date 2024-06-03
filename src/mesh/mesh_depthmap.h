/*
 * Copyright (C) 2024 Splash authors
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
 * @mesh_depthmap.h
 * The Mesh_Depthmap class
 */

#ifndef SPLASH_MESH_DEPTHMAP
#define SPLASH_MESH_DEPTHMAP

#include <memory>

#include "./mesh/mesh.h"

namespace Splash
{

class Image;

class Mesh_Depthmap final : public Mesh
{
  public:
    /**
     * A simple structure to describe the camera intrinsic parameters
     * All values are in pixels.
     * Default values are meant to be sensible defaults, and should allow
     * for seeing something with a 640x480 image. Do not expect more
     * unless you set the correct (or close enough) values.
     */
    struct CameraIntrinsics
    {
        float cx = 320.f;
        float cy = 240.f;
        float fx = 100.f;
        float fy = 100.f;
    };

  public:
    /**
     * Constructor
     * \param root Root object
     */
    explicit Mesh_Depthmap(RootObject* root);

    /**
     * Destructor
     */
    ~Mesh_Depthmap() final = default;

    /**
     * Constructors/operators
     */
    Mesh_Depthmap(const Mesh_Depthmap&) = delete;
    Mesh_Depthmap& operator=(const Mesh_Depthmap&) = delete;
    Mesh_Depthmap(Mesh_Depthmap&&) = delete;
    Mesh_Depthmap& operator=(Mesh_Depthmap&&) = delete;

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

    /**
     * This Mesh can not be updated from a file
     */
    bool read(const std::string&) final { return true; }

    /**
     * Update the content of the mesh
     */
    void update() final;

  private:
    struct Point
    {
        float x;
        float y;
        float z;
    };

  private:
    std::shared_ptr<Image> _depthMap{nullptr};
    int64_t _depthUpdateTimestamp{0};

    float _maxDepthDistance{1.f}; //< Max distance represented by the max value in the depth map
    CameraIntrinsics _intrinsics{};

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif
