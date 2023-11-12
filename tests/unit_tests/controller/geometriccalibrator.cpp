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

#include <doctest.h>

#include <calimiro/calimiro.h>

/*************/
TEST_CASE("Testing Calimiro integration to Splash")
{
    bool result = false;
    try
    {
        calimiro::Workspace workspace;
        result = true;
    }
    catch (...)
    {
    }
    CHECK(result == true);
}
