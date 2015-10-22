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

#include <list>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "config.h"

#include "coretypes.h"
#include "basetypes.h"
#include "filter.h"
#include "image.h"
#include "texture.h"

namespace Splash {

class World;

/*************/
class Queue : public BufferObject
{
    public:
        /**
         * Constructor
         */
        Queue(RootObjectWeakPtr root);

        /**
         * Destructor
         */
        ~Queue();

        /**
         * No copy constructor, but a move one
         */
        Queue(const Queue&) = delete;
        Queue(Queue&&) = default;
        Queue& operator=(const Queue&) = delete;

        /**
         * The Queue does not exist on the Scene side, there is the QueueSurrogate for this
         * So deserialization has no meaning
         */
        bool deserialize(std::unique_ptr<SerializedObject> obj) {return false;}
        
        /**
         * Return the name of the distant buffer object
         */
        std::string getDistantName() const;

        /**
         * Serialize the underlying source
         */
        std::unique_ptr<SerializedObject> serialize() const;

        /**
         * Returns always true, the Queue object handles update itself
         */
        bool wasUpdated() const {return true;}

        /**
         * Update the current texture
         */
        void update();

    private:
        std::weak_ptr<World> _world;

        struct Source
        {
            std::string type;
            std::string filename;
            int64_t start; // in useconds
            int64_t stop; // in useconds
            Values args;
        };
        std::vector<Source> _playlist {}; // Holds a vector of [type, filename, start, stop, args], which is also a Values

        bool _useClock {false};

        std::shared_ptr<BufferObject> _currentSource; // The source being played
        bool _defaultSource {false};

        int32_t _currentSourceIndex {-1};
        bool _playing {false};

        bool _loop {false};
        int64_t _startTime {-1}; // Beginning of the current loop, in us
        int64_t _currentTime {-1}; // Elapsed time since _startTime

        /**
         * Create a source object from the given type
         */
        std::shared_ptr<BufferObject> createSource(std::string type);

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

/*************/
class QueueSurrogate : public Texture
{
    public:
        /**
         * Constructor
         */
        QueueSurrogate(RootObjectWeakPtr root);

        /**
         * Destructor
         */
        ~QueueSurrogate();

        /**
         * No copy constructor, but a move one
         */
        QueueSurrogate(const QueueSurrogate&) = delete;
        QueueSurrogate(QueueSurrogate&&) = default;
        QueueSurrogate& operator=(const QueueSurrogate&) = delete;

        /**
         * Bind / unbind this texture of this filter
         */
        void bind();
        void unbind();

        /**
         * Get the shader parameters related to this texture
         * Texture should be locked first
         */
        std::unordered_map<std::string, Values> getShaderUniforms() const;

        /**
         * Get spec of the texture
         */
        oiio::ImageSpec getSpec() const;

        /**
         * Update the texture according to the owned Image
         */
        void update();

    private:
        std::shared_ptr<Filter> _filter;
        std::shared_ptr<BaseObject> _source;

        std::list<std::function<void()>> _taskQueue;
        std::mutex _taskMutex;

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

} // end of namespace

#endif // SPLASH_QUEUE_H
