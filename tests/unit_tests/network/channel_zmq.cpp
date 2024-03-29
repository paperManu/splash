/*
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

#include "./network/channel_zmq.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <doctest.h>

#include "./core/root_object.h"
#include "./core/serialized_object.h"

using namespace Splash;

/*************/
TEST_CASE("Test sending a message and a buffer through a zmq channel")
{
    auto root = RootObject();

    std::atomic_bool isMsgReceived = false;
    std::atomic_bool isBufferReceived = false;
    std::mutex receivedMutex;

    std::vector<uint8_t> receivedMsg;
    SerializedObject receivedObj;

    auto channelOutput = ChannelOutput_ZMQ(&root, "output");
    auto channelInput = ChannelInput_ZMQ(
        &root,
        "input",
        [&](const std::vector<uint8_t> msg) {
            isMsgReceived = true;
            std::unique_lock<std::mutex> lock(receivedMutex);
            receivedMsg = msg;
        },
        [&](SerializedObject&& obj) {
            isBufferReceived = true;
            std::unique_lock<std::mutex> lock(receivedMutex);
            receivedObj = std::move(obj);
        });

    CHECK(channelInput.connectTo("output"));

    // Connection through ZMQ takes some time to establish, and there is no
    // way to know for sure that it is active without sending a message
    const std::vector<uint8_t> dummyMsg = {42};
    while (!isMsgReceived)
    {
        channelOutput.sendMessage(dummyMsg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    isMsgReceived = false;

    // Messages and buffers are not sent instantly through ZMQ, so
    // we wait for the message to arrive
    const std::vector<uint8_t> msg = {1, 2, 3, 4};
    CHECK(channelOutput.sendMessage(msg));
    while (!isMsgReceived)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    {
        std::unique_lock<std::mutex> lock(receivedMutex);
        CHECK_EQ(msg, receivedMsg);
    }

    auto array = ResizableArray<uint8_t>({1, 2, 3});
    auto object = SerializedObject(std::move(array));
    CHECK(channelOutput.sendBuffer(std::move(object)));
    while (!isBufferReceived)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    {
        std::unique_lock<std::mutex> lock(receivedMutex);
        CHECK_EQ(receivedObj.data()[0], 1);
        CHECK_EQ(receivedObj.data()[1], 2);
        CHECK_EQ(receivedObj.data()[2], 3);
    }
}
