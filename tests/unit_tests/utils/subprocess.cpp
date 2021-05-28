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

#include <doctest.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "./utils/subprocess.h"

using namespace Splash::Utils;

/*************/
TEST_CASE("Testing Subprocess")
{
    {
        auto process = Subprocess("sleep", "1");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        CHECK_EQ(process.isRunning(), false);
    }

    {
        auto process = Subprocess("sleep", "2");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        CHECK_EQ(process.isRunning(), true);
    }
}
