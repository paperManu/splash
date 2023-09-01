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

#ifndef SPLASH_CHANNEL_ZMQ_H
#define SPLASH_CHANNEL_ZMQ_H

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <zmq.hpp>

#include "./network/channel.h"
#include "./core/spinlock.h"

namespace Splash
{

/*************/
class ChannelOutput_ZMQ final : public ChannelOutput
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
    ~ChannelOutput_ZMQ() final;
    /**
     * Constructors/operators
     */
    ChannelOutput_ZMQ(const ChannelOutput_ZMQ&) = delete;
    ChannelOutput_ZMQ& operator=(const ChannelOutput_ZMQ&) = delete;
    ChannelOutput_ZMQ(ChannelOutput_ZMQ&&) = delete;
    ChannelOutput_ZMQ& operator=(ChannelOutput_ZMQ&&) = delete;

    /**
     * Connect to a target
     * \param target Target name
     * \return Return true if connection was successful
     */
    [[nodiscard]] bool connectTo(const std::string& target) final;

    /**
     * Disconnect from a target
     * \param target Target name
     * \return Return true if disconnection was successful, false otherwise or if no connection existed
     */
    [[nodiscard]] bool disconnectFrom(const std::string& target) final;

    /**
     * Get the connect direction for this channel type
     * ZMQ channels need the output to connect to the input
     * \return Return ConnectDirection::OutToIn
     */
    ConnectDirection getConnectDirection() const final { return ConnectDirection::OutToIn; }

    /**
     * Send a message
     * \param message Message to be sent
     * \return Return true if the message was successfully sent
     */
    bool sendMessage(const std::vector<uint8_t>& message) final;

    /**
     * Send a buffer
     * \param buffer Buffer to be sent
     * \return Return true if the buffer was successfully sent
     */
    bool sendBuffer(SerializedObject&& buffer) final;

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
    std::unique_ptr<zmq::socket_t> _socketMessageOut{nullptr};
    std::unique_ptr<zmq::socket_t> _socketBufferOut{nullptr};

    std::set<std::string> _targets;

    static void freeSerializedBuffer(void* data, void* hint);
};

/*************/
class ChannelInput_ZMQ final : public ChannelInput
{
  public:
    /**
     * Constructor
     * \param root Root object
     * \param name Channel name, which should be the same as the parent Link
     * \param msgRecvCb Callback to call when receiving a message
     * \param bufferRecvCb Callback to call when receiving a buffer
     */
    ChannelInput_ZMQ(const RootObject* root, const std::string& name, const MessageRecvCallback& msgRecvCb, const BufferRecvCallback& bufferRecvCb);

    /**
     * Destructor
     */
    ~ChannelInput_ZMQ() final;

    /**
     * Constructors/operators
     */
    ChannelInput_ZMQ(const ChannelInput_ZMQ&) = delete;
    ChannelInput_ZMQ& operator=(const ChannelInput_ZMQ&) = delete;
    ChannelInput_ZMQ(ChannelInput_ZMQ&&) = delete;
    ChannelInput_ZMQ& operator=(ChannelInput_ZMQ&&) = delete;

    /**
     * Connect to the given target
     * \param target Target name
     * \return Return true if connection was successful
     */
    [[nodiscard]] bool connectTo(const std::string&) final { return true; }

    /**
     * Disconnect from the given target
     * \param target name
     * \return Return true if disconnection was successful, false otherwise or if no connection existed
     */
    [[nodiscard]] bool disconnectFrom(const std::string&) final { return true; }

  private:
    bool _continueListening;
    std::string _pathPrefix;

    zmq::context_t _context{1};
    std::unique_ptr<zmq::socket_t> _socketMessageIn{nullptr};
    std::unique_ptr<zmq::socket_t> _socketBufferIn{nullptr};

    std::thread _messageInThread;
    std::thread _bufferInThread;

    void handleInputMessages();
    void handleInputBuffers();
};

} // namespace Splash

#endif // SPLASH_CHANNEL_SHMDATA_H

