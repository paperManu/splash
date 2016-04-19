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

#
# Targets
#
add_library(splash-${API_VERSION} STATIC world.cpp)
add_executable(splash splash-app.cpp)
add_executable(splash-scene splash-scene.cpp)

#
# Includes
#
include_directories(../include)
include_directories(../external/ccpzmq)
include_directories(../external/glm)
include_directories(../external/glad/core/include)
include_directories(../external/hap/source)
include_directories(../external/imgui)
include_directories(../external/jsoncpp)
include_directories(../external/libltc/src)
include_directories(../external/piccante/include)
include_directories(../external/stb)
include_directories(../external/syphon/build/Release/Syphon.framework/Headers)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${GSL_INCLUDE_DIRS})
include_directories(${ZMQ_INCLUDE_DIRS})
include_directories(${SHMDATA_INCLUDE_DIRS})
include_directories(${GPHOTO_INCLUDE_DIRS})
include_directories(${FFMPEG_INCLUDE_DIRS})
include_directories(${PORTAUDIO_INCLUDE_DIRS})
include_directories(${OPENCV_INCLUDE_DIRS})
include_directories(${SNAPPY_INCLUDE_DIRS})

#
# Splash library
#
target_compile_features(splash-${API_VERSION} PRIVATE cxx_variadic_templates)
target_sources(splash-${API_VERSION} PRIVATE
               camera.cpp
               cgUtils.cpp
               colorcalibrator.cpp
               filter.cpp
               geometry.cpp
               gpuBuffer.cpp
               gui.cpp
               imageBuffer.cpp
               image.cpp
               link.cpp
               listener.cpp
               ltcclock.cpp
               mesh_bezierPatch.cpp
               mesh.cpp
               mesh_shmdata.cpp
               object.cpp
               queue.cpp
               scene.cpp
               shader.cpp
               speaker.cpp
               texture.cpp
               texture_image.cpp
               threadpool.cpp
               warp.cpp
               widgets.cpp
               window.cpp
               ../external/imgui/imgui_draw.cpp
               ../external/imgui/imgui.cpp
	           ../external/glad/core/src/glad_core.c
	           ../external/jsoncpp/jsoncpp.cpp
	           ../external/hap/source/hap.c
               ../external/libltc/src/decoder.c
               ../external/libltc/src/encoder.c
               ../external/libltc/src/ltc.c
               ../external/libltc/src/timecode.c
)

if (Boost_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE httpServer.cpp)
endif()

if (FFMPEG_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE image_ffmpeg.cpp)
endif()

if (GPHOTO_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE image_gphoto.cpp)
endif()

if (OPENCV_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE image_opencv.cpp)
endif()

if (SHMDATA_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE image_shmdata.cpp)
endif()

if (APPLE)
    target_sources(splash-${API_VERSION} PRIVATE
                   texture_syphon.cpp
                   texture_syphon_client.mm)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F$(top_srcdir)/external/syphon/build/Release -framework Syphon")
endif()

target_link_libraries(splash-${API_VERSION} ${Boost_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GLFW_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GSL_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${ZMQ_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${SHMDATA_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GPHOTO_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${FFMPEG_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${PORTAUDIO_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${OPENCV_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${SNAPPY_LIBRARIES})

#
# splash and splash-scene executables
#
target_compile_features(splash PRIVATE cxx_variadic_templates)
target_link_libraries(splash splash-${API_VERSION})

target_compile_features(splash-scene PRIVATE cxx_variadic_templates)
target_link_libraries(splash-scene splash-${API_VERSION})

#
# Installation
#
install(TARGETS splash-${API_VERSION} DESTINATION "lib/")
install(TARGETS splash DESTINATION "bin/")
install(TARGETS splash-scene DESTINATION "bin/")
