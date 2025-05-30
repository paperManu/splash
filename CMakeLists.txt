#
# Copyright (C) 2016 Splash authors
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

cmake_minimum_required(VERSION 3.16)

# Force the installation prefix, as under certain circumstances on Windows it defaults
# to "C:\Program Files (x86)", which is not correct and conflicts the installation path
# of NSIS.
# Note that this has to be set BEFORE the call to project(...), otherwise it will have
# no effect
if (WIN32)
    set(CMAKE_INSTALL_PREFIX "C:\\Program Files\\splash" CACHE PATH "Default directory for Splash installation on Windows")
endif()

project(
    splash
    VERSION 0.11.2
    LANGUAGES C CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

enable_testing()

#
# Package information
#
set(API_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
set(PACKAGE_VERSION ${PROJECT_VERSION})

#
# Build options
#
set(BUILD_GENERIC_ARCH OFF CACHE BOOL "Enable building generic binaries, without more modern extensions")
set(BUILD_CODE ON CACHE BOOL "Enable building the code")
set(DEBUG_OPENGL OFF CACHE BOOL "Enable OpenGL debugging")
set(PROFILE OFF CACHE BOOL "Enable profiling")
set(PROFILE_OPENGL OFF CACHE BOOL "Enable OpenGL profiling")
set(USE_SYSTEM_LIBS OFF CACHE BOOL "Enable using system libraries instead of bundle ones")
set(TEST_COVERAGE OFF CACHE BOOL "Enable test coverage")
set(WITH_LTO OFF CACHE BOOL "Activate link time optimization (LTO)")
set(WITH_SANITIZE OFF CACHE BOOL "Enable sanitize compile options")

include(CheckIPOSupported)
include(set_icon)

check_ipo_supported(RESULT IPO_SUPPORTED)
if (NOT IPO_SUPPORTED)
    set(WITH_LTO FALSE)
endif()

# Force color output (useful when building with Ninja)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   add_compile_options (-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   add_compile_options (-fcolor-diagnostics)
endif ()

#
# Find the various libraries
#
if (NOT USE_SYSTEM_LIBS)
  set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/external/third_parties/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
endif()

find_package(PkgConfig REQUIRED)
find_package(Threads)

if (BUILD_CODE)
    # Mandatory dependencies
    pkg_search_module(JSONCPP REQUIRED jsoncpp)
    pkg_search_module(GSL REQUIRED gsl)
    pkg_search_module(ZMQ REQUIRED libzmq)
    pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)

    # Optional dependencies
    find_package (Python3 COMPONENTS Interpreter Development)
    find_package(OpenCV)
    pkg_search_module(GPHOTO libgphoto2)
    pkg_search_module(JACK jack)
    pkg_search_module(PORTAUDIO portaudio-2.0)
    pkg_search_module(SH4LT sh4lt-0.1)
    pkg_search_module(SHMDATA shmdata-1.3)
    pkg_search_module(CALIMIRO calimiro-0.2)
endif()

find_program(ZIP zip)
find_package(Doxygen)

#
# Configuration
#
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLM_FORCE_RADIANS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDATADIR=\"\\\"${CMAKE_INSTALL_PREFIX}/share/splash/\\\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSPLASHPREFIX=\"\\\"${CMAKE_INSTALL_PREFIX}\\\"\"")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

