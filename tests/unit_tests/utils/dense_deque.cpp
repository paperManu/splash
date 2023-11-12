/*
 * Copyright (C) 2019 Splash authors
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

#include "./utils/dense_deque.h"

using namespace Splash;

/*************/
TEST_CASE("Testing Splash::DenseDeque")
{
    auto ddeque = DenseDeque<float>();
    CHECK(ddeque.size() == 0);

    ddeque = DenseDeque<float>((size_t)8);
    CHECK(ddeque.size() == 8);

    float integers[] = {1.f, 2.f, 3.f, 4.f};
    ddeque = DenseDeque<float>(integers, integers + 4);
    CHECK(ddeque.size() == 4);
    ddeque = DenseDeque({1.f, 2.f, 3.f, 4.f});
    CHECK(ddeque[3] == 4.f);
    CHECK(ddeque.front() == 1.f);
    CHECK(ddeque.back() == 4.f);

    ddeque.push_back(5.f);
    CHECK(ddeque.back() == 5.f);
    ddeque.emplace_back(6.f);
    CHECK(ddeque.back() == 6.f);
    ddeque.pop_back();
    CHECK(ddeque.back() == 5.f);

    ddeque.push_front(0.f);
    CHECK(ddeque.front() == 0.f);
    ddeque.emplace_front(-1.f);
    CHECK(ddeque.front() == -1.f);
    ddeque.pop_front();
    CHECK(ddeque.front() == 0.f);
}
