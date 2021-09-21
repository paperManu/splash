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
#include <string>
#include <vector>

#include <shmdata/follower.hpp>
#include <shmdata/writer.hpp>

#include "./utils/osutils.h"

const std::string shmpath = "/tmp/perf_shmdata_socket";
const size_t shmsize = 1 << 26;
const size_t loopCount = 1 << 8;
const std::string caps = "raw/nocaps";
Splash::Utils::ShmdataLogger shmlogger;

std::atomic_int bufferCount{0};
std::mutex copyMutex{};
std::condition_variable copyCondition{};

std::vector<uint8_t> srcData((size_t)shmsize);
std::vector<uint8_t> sinkData((size_t)shmsize);

void onData(void* data, size_t size)
{
    std::unique_lock<std::mutex> lock(copyMutex);
    std::copy(static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size, sinkData.data());
    std::cout << bufferCount << " received\n";
    bufferCount++;
    copyCondition.notify_all();
}

int main()
{

    shmdata::Writer writer(shmpath, shmsize, caps, &shmlogger);
    shmdata::Follower follower(
        shmpath, [&](void* data, size_t size) { onData(data, size); }, [&](const std::string& caps) {}, [&]() {}, &shmlogger);

    std::cout << "Preparing buffer data...\n";
    for (size_t i = 0; i < shmsize; ++i)
        srcData[i] = static_cast<uint8_t>(i % 256);

    const auto start = std::chrono::steady_clock::now();

    for (size_t loopId = 0; loopId < loopCount; ++loopId)
    {
        std::unique_lock<std::mutex> lock(copyMutex);
        writer.copy_to_shm(srcData.data(), srcData.size());
        std::cout << "Buffer " << loopId << " sent... ";

        while (true)
        {
            copyCondition.wait(lock);
            if (static_cast<size_t>(bufferCount) > loopId)
                break;
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Sent " << loopCount << " buffers of size " << static_cast<float>(shmsize) / static_cast<float>(1 << 20) << " MB through shmdata, in " << duration << " us\n";
    std::cout << "Bandwidth : " << (static_cast<double>(loopCount * shmsize) / static_cast<double>(1 << 20)) / (static_cast<double>(duration) / 1000000.0) << " MB/sec\n";
}
