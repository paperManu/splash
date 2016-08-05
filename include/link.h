/*
 * Copyright (C) 2014 Emmanuel Durand
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
 * @link.h
 * The Link class, used for communication between World and Scenes
 */

#ifndef SPLASH_LINK_H
#define SPLASH_LINK_H

#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <zmq.hpp>

#include "config.h"
#include "coretypes.h"

namespace Splash {

class RootObject;
class BufferObject;

/*************/
class Link
{
    public:
        /**
         * Constructor
         */
        Link(std::weak_ptr<RootObject> root, std::string name);

        /**
         * Destructor
         */
        ~Link();

        /**
         * Connect to a pair given its name, or a shared_ptr
         */
        void connectTo(const std::string& name);
        void connectTo(const std::string& name, const std::weak_ptr<RootObject>& peer);

        /**
         * Disconnect from a pair given its name
         */
        void disconnectFrom(const std::string& name);

        /**
         * Send a buffer to the connected pairs
         */
        bool sendBuffer(const std::string& name, std::shared_ptr<SerializedObject> buffer);
        bool sendBuffer(const std::string& name, const std::shared_ptr<BufferObject>& object);

        /**
         * Send a message to connected pairs
         * The second one converts known base types to vector<Value> before sending
         */
        bool sendMessage(const std::string& name, const std::string& attribute, const Values& message);
        template <typename T>
        bool sendMessage(const std::string& name, const std::string& attribute, const std::vector<T>& message);

        /**
         * Check that all buffers were sent to the client
         */
        bool waitForBufferSending(std::chrono::milliseconds maximumWait);

    private:
        std::weak_ptr<RootObject> _rootObject;
        std::string _name;
        std::shared_ptr<zmq::context_t> _context;
        std::mutex _msgSendMutex;
        std::mutex _bufferSendMutex;

        std::vector<std::string> _connectedTargets;
        std::map<std::string, std::weak_ptr<RootObject>> _connectedTargetPointers;

        bool _connectedToInner {false};
        bool _connectedToOuter {false};

        std::shared_ptr<zmq::socket_t> _socketBufferIn;
        std::shared_ptr<zmq::socket_t> _socketBufferOut;
        std::shared_ptr<zmq::socket_t> _socketMessageIn;
        std::shared_ptr<zmq::socket_t> _socketMessageOut;

        std::deque<std::shared_ptr<SerializedObject>> _otgBuffers;
        std::mutex _otgMutex;
        std::atomic_int _otgNumber {0};

        std::thread _bufferInThread;
        std::thread _messageInThread;

        /**
         * Callback to remove the shared_ptr to a sent buffer
         */
        static void freeOlderBuffer(void* data, void* hint);

        /**
         * Message input thread function
         */
        void handleInputMessages();

        /**
         * Buffer input thread function
         */
        void handleInputBuffers();
};

/*************/
template <typename T>
bool Link::sendMessage(const std::string& name, const std::string& attribute, const std::vector<T>& message)
{
    Values convertedMsg;

    for (auto& m : message)
        convertedMsg.emplace_back(m);

    return sendMessage(name, attribute, convertedMsg);
}

} // end of namespace

#endif // SPLASH_LINK_H
