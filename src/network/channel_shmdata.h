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
 * @channel_shmdata.h
 * shmdata implementation of ChannelOutput and ChannelInput
 */

#ifndef SPLASH_CHANNEL_SHMDATA_H
#define SPLASH_CHANNEL_SHMDATA_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

#include <shmdata/follower.hpp>
#include <shmdata/writer.hpp>

#include "./network/channel.h"
#include "./utils/osutils.h"

namespace Splash
{

/*************/
class ChannelOutput_Shmdata final : public ChannelOutput
{
  public:
    /**
     * Constructor
     * \param root Root object, used as context for this class
     * \param name Channel name, which should be the same as the parent Link
     */
    ChannelOutput_Shmdata(const RootObject* root, const std::string& name);

    /**
     * Destructor
     */
    ~ChannelOutput_Shmdata() final;

    /**
     * Other constructors
     */
    ChannelOutput_Shmdata(ChannelOutput_Shmdata&) = delete;
    ChannelOutput_Shmdata(ChannelOutput_Shmdata&&) = delete;
    ChannelOutput_Shmdata& operator=(ChannelOutput_Shmdata&) = delete;
    ChannelOutput_Shmdata& operator=(ChannelOutput_Shmdata&&) = delete;

    /**
     * Connect to a target
     *
     * With shmdata, it is the reader which connects to the writer, hence this
     * method always returns true. There is a special case when the target is
     * the World object, in which case the method will wait for the World
     * shmdata::Follower to connect to the message writer.
     *
     * \param target Target name
     * \return Return true if connection was successful
     */
    bool connectTo(const std::string& target) final;

    /**
     * Disconnect from a target
     *
     * With shmdata, it is the reader which connects to the writer, hence this
     * method always returns true.
     *
     * \param target Target name
     * \return Return true if disconnection was successful, false otherwise or if no connection existed
     */
    bool disconnectFrom(const std::string& /*target*/) final { return true; }

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
     *
     * Copy to shmdata is (for now) synchronous so there is no need to wait.
     *
     * \param maximumWait Maximum waiting time
     * \return Return true if all went well
     */
    bool waitForBufferSending(std::chrono::milliseconds /*maximumWait*/) final { return true; }

  private:
    static const size_t _shmDefaultSize;
    static const std::string _defaultPathPrefix;
    bool _joinAllThreads{false};

    bool _isWorldConnected{false};
    std::mutex _worldConnectedMutex;
    std::condition_variable _worldConnectedCondition;

    std::vector<std::vector<uint8_t>> _msgQueue;
    std::mutex _msgConsumeMutex;
    std::condition_variable _msgCondition;
    bool _msgNewInQueue{false};
    std::thread _msgConsumeThread;

    std::vector<SerializedObject> _bufQueue;
    std::mutex _bufConsumeMutex;
    std::condition_variable _bufCondition;
    bool _bufNewInQueue{false};
    std::thread _bufConsumeThread;

    Utils::ShmdataLogger _shmlogger{};
    std::string _pathPrefix{};

    std::unique_ptr<shmdata::Writer> _msgWriter{nullptr};
    std::unique_ptr<shmdata::Writer> _bufWriter{nullptr};

    /**
     * Message consume method, responsible for sending the queued messages
     * Used by the message consume thread
     */
    void messageConsume();

    /**
     * Buffer consume method, responsible for sending the queued buffers
     * Used by the buffer consume thread
     */
    void bufferConsume();
};

/*************/
class ChannelInput_Shmdata final : public ChannelInput
{
  public:
    /**
     * Constructor
     * \param root Root object, used as context for this class
     * \param name Channel name, which should be the same as the parent Link
     * \param msgRecvCb Callback to call when receiving a message
     * \param bufferRecvCb Callback to call when receiving a buffer
     */
    ChannelInput_Shmdata(const RootObject* root, const std::string& name, const MessageRecvCallback& msgRecvCb, const BufferRecvCallback& bufferRecvCb);

    /**
     * Destructor
     */
    ~ChannelInput_Shmdata() final;

    /**
     * Other constructors
     */
    ChannelInput_Shmdata(ChannelInput_Shmdata&) = delete;
    ChannelInput_Shmdata(ChannelInput_Shmdata&&) = delete;
    ChannelInput_Shmdata& operator=(ChannelInput_Shmdata&) = delete;
    ChannelInput_Shmdata& operator=(ChannelInput_Shmdata&&) = delete;

    /**
     * Connect to the given target
     *
     * \param target Target name
     * \return Return true if connection was successful
     */
    bool connectTo(const std::string& target) final;

    /**
     * Disconnect from the given target
     *
     * \param target name
     * \return Return true if disconnection was successful, false otherwise or if no connection existed
     */
    bool disconnectFrom(const std::string& target) final;

  private:
    static const std::string _defaultPathPrefix;

    Utils::ShmdataLogger _shmlogger{};
    std::string _pathPrefix{};
    std::string _msgCaps{};
    std::string _bufCaps{};

    bool _joinAllThreads{false};

    std::vector<std::vector<uint8_t>> _msgQueue;
    std::mutex _msgConsumeMutex;
    std::condition_variable _msgCondition;
    bool _msgNewInQueue{false};
    std::thread _msgConsumeThread;

    std::vector<SerializedObject> _bufQueue;
    std::mutex _bufConsumeMutex;
    std::condition_variable _bufCondition;
    bool _bufNewInQueue{false};
    std::thread _bufConsumeThread;

    std::unordered_map<std::string, std::unique_ptr<shmdata::Follower>> _msgFollowers{};
    std::unordered_map<std::string, std::unique_ptr<shmdata::Follower>> _bufFollowers{};

    /**
     * Message consume method, responsible for handling the input queued messages
     * Used by the message consume thread
     */
    void messageConsume();

    /**
     * Buffer consume method, responsible for handling the inputt queued buffers
     * Used by the buffer consume thread
     */
    void bufferConsume();
};

} // namespace Splash

#endif // SPLASH_CHANNEL_SHMDATA_H
