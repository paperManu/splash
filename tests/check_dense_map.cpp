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

#include <doctest.h>

#include <iostream>
#include <utility>
#include <vector>

#include "./utils/dense_map.h"

using namespace Splash;

/*************/
TEST_CASE("Testing Splash::DenseMap")
{
    auto dmap = DenseMap<int, float>();

    std::vector<std::pair<int, float>> values{{1, 1.f}, {2, 2.f}, {3, 3.f}};
    dmap = DenseMap<int, float>(values.begin(), values.end());
    CHECK(dmap.size() == values.size());
    CHECK(!dmap.empty());
    for (const auto& v : values)
        CHECK(dmap.at(v.first) == v.second);

    auto otherMap = DenseMap(dmap);
    for (const auto& v : values)
        CHECK(otherMap.at(v.first) == v.second);

    dmap.clear();
    CHECK(dmap.size() == 0);
    CHECK(dmap.empty());
    dmap = DenseMap(std::move(otherMap));
    for (const auto& v : values)
        CHECK(otherMap.at(v.first) == v.second);

    dmap = DenseMap<int, float>({{1, 1.f}, {2, 2.f}, {3, 3.f}, {42, 3.14159f}});
    CHECK(dmap[1] == 1.f);
    CHECK(dmap[42] == 3.14159f);

    dmap = {{4, 4.f}, {5, 5.f}, {6, 6.f}};
    CHECK(dmap[4] == 4.f);
    CHECK(dmap[6] == 6.f);
    CHECK(dmap[3] == 0.f);
    CHECK(dmap.at(5) == 5.f);
    CHECK(dmap.find(1) == dmap.end());
    CHECK(!(dmap < dmap));
    CHECK(!(dmap > dmap));
    CHECK(dmap <= dmap);
    CHECK(dmap >= dmap);

    auto dmapIt = dmap.insert({7, 7.f});
    CHECK(dmapIt.first->first == 7);
    CHECK(dmapIt.first->second == 7.f);
    CHECK(dmap.at(7) == 7.f);
    CHECK(dmap.size() == 5);

    dmapIt = dmap.insert({8, 8.f});
    CHECK(dmapIt.first->first == 8);
    CHECK(dmapIt.first->second == 8.f);
    CHECK(dmap.at(8) == 8.f);
    CHECK(dmap.size() == 6);

    dmap.insert_or_assign(2, 3.f);
    CHECK(dmap.at(2) == 3.f);

    dmap.emplace(1337, 7331.f);
    CHECK(dmap.at(1337) == 7331.f);

    dmapIt = dmap.try_emplace(4, 1.4142f);
    CHECK(dmapIt.second == false);
    CHECK(dmapIt.first->first == 4);
    CHECK(dmapIt.first->second == 4.f);
    CHECK(dmap.at(4) == 4.f);

    dmap = {{4, 4.f}, {5, 5.f}, {6, 6.f}, {7, 7.f}, {8, 8.f}};
    auto it = dmap.erase(dmap.find(6));
    CHECK(it->first == 7);
    CHECK(it->second == 7.f);
    otherMap = DenseMap<int, float>({{4, 4.f}, {5, 5.f}, {7, 7.f}, {8, 8.f}});
    CHECK(dmap == otherMap);

    dmap = {{4, 4.f}, {5, 5.f}, {6, 6.f}, {7, 7.f}, {8, 8.f}};
    it = dmap.erase(dmap.find(5), dmap.find(7));
    otherMap = DenseMap<int, float>({{4, 4.f}, {8, 8.f}});
    CHECK(dmap == otherMap);
}
