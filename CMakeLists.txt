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

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

#
# Package information
#
set(API_VERSION 0.3)
set(PACKAGE_VERSION 0.3.13)

# We compile using clang
set(CMAKE_CXX_COMPILER "clang++")

#
# Detect if we have a C++11 compiler
#
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

#
# Find the various libraries
#
find_package(Boost REQUIRED COMPONENTS system)
find_package(PkgConfig REQUIRED)

# Mandatory dependencies
find_package(Snappy REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
pkg_search_module(GSL REQUIRED gsl)
pkg_search_module(ZMQ REQUIRED libzmq)

# Optional dependencies
pkg_search_module(GPHOTO libgphoto2)
pkg_check_modules(FFMPEG libavformat libavcodec libavutil libswscale)
pkg_search_module(OPENCV opencv)
pkg_search_module(PORTAUDIO portaudio-2.0)
pkg_search_module(SHMDATA shmdata-1.2)

#
# Configuration
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLM_FORCE_RADIANS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDATADIR=\\\"${CMAKE_INSTALL_PREFIX}/share/splash/\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSPLASHPREFIX=\\\"${CMAKE_INSTALL_PREFIX}\\\"")

set(HAVE_SHMDATA ${SHMDATA_FOUND})
set(HAVE_GPHOTO ${GPHOTO_FOUND})
set(HAVE_FFMPEG ${FFMPEG_FOUND})
set(HAVE_PORTAUDIO ${PORTAUDIO_FOUND})
set(HAVE_OPENCV ${OPENCV_FOUND})
configure_file("${CMAKE_HOME_DIRECTORY}/include/config.h.cmake" "${CMAKE_HOME_DIRECTORY}/include/config.h")

#
# Sources
#
add_subdirectory(src)
