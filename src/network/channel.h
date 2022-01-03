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
 * @channel.h
 * The ChannelInput and ChannelOutput base classes, used by Link to
 * transmit messages/buffers
 */

#ifndef SPLASH_CHANNEL_H
#define SPLASH_CHANNEL_H

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include "./core/constants.h"

#include "./core/serialized_object.h"
#include "./core/value.h"

namespace Splash
{

class RootObject;

/*************/
class ChannelOutput
{
  public:
    /**
     * Constructor
     * \param root Root object, used as context for this class
     * \param name Channel name, which should be the same as the parent Link
     */
    ChannelOutput(const RootObject* root, const std::string& name)
        : _root(root)
        , _name(name)
    {
    }

    /**
     * Destructor
     */
    virtual ~ChannelOutput() = default;

    /**
     * Constructors/operators
     */
    ChannelOutput(const ChannelOutput&) = delete;
    ChannelOutput& operator=(const ChannelOutput&) = delete;
    ChannelOutput(ChannelOutput&&) = delete;
    ChannelOutput& operator=(ChannelOutput&&) = delete;

    /**
     * Connect to a target
     * \param target Target name
     * \return Return true if connection was successful
     */
    virtual bool connectTo(const std::string& target) = 0;

    /**
     * Disconnect from a target
     * \param target Target name
     * \return Return true if disconnection was successful, false otherwise or if no connection existed
     */
    virtual bool disconnectFrom(const std::string& target) = 0;

    /**
     * Check whether the channe is ready
     * \return Return true if ready
     */
    bool isReady() const { return _ready; }

    /**
     * Send a message
     * \param message Message to be sent
     * \return Return true if the message was successfully sent
     */
    virtual bool sendMessage(const std::vector<uint8_t>& message) = 0;

    /**
     * Send a buffer
     * \param buffer Buffer to be sent
     * \return Return true if the buffer was successfully sent
     */
    virtual bool sendBuffer(SerializedObject&& buffer) = 0;

    /**
     * Check that all buffers were sent to the client
     * \param maximumWait Maximum waiting time
     * \return Return true if all went well
     */
    virtual bool waitForBufferSending(std::chrono::milliseconds maximumWait) = 0;

  protected:
    const RootObject* _root;
    const std::string _name;
    bool _ready{false};
};

/*************/
class ChannelInput
{
  public:
    using MessageRecvCallback = std::function<void(const std::vector<uint8_t>&)>;
    using BufferRecvCallback = std::function<void(SerializedObject&&)>;

  public:
    /**
     * Constructor
     * \param root Root object, used as context for this class
     * \param name Channel name, which should be the same as the parent Link
     * \param msgRecvCb Callback to call when receiving a message
     * \param bufferRecvCb Callback to call when receiving a buffer
     */
    ChannelInput(const RootObject* root, const std::string& name, const MessageRecvCallback& msgRecvCb, const BufferRecvCallback& bufferRecvCb)
        : _root(root)
        , _name(name)
        , _msgRecvCb(msgRecvCb)
        , _bufferRecvCb(bufferRecvCb)
    {
    }

    /**
     * Destructor
     */
    virtual ~ChannelInput() = default;

    /**
     * Constructors/operators
     */
    ChannelInput(const ChannelInput&) = delete;
    ChannelInput& operator=(const ChannelInput&) = delete;
    ChannelInput(ChannelInput&&) = delete;
    ChannelInput& operator=(ChannelInput&&) = delete;

    /**
     * Connect to the given target
     * \param target Target name
     * \return Return true if connection was successful
     */
    virtual bool connectTo(const std::string& target) = 0;

    /**
     * Disconnect from the given target
     * \param target name
     * \return Return true if disconnection was successful, false otherwise or if no connection existed
     */
    virtual bool disconnectFrom(const std::string& target) = 0;

    /**
     * Check whether the channe is ready
     * \return Return true if ready
     */
    bool isReady() const { return _ready; }

  protected:
    const RootObject* _root;
    const std::string _name;
    bool _ready{false};

    MessageRecvCallback _msgRecvCb;
    BufferRecvCallback _bufferRecvCb;
};

} // namespace Splash

#endif // SPLASH_CHANNEL_H
