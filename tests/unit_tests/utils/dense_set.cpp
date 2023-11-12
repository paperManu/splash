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

#include "./utils/dense_set.h"

using namespace Splash;

/*************/
TEST_CASE("Testing Splash::DenseSet")
{
    auto dset = DenseSet<int>();
    CHECK(dset.size() == 0);

    int integers[] = {1, 2, 3, 4, 2};
    dset = DenseSet<int>(integers, integers + 5);
    CHECK(dset.size() == 4);
    dset = DenseSet({1, 2, 3, 4, 1, 2, 3, 4});
    CHECK(dset.size() == 4);
    dset.insert(5);
    CHECK(dset.size() == 5);
    dset.insert(2);
    CHECK(dset.size() == 5);
    dset.insert({6, 7, 8});
    CHECK(dset.size() == 8);
    dset.emplace(9);
    CHECK(dset.size() == 9);
    dset.erase(dset.find(3));
    CHECK(dset.count(2) == 1);
    CHECK(dset.count(42) == 0);
    CHECK(dset.find(2) != dset.end());

    CHECK(DenseSet({1, 2, 3, 4}) == DenseSet({4, 3, 2, 1}));
    CHECK(DenseSet({1, 2, 3, 4}) != DenseSet({5, 3, 2, 1}));
    CHECK(DenseSet({1, 2, 3, 4}) != DenseSet({1, 2, 3}));
}
