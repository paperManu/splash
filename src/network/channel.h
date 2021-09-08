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
     * Send a message to the given object
     * \param name Object name to send message to
     * \param attribute Object attribute for the message
     * \param value Object value to update
     * \return Return true if the message was successfully sent
     */
    virtual bool sendMessageTo(const std::string& name, const std::string& attribute, const Values& value) = 0;

    /**
     * Send a buffer to the given object
     * \param name Buffer name to send
     * \param buffer Buffer to be sent
     * \return Return true if the buffer was successfully sent
     */
    virtual bool sendBufferTo(const std::string& name, SerializedObject&& buffer) = 0;

    /**
     * Check that all buffers were sent to the client
     * \param maximumWait Maximum waiting time
     * \return Return true if all went well
     */
    virtual bool waitForBufferSending(std::chrono::milliseconds maximumWait) = 0;

  protected:
    const RootObject* _root;
    const std::string _name;
};

/*************/
class ChannelInput
{
  public:
    using MessageRecvCallback = std::function<void(const std::string&, const std::string&, const Values&)>;
    using BufferRecvCallback = std::function<void(const std::string&, SerializedObject&&)>;

  public:
    /**
     * Constructor
     * \param root Root object, used as context for this class
     * \param name Channel name, which should be the same as the parent Link
     * \param msgRecvCb Callback to call when receiving a message
     * \param bufferRecvCb Callback to call when receiving a buffer
     */
    ChannelInput(const RootObject* root, const std::string& name, MessageRecvCallback msgRecvCb, BufferRecvCallback bufferRecvCb)
        : _root(root)
        , _name(name)
        , _msgRecvCb(msgRecvCb)
        , _bufferRecvCb(bufferRecvCb)
    {
    }

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

  protected:
    const RootObject* _root;
    const std::string _name;

    MessageRecvCallback _msgRecvCb;
    BufferRecvCallback _bufferRecvCb;
};

} // namespace Splash

#endif // SPLASH_CHANNEL_H
