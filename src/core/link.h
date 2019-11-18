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

#include "./config.h"
#include "./core/coretypes.h"
#include "./core/serialized_object.h"
#include "./core/spinlock.h"
#include "./core/value.h"

namespace Splash
{

class RootObject;
class BufferObject;

/*************/
class Link
{
  public:
    /**
     * \brief Constructor
     * \param root Root object
     * \param name Name of the link
     */
    Link(RootObject* root, const std::string& name);

    /**
     * \brief Destructor
     */
    ~Link();

    /**
     * \brief Connect to a pair given its name
     * \param name Peer name
     */
    void connectTo(const std::string& name);

    /**
     * \brief Connect to a pair given its name and a shared_ptr, useful when the peer is an object of _root
     * \param name Peer name
     * \param peer Pointer to an inner peer
     */
    void connectTo(const std::string& name, RootObject* peer);

    /**
     * \brief Disconnect from a pair given its name
     * \param name Peer name
     */
    void disconnectFrom(const std::string& name);

    /**
     * \brief Send a buffer to the connected peers
     * \param name Buffer name
     * \param buffer Serialized buffer
     */
    bool sendBuffer(const std::string& name, std::shared_ptr<SerializedObject> buffer);

    /**
     * \brief Send a buffer to the connected peers
     * \param name Buffer name
     * \param object Object to get a serialized version from
     */
    bool sendBuffer(const std::string& name, const std::shared_ptr<BufferObject>& object);

    /**
     * \brief Send a message to connected peers
     * \param name Destination object name
     * \param attribute Attribute
     * \param message Message
     * \return Return true if all went well
     */
    bool sendMessage(const std::string& name, const std::string& attribute, const Values& message);

    /**
     * \brief Send a message to connected peers. Converts known base types to vector<Value> before sending.
     * \param name Destination object name
     * \param attribute Attribute
     * \param message Message
     * \return Return true if all went well
     */
    template <typename T>
    bool sendMessage(const std::string& name, const std::string& attribute, const std::vector<T>& message);

    /**
     * \brief Check that all buffers were sent to the client
     * \param maximumWait Maximum waiting time
     * \return Return true if all went well
     */
    bool waitForBufferSending(std::chrono::milliseconds maximumWait);

  private:
    RootObject* _rootObject;
    std::string _basePath{""};
    std::string _name{""};

    std::unique_ptr<zmq::context_t> _context;
    std::unique_ptr<zmq::socket_t> _socketBufferIn;
    std::unique_ptr<zmq::socket_t> _socketBufferOut;
    std::unique_ptr<zmq::socket_t> _socketMessageIn;
    std::unique_ptr<zmq::socket_t> _socketMessageOut;

    std::vector<std::string> _connectedTargets;
    std::map<std::string, RootObject*> _connectedTargetPointers;

    bool _connectedToInner{false};
    bool _connectedToOuter{false};
    bool _running{false};

    Spinlock _msgSendMutex;
    Spinlock _bufferSendMutex;

    std::deque<std::shared_ptr<SerializedObject>> _otgBuffers;
    Spinlock _otgMutex;
    std::atomic_int _otgNumber{0};

    std::thread _bufferInThread;
    std::thread _messageInThread;

    /**
     * \brief Callback to remove the shared_ptr to a sent buffer
     * \param data Pointer to sent data
     * \param hint Pointer to the Link
     */
    static void freeOlderBuffer(void* data, void* hint);

    /**
     * \brief Message input thread function
     */
    void handleInputMessages();

    /**
     * \brief Buffer input thread function
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
