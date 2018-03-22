/*
 * Copyright (C) 2015 Emmanuel Durand
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

#include "./config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./core/factory.h"
#include "./graphics/filter.h"
#include "./graphics/texture.h"
#include "./image/image.h"

namespace Splash
{

class World;

/*************/
class Queue : public BufferObject
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    Queue(RootObject* root);

    /**
     * \brief Destructor
     */
    ~Queue() override;

    /**
     * No copy constructor, but a move one
     */
    Queue(const Queue&) = delete;
    Queue(Queue&&) = default;
    Queue& operator=(const Queue&) = delete;

    /**
     * \brief The Queue does not exist on the Scene side, there is the QueueSurrogate for this. So deserialization has no meaning
     * \return Return always false
     */
    bool deserialize(const std::shared_ptr<SerializedObject>& /*obj*/) override { return false; }

    /**
     * \brief Return the name of the distant buffer object
     * \return Return the distant name
     */
    std::string getDistantName() const;

    /**
     * \brief Serialize the underlying source
     * \return Return the serialized object
     */
    std::shared_ptr<SerializedObject> serialize() const override;

    /**
     * \brief Returns always true, the Queue object handles update itself
     * \return Return true if the queue was updated
     */
    bool wasUpdated() const { return true; }

    /**
     * \brief Update the current texture
     */
    void update();

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
    bool _defaultSource{false};

    int32_t _currentSourceIndex{-1};
    bool _playing{false};

    bool _loop{false};
    bool _seeked{false};
    int64_t _startTime{-1};   // Beginning of the current loop, in us
    int64_t _currentTime{-1}; // Elapsed time since _startTime

    /**
     * \brief Clean the playlist for holes and overlaps
     * \param playlist Playlist to clean
     */
    void cleanPlaylist(std::vector<Source>& playlist);

    /**
     * Regist\brief er new functors to modify attributes
     */
    void registerAttributes();
};

/*************/
class QueueSurrogate : public Texture
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    QueueSurrogate(RootObject* root);

    /**
     * No copy constructor, but a move one
     */
    QueueSurrogate(const QueueSurrogate&) = delete;
    QueueSurrogate(QueueSurrogate&&) = default;
    QueueSurrogate& operator=(const QueueSurrogate&) = delete;

    /**
     * \brief Bind this texture of this filter
     */
    void bind() final;

    /**
     * \brief Unbind this texture of this filter
     */
    void unbind() final;

    /**
     * \brief Get the shader parameters related to this texture. Texture should be locked first.
     * \return Return the shader uniforms
     */
    std::unordered_map<std::string, Values> getShaderUniforms() const;

    /**
     * \brief Get the filter created by the queue
     * \return Return the filter
     */
    std::shared_ptr<Filter> getFilter() const { return _filter; }

    /**
     * \brief Get the id of the gl texture
     * \return Return the texture id
     */
    GLuint getTexId() const { return _filter->getTexId(); }

    /**
     * \brief Get the current source created by the queue
     * \return Return the current source
     */
    std::shared_ptr<BaseObject> getSource() const { return _source; }

    /**
     * \brief Get spec of the texture
     * \return Return the spec
     */
    ImageBufferSpec getSpec() const;

    /**
     * \brief Update the texture according to the owned Image
     */
    void update() final{};

  private:
    int _filterIndex{0};
    std::shared_ptr<Filter> _filter;
    std::shared_ptr<BaseObject> _source;

    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_QUEUE_H
