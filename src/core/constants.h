/*
 * Copyright (C) 2014 Emmanuel Durand
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

/*
 * @constants.h
 * Splash-wide constants, and useful defines
 */

#ifndef SPLASH_CONSTANTS_H
#define SPLASH_CONSTANTS_H

#include "./config.h"

#define SPLASH
#define SPLASH_GL_DEBUG true

#define SPLASH_ALL_PEERS "__ALL__"
#define SPLASH_DEFAULTS_FILE_ENV "SPLASH_DEFAULTS"

#define SPLASH_FILE_CONFIGURATION "splashConfiguration"
#define SPLASH_FILE_PROJECT "splashProject"

#define SPLASH_CAMERA_LINK "__camera_link"

#define GL_VENDOR_NVIDIA "NVIDIA Corporation"
#define GL_VENDOR_AMD "X.Org"
#define GL_VENDOR_INTEL "Intel"

#define GL_TIMING_PREFIX "__gl_timing_"
#define GL_TIMING_TIME_PER_FRAME "time_per_frame"
#define GL_TIMING_TEXTURES_UPLOAD "texture_upload"
#define GL_TIMING_RENDERING "rendering"
#define GL_TIMING_SWAP "swap"

#include <execinfo.h>
#include <iostream>

// clang-format off
#include "./glad/glad.h"
#include <GLFW/glfw3.h>
// clang-format on

#define PRINT_FUNCTION_LINE std::cout << "------> " << __PRETTY_FUNCTION__ << "::" << __LINE__ << std::endl;

#define PRINT_CALL_STACK                                                                                                                                                           \
    {                                                                                                                                                                              \
        int j, nptrs;                                                                                                                                                              \
        int size = 100;                                                                                                                                                            \
        void* buffers[size];                                                                                                                                                       \
        char** strings;                                                                                                                                                            \
                                                                                                                                                                                   \
        nptrs = backtrace(buffers, size);                                                                                                                                          \
        strings = backtrace_symbols(buffers, nptrs);                                                                                                                               \
        for (j = 0; j < nptrs; ++j)                                                                                                                                                \
            std::cout << strings[j] << std::endl;                                                                                                                                  \
                                                                                                                                                                                   \
        free(strings);                                                                                                                                                             \
    }

#endif // SPLASH_CONSTANTS_H
