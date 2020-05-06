/*
 * Copyright (C) 2019 Emmanuel Durand
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

#include "./utils/dense_map.h"

#include <chrono>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

using namespace Splash;

int main()
{
    const size_t count = 1 << 8;
    const size_t loopCount = 1 << 8;
    volatile int key;
    volatile float value;

    std::cout << "----> DenseMap performance test\n";

    /**
     * DenseMap
     */
    std::cout << "DenseMap::insert -> " << std::flush;
    DenseMap<int, float> dmap{};
    auto start = std::chrono::steady_clock::now();
    for (int loop = 0; loop < loopCount; ++loop)
    {
        dmap.clear();
        for (size_t i = 0; i < count; ++i)
            dmap.insert({i, static_cast<float>(i)});
    }
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << "µs\n";

    std::cout << "DenseMap::iterator -> " << std::flush;
    start = std::chrono::steady_clock::now();
    for (int loop = 0; loop < loopCount; ++loop)
        for (const auto& entry : dmap)
        {
            key = entry.first;
            value = entry.second;
        }
    end = std::chrono::steady_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << "µs\n";

    /**
     * std::map
     */
    std::cout << "std::map::insert -> " << std::flush;
    std::map<int, float> map{};
    start = std::chrono::steady_clock::now();
    for (int loop = 0; loop < loopCount; ++loop)
    {
        map.clear();
        for (size_t i = 0; i < count; ++i)
            map.insert({i, static_cast<float>(i)});
    }
    end = std::chrono::steady_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << "µs\n";

    std::cout << "std::map::iterator -> " << std::flush;
    start = std::chrono::steady_clock::now();
    for (int loop = 0; loop < loopCount; ++loop)
        for (const auto& entry : map)
        {
            key = entry.first;
            value = entry.second;
        }
    end = std::chrono::steady_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << "µs\n";

    /**
     * std::unordered_map
     */
    std::cout << "std::unordered_map::insert -> " << std::flush;
    std::unordered_map<int, float> unordered_map{};
    start = std::chrono::steady_clock::now();
    for (int loop = 0; loop < loopCount; ++loop)
    {
        unordered_map.clear();
        for (size_t i = 0; i < count; ++i)
            unordered_map.insert({i, static_cast<float>(i)});
    }
    end = std::chrono::steady_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << "µs\n";

    std::cout << "std::unordered_map::iterator -> " << std::flush;
    start = std::chrono::steady_clock::now();
    for (int loop = 0; loop < loopCount; ++loop)
        for (const auto& entry : unordered_map)
        {
            key = entry.first;
            value = entry.second;
        }
    end = std::chrono::steady_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << "µs\n";
}