if (TEST_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
endif()

set(HAVE_GPHOTO ${GPHOTO_FOUND})
set(HAVE_OPENCV ${OPENCV_FOUND})
set(HAVE_PORTAUDIO ${PORTAUDIO_FOUND})
set(HAVE_SH4LT ${SH4LT_FOUND})
set(HAVE_SHMDATA ${SHMDATA_FOUND})
set(HAVE_PYTHON ${Python3_FOUND})
set(HAVE_CALIMIRO ${CALIMIRO_FOUND})

if (FFMPEG_libavformat_VERSION LESS 57)
    message(WARNING "FFmpeg version is older than 3.1, support disabled")
    set(FFMPEG_FOUND 0)
endif()

if (UNIX)
    set(HAVE_LINUX 1)
elseif (WIN32)
    set(HAVE_WINDOWS 1)
endif()

if (DEBUG_OPENGL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUGGL")
endif()

if (PROFILE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRACY_ENABLE")
endif()

if (PROFILE_OPENGL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROFILE_GPU")
endif()

if ("${JACK_LIBRARIES}" STREQUAL "")
    set(HAVE_JACK 0)
else()
    set(HAVE_JACK 1)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h")

#
# Generic compilation flags
#
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE AND NOT BUILD_GENERIC_ARCH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

#
# Sources
#
add_subdirectory(addons)
add_subdirectory(docs)

if (BUILD_CODE)
    add_subdirectory(data)
    add_subdirectory(external)
    add_subdirectory(src)
    add_subdirectory(tests)
endif()

# Uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)

#
# CPack related info
# Creates package on Linux.
#
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Splash, a modular multi-projector video-mapping software")
set(CPACK_PACKAGE_VENDOR "Splash authors")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/License.md")
  
if (UNIX)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-mapper")
  set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
  set(CPACK_DEBIAN_PROJECT_HOMEPAGE "https://splashmapper.xyz")
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <emmanueldurand@protonmail.com>")
  set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Splash is a free (as in GPL) modular mapping software. Provided that the user creates a 3D model with UV mapping of the projection surface, Splash will take care of calibrating the videoprojectors (intrinsic and extrinsic parameters, blending), and feed them with the input video sources. Splash can handle multiple inputs, mapped on multiple 3D models, and has been tested with up to eight outputs on two graphic cards. It currently runs on a single computer but support for multiple computers is planned.")
  set(CPACK_DEBIAN_PACKAGE_SECTION "video")
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
elseif (WIN32)
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_PACKAGE_EXECUTABLES "splash" "Splash")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
  set(CPACK_NSIS_DISPLAY_NAME "splash")
  set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/data/share/icons/splash.ico")
  set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/data/share/icons/splash.ico")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\${CMAKE_PROJECT_NAME}.exe")
  set(CPACK_NSIS_HELP_LINK "https://splashmapper.xyz/en/contents.html")
  set(CPACK_NSIS_URL_INFO_ABOUT "https://splashmapper.xyz")
  set(CPACK_NSIS_MODIFY_PATH OFF)
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
  set(CPACK_PACKAGING_INSTALL_PREFIX "/")
endif()

include(CPack)

#
# Summary
#

set(_config_msg "\nSplash Configuration\n=====================")

function(info_cfg_option
    _setting
    )

    set(_msg "  - ${_setting}")
    string(LENGTH "${_msg}" _len)
    while("32" GREATER "${_len}")
        set(_msg "${_msg} ")
         math(EXPR _len "${_len} + 1")
    endwhile()

    set(_config_msg "${_config_msg}\n${_msg}${${_setting}}" PARENT_SCOPE)
endfunction()

function(info_cfg_text
    _text
    )

    set(_config_msg "${_config_msg}\n\n  ${_text}" PARENT_SCOPE)
endfunction()

info_cfg_option(USE_SYSTEM_LIBS)
info_cfg_option(GPHOTO_VERSION)
info_cfg_option(FFMPEG_libavformat_VERSION)
info_cfg_option(FFMPEG_libavcodec_VERSION)
info_cfg_option(FFMPEG_libavutil_VERSION)
info_cfg_option(FFMPEG_libswscale_VERSION)
info_cfg_option(OpenCV_VERSION)
info_cfg_option(PORTAUDIO_VERSION)
info_cfg_option(JACK_VERSION)
info_cfg_option(Python3_VERSION)
info_cfg_option(JSONCPP_VERSION)
info_cfg_option(SH4LT_VERSION)
info_cfg_option(SHMDATA_VERSION)
info_cfg_option(CALIMIRO_VERSION)
info_cfg_option(ZMQ_VERSION)
info_cfg_option(DOXYGEN_FOUND)
info_cfg_option(ZIP)
info_cfg_option(DEBUG_OPENGL)
info_cfg_option(PROFILE)
info_cfg_option(PROFILE_OPENGL)
info_cfg_option(WITH_LTO)

info_cfg_text("")
message("${_config_msg}")
