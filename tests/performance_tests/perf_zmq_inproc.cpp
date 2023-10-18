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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include <zmq.hpp>

zmq::context_t zmqContext(2);
zmq::socket_t zmqBufferOut(zmqContext, ZMQ_PUB);
zmq::socket_t zmqBufferIn(zmqContext, ZMQ_SUB);

const int outHighWaterMark = 0;
const int inHighWaterMark = 0;

const std::string socketPath = "ipc:///tmp/perf_zmq_socket";
const size_t bufferSize = 1 << 28;
const size_t loopCount = 1 << 6;

std::vector<uint8_t> srcData((size_t)bufferSize);
std::vector<uint8_t> sinkData((size_t)bufferSize);

std::atomic_int bufferCount{0};
std::mutex copyMutex{};
std::condition_variable copyCondition{};

void nop(void*, void*) {}

int main()
{
    zmqBufferOut.set(zmq::sockopt::sndhwm, outHighWaterMark);
    zmqBufferIn.set(zmq::sockopt::sndhwm, inHighWaterMark);

    std::cout << "Connecting socket output to " << socketPath << "\n";
    zmqBufferOut.connect(socketPath);

    bool continueThread = true;
    bool threadRunning = false;
    std::thread bufferInThread([&]() {
        std::cout << "Launching buffer input thread...\n";
        zmqBufferIn.bind(socketPath);
        zmqBufferIn.set(zmq::sockopt::subscribe, "");

        threadRunning = true;
        while (continueThread)
        {
            zmq::message_t message;

            if (!zmqBufferIn.recv(message, zmq::recv_flags::dontwait))
                continue;

            std::unique_lock<std::mutex> lock(copyMutex);
            std::copy(static_cast<uint8_t*>(message.data()), static_cast<uint8_t*>(message.data()) + message.size(), sinkData.data());
            std::cout << bufferCount << " received\n";

            bufferCount++;
            copyCondition.notify_all();
        }
    });

    for (size_t i = 0; i < bufferSize; ++i)
        srcData[i] = static_cast<uint8_t>(i % 256);

    while (!threadRunning)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::this_thread::sleep_for(std::chrono::seconds(1));

    const auto start = std::chrono::steady_clock::now();

    for (size_t loopId = 0; loopId < loopCount; ++loopId)
    {
        std::unique_lock<std::mutex> lock(copyMutex);
        zmq::message_t message;
        message.rebuild(srcData.data(), srcData.size(), nop, nullptr);
        std::cout << "Sending buffer " << loopId << "... ";
        zmqBufferOut.send(message, zmq::send_flags::none);

        while (true)
        {
            copyCondition.wait(lock);
            if (static_cast<size_t>(bufferCount) > loopId)
                break;
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Sent " << loopCount << " buffers of size " << static_cast<float>(bufferSize) / static_cast<float>(1 << 20) << " MB through shmdata, in " << duration << " us\n";
    std::cout << "Bandwidth : " << (static_cast<double>(loopCount * bufferSize) / static_cast<double>(1 << 20)) / (static_cast<double>(duration) / 1000000.0) << " MB/sec\n";

    continueThread = false;
    bufferInThread.join();
}
