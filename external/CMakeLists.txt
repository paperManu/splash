#
# Copyright (C) 2016 Emmanuel Durand
#
# This file is part of Splash.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Splash is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Splash.  If not, see <http://www.gnu.org/licenses/>.
#

# Disable warnings from external libs
add_definitions(-w)

#
# External sources
#
if (NOT USE_SYSTEM_LIBS)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory(glfw EXCLUDE_FROM_ALL)
  
  set(SNAPPY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  add_subdirectory(snappy EXCLUDE_FROM_ALL)
endif()

if(APPLE)
    add_custom_command(
        OUTPUT SyphonBuild
        COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/syphon && xcodebuild -project Syphon.xcodeproj -configuration Release SYMROOT=./build DSTROOT=/
    )

    add_custom_target(Syphon DEPENDS SyphonBuild)

    add_custom_command(
        OUTPUT macdylibbundlerBuild
        COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/macdylibbundler && make
    )

    add_custom_target(macdylibbundler DEPENDS macdylibbundlerBuild)
endif()
