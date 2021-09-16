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
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <zmq.hpp>

#include "./core/constants.h"

#include "./core/serialized_object.h"
#include "./core/spinlock.h"
#include "./core/value.h"
#include "./network/channel.h"

namespace Splash
{

class RootObject;
class BufferObject;

/*************/
class Link
{
  public:
    /**
     * Constructor
     * \param root Root object
     * \param name Name of the link
     */
    Link(RootObject* root, const std::string& name);

    /**
     * Destructor
     */
    ~Link();

    /**
     * Connect to a pair given its name
     * \param name Peer name
     */
    void connectTo(const std::string& name);

    /**
     * Disconnect from a pair given its name
     * \param name Peer name
     */
    void disconnectFrom(const std::string& name);

    /**
     * Send a buffer to the connected peers
     *
     * The name of the target has to be serialized at the beginning of the buffer,
     * in order to be used to point it to the correct target
     *
     * \param buffer Serialized buffer
     */
    bool sendBuffer(SerializedObject&& buffer);

    /**
     * Send a buffer to the connected peers
     * \param name Buffer name
     * \param object Object to get a serialized version from
     */
    bool sendBuffer(const std::shared_ptr<BufferObject>& object);

    /**
     * Send a message to connected peers
     * \param name Destination object name
     * \param attribute Attribute
     * \param message Message
     * \return Return true if all went well
     */
    bool sendMessage(const std::string& name, const std::string& attribute, const Values& message);

    /**
     * Send a message to connected peers. Converts known base types to vector<Value> before sending.
     * \param name Destination object name
     * \param attribute Attribute
     * \param message Message
     * \return Return true if all went well
     */
    template <typename T>
    bool sendMessage(const std::string& name, const std::string& attribute, const std::vector<T>& message);

    /**
     * Check that all buffers were sent to the client
     * \param maximumWait Maximum waiting time
     * \return Return true if all went well
     */
    bool waitForBufferSending(std::chrono::milliseconds maximumWait);

  private:
    RootObject* _rootObject;
    std::string _name{""};

    std::unique_ptr<ChannelOutput> _channelOutput;
    std::unique_ptr<ChannelInput> _channelInput;

    /**
     * Message input thread function
     * \param name Object name to send message to
     * \param attribute Object attribute for the message
     * \param value Object value to update
     */
    void handleInputMessages(const std::vector<uint8_t>& message);

    /**
     * Buffer input thread function
     * \param buffer Buffer to be sent
     */
    void handleInputBuffers(SerializedObject&& buffer);
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
