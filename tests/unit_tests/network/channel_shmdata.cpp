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

#include "./network/channel_shmdata.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <doctest.h>

#include "./core/root_object.h"
#include "./core/serialized_object.h"

using namespace Splash;

/*************/
TEST_CASE("Test sending a message and a buffer through a shmdata channel")
{
    auto root = RootObject();

    bool isMsgReceived = false;
    bool isBufferReceived = false;

    std::vector<uint8_t> receivedMsg;
    SerializedObject receivedObj;

    auto channelOutput = ChannelOutput_Shmdata(&root, "output");
    auto channelInput = ChannelInput_Shmdata(
        &root,
        "input",
        [&](const std::vector<uint8_t> msg) {
            isMsgReceived = true;
            receivedMsg = msg;
        },
        [&](SerializedObject&& obj) {
            isBufferReceived = true;
            receivedObj = std::move(obj);
        });

    CHECK(channelOutput.connectTo("input"));
    CHECK(channelInput.connectTo("output"));

    std::vector<uint8_t> msg = {1, 2, 3, 4};
    CHECK(channelOutput.sendMessage(msg));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(isMsgReceived);
    CHECK_EQ(msg, receivedMsg);

    auto array = ResizableArray<uint8_t>({1, 2, 3});
    auto object = SerializedObject(std::move(array));
    CHECK(channelOutput.sendBuffer(std::move(object)));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(isBufferReceived);
    CHECK_EQ(receivedObj.data()[0], 1);
    CHECK_EQ(receivedObj.data()[1], 2);
    CHECK_EQ(receivedObj.data()[2], 3);
}
