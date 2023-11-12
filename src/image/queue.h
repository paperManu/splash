/*
 * Copyright (C) 2015 Splash authors
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
 * @queue.h
 * The Queue class, and its sibling QueueSurrogate
 */

#ifndef SPLASH_QUEUE_H
#define SPLASH_QUEUE_H

#include <glm/glm.hpp>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "./core/constants.h"

#include "./core/attribute.h"
#include "./core/factory.h"
#include "./graphics/filter.h"
#include "./graphics/texture.h"
#include "./image/image.h"

namespace Splash
{

/*************/
class Queue : public BufferObject
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    explicit Queue(RootObject* root);

    /**
     * Destructor
     */
    ~Queue() override;

    /**
     * Constructors/operators
     */
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(Queue&&) = delete;

    /**
     * The Queue does not exist on the Scene side, there is the QueueSurrogate for this. So deserialization has no meaning
     * \return Return always false
     */
    bool deserialize(SerializedObject&& /*obj*/) override { return false; }

    /**
     * Return the name of the distant buffer object
     * \return Return the distant name
     */
    std::string getDistantName() const override;

    /**
     * Serialize the underlying source
     * \return Return the serialized object
     */
    SerializedObject serialize() const override;

    /**
     * Returns always true, the Queue object handles update itself
     * \return Return true if the queue was updated
     */
    bool wasUpdated() const override { return true; }

    /**
     * Update the current texture
     */
    void update() override;

  private:
    std::unique_ptr<Factory> _factory;

    struct Source
    {
        std::string type{"image"};
        std::string filename{""};
        int64_t start{0};    // in useconds
        int64_t stop{0};     // in useconds
        bool freeRun{false}; // if true, never use master clock
        Values args{};
    };
    std::vector<Source> _playlist{}; // Holds a vector of [type, filename, start, stop, args], which is also a Values
    std::mutex _playlistMutex;

    bool _useClock{false};
    bool _paused{false};

    std::shared_ptr<BufferObject> _currentSource; // The source being played

    int32_t _currentSourceIndex{-1};
    bool _playing{false};

    bool _loop{false};
    bool _seeked{false};
    float _seekTime{0};
    int64_t _startTime{-1};   // Beginning of the current loop, in us
    int64_t _currentTime{-1}; // Elapsed time since _startTime

    /**
     * Clean the playlist for holes and overlaps
     * \param playlist Playlist to clean
     */
    void cleanPlaylist(std::vector<Source>& playlist);

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();

    /**
     * Run the tasks waiting in the object's queue.
     * Also runs tasks for objects created by this queue,
     * namely _currentSource, if it is valid.
     */
    void runTasks() override;
};

/*************/
class QueueSurrogate final : public Texture
{
  public:
    /**
     * Constructor
     * \param root Root object
     */
    explicit QueueSurrogate(RootObject* root);

    /**
     * Destructor
     */
    ~QueueSurrogate() final = default;

    /**
     * Constructors/operators
     */
    QueueSurrogate(const QueueSurrogate&) = delete;
    QueueSurrogate& operator=(const QueueSurrogate&) = delete;
    QueueSurrogate(QueueSurrogate&&) = delete;
    QueueSurrogate& operator=(QueueSurrogate&&) = delete;

    /**
     * Bind this texture of this filter
     */
    void bind() final;

    /**
     * Unbind this texture of this filter
     */
    void unbind() final;

    /**
     * Get the shader parameters related to this texture. Texture should be locked first.
     * \return Return the shader uniforms
     */
    std::unordered_map<std::string, Values> getShaderUniforms() const;

    /**
     * Get the filter created by the queue
     * \return Return the filter
     */
    std::shared_ptr<Filter> getFilter() const { return _filter; }

    /**
     * Get the id of the gl texture
     * \return Return the texture id
     */
    GLuint getTexId() const { return _filter->getTexId(); }

    /**
     * Get the current source created by the queue
     * \return Return the current source
     */
    std::shared_ptr<GraphObject> getSource() const { return _source; }

    /**
     * Get spec of the texture
     * \return Return the spec
     */
    ImageBufferSpec getSpec() const;

    /**
     * Update the texture according to the owned Image
     */
    void update() final{};

  private:
    int _filterIndex{0};
    std::shared_ptr<Filter> _filter;
    std::shared_ptr<GraphObject> _source;

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif // SPLASH_QUEUE_H
