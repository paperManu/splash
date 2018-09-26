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
 * @texture.h
 * The Geometry base class
 */

#ifndef SPLASH_GEOMETRY_H
#define SPLASH_GEOMETRY_H

#include <chrono>
#include <glm/glm.hpp>
#include <map>
#include <utility>
#include <vector>

#include "config.h"

#include "./core/attribute.h"
#include "./core/buffer_object.h"
#include "./core/coretypes.h"
#include "./graphics/gpu_buffer.h"
#include "./mesh/mesh.h"

namespace Splash
{

class Geometry : public BufferObject
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Geometry(RootObject* root);

    /**
     * \brief Destructor
     */
    ~Geometry() override;

    /**
     * No copy constructor, but a move one
     */
    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;

    /**
     * \brief Activate the geometry for rendering
     */
    void activate();

    /**
     * \brief Activate the geometry for gpgpu
     */
    void activateAsSharedBuffer();

    /**
     * \brief Activate the geomtry for feedback into the alternative buffers
     */
    void activateForFeedback();

    /**
     * \brief Deactivate the geometry for rendering
     */
    void deactivate() const;

    /**
     * \brief Deactivate for feedback
     */
    void deactivateFeedback();

    /**
     * \brief Get the number of vertices for this geometry
     * \return Return the vertice count
     */
    int getVerticesNumber() const { return _useAlternativeBuffers ? _alternativeVerticesNumber : _verticesNumber; }

    /**
     * \brief Get the geometry as serialized
     * \return Return the serialized geometry
     */
    std::shared_ptr<SerializedObject> serialize() const override;

    /**
     * \brief Deserialize the geometry
     * \param obj Serialized object
     * \return Return true if all went well
     */
    bool deserialize(const std::shared_ptr<SerializedObject>& obj) override;

    /**
     * \brief Get whether the alternative buffers have been resized during the last feedback call
     * \return Return true if the buffers have been resized
     */
    bool hasBeenResized() { return _buffersResized; }

    /**
     * \brief Try to link the given GraphObject to this object
     * \param obj Shared pointer to the (wannabe) child object
     */
    bool linkTo(const std::shared_ptr<GraphObject>& obj) override;

    /**
     * \brief Get the coordinates of the closest vertex to the given point
     * \param p Point around which to look
     * \param v If detected, vertex coordinates
     * \return Return the distance from p to v
     */
    float pickVertex(glm::dvec3 p, glm::dvec3& v);

    /**
     * \brief Set the mesh for this object
     * \param mesh Mesh
     */
    void setMesh(const std::shared_ptr<Mesh>& mesh) { _mesh = std::weak_ptr<Mesh>(mesh); }

    /**
     * \brief Swap between temporary and alternative buffers
     */
    void swapBuffers();

    /**
     * \brief Updates the object
     */
    void update() override;

    /**
     * \brief Activate alternative buffers for draw
     * \param isActive If true, use alternative buffers
     */
    void useAlternativeBuffers(bool isActive);

  private:
    mutable std::mutex _mutex;
    bool _onMasterScene{false};

    std::shared_ptr<Mesh> _defaultMesh;
    std::weak_ptr<Mesh> _mesh;

    std::map<GLFWwindow*, GLuint> _vertexArray;
    std::vector<std::shared_ptr<GpuBuffer>> _glBuffers{};
    std::vector<std::shared_ptr<GpuBuffer>> _glAlternativeBuffers{}; // Alternative buffers used for rendering
    std::vector<std::shared_ptr<GpuBuffer>> _glTemporaryBuffers{};   // Temporary buffers used for feedback
    bool _buffersDirty{false};
    bool _buffersResized{false}; // Holds whether the alternative buffers have been resized in the previous feedback
    bool _useAlternativeBuffers{false};

    SerializedObject _serializedMesh{};

    int _verticesNumber{0};
    int _alternativeVerticesNumber{0};
    int _alternativeBufferSize{0};
    int _temporaryVerticesNumber{0};
    int _temporaryBufferSize{0};

    // Transform feedback
    GLuint _feedbackQuery;
    int _feedbackMaxNbrPrimitives{0};

    /**
     * \brief Initialization
     */
    void init();

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_GEOMETRY_H
