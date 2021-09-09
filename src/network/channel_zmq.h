/*
 * Copyright (C) 2021 Emmanuel Durand
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
 * @channel_zmq.h
 * ZMQ implementation of ChannelOutput and ChannelInput
 */

#ifndef SPLASH_CHANNEL_SHMDATA_H
#define SPLASH_CHANNEL_SHMDATA_H

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <zmq.hpp>

#include "./network/channel.h"
#include "./core/spinlock.h"

namespace Splash
{

/*************/
class ChannelOutput_ZMQ : public ChannelOutput
{
  public:
    /**
     * Constructor
     * \param root Root object
     * \param name Channel name, which should be the same as the parent Link
     */
    ChannelOutput_ZMQ(const RootObject* root, const std::string& name);

    /**
     * Destructor
     */
    ~ChannelOutput_ZMQ();

    /**
     * Connect to a target
     * \param target Target name
     * \return Return true if connection was successful
     */
    bool connectTo(const std::string& target) final;

    /**
     * Disconnect from a target
     * \param target Target name
     * \return Return true if disconnection was successful, false otherwise or if no connection existed
     */
    bool disconnectFrom(const std::string& target) final;

    /**
     * Send a message
     * \param message Message to be sent
     * \return Return true if the message was successfully sent
     */
    bool sendMessage(const std::vector<uint8_t>& message) final;

    /**
     * Send a buffer to the given object
     * \param name Buffer name to send
     * \param buffer Buffer to be sent
     * \return Return true if the buffer was successfully sent
     */
    bool sendBufferTo(const std::string& name, SerializedObject&& buffer) final;

    /**
     * Check that all buffers were sent to the client
     * \param maximumWait Maximum waiting time
     * \return Return true if all went well
     */
    bool waitForBufferSending(std::chrono::milliseconds maximumWait) final;

  private:
    Spinlock _msgSendMutex;
    Spinlock _bufferSendMutex;

    Spinlock _sendQueueMutex;
    std::deque<SerializedObject> _bufferSendQueue;
    uint32_t _sendQueueBufferCount{0};
    std::condition_variable _bufferTransmittedCondition{};
    std::mutex _bufferTransmittedMutex{};
    
    std::string _pathPrefix;

    zmq::context_t _context{1};
    std::unique_ptr<zmq::socket_t> _socketMessageOut;
    std::unique_ptr<zmq::socket_t> _socketBufferOut;

    std::vector<std::string> _targets;

    static void freeSerializedBuffer(void* data, void* hint);
};

/*************/
class ChannelInput_ZMQ : public ChannelInput
{
  public:
    /**
     * Constructor
     * \param root Root object
     * \param name Channel name, which should be the same as the parent Link
     * \param msgRecvCb Callback to call when receiving a message
     * \param bufferRecvCb Callback to call when receiving a buffer
     */
    ChannelInput_ZMQ(const RootObject* root, const std::string& name, MessageRecvCallback msgRecvCb, BufferRecvCallback bufferRecvCb);

    /**
     * Destructor
     */
    ~ChannelInput_ZMQ();

    /**
     * Connect to the given target
     * \param target Target name
     * \return Return true if connection was successful
     */
    bool connectTo(const std::string&) final { return false; }

    /**
     * Disconnect from the given target
     * \param target name
     * \return Return true if disconnection was successful, false otherwise or if no connection existed
     */
    bool disconnectFrom(const std::string&) final { return false; }

  private:
    bool _continueListening;
    std::string _pathPrefix;

    zmq::context_t _context{1};
    std::unique_ptr<zmq::socket_t> _socketMessageIn;
    std::unique_ptr<zmq::socket_t> _socketBufferIn;

    std::thread _messageInThread;
    std::thread _bufferInThread;

    void handleInputMessages();
    void handleInputBuffers();
};

} // namespace Splash

#endif // SPLASH_CHANNEL_SHMDATA_H

