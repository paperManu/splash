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
# Includes
#
include_directories(../include)
include_directories(../external/cppzmq)
include_directories(../external/glm)
include_directories(../external/hap/source)
include_directories(../external/imgui)
include_directories(../external/jsoncpp)
include_directories(../external/libltc/src)
include_directories(../external/piccante/include)
include_directories(../external/stb)
include_directories(../external/syphon/build/Release/Syphon.framework/Headers)

if (APPLE)
    include_directories(../external/glad/compatibility/include)
else()
    include_directories(../external/glad/core/include)
endif()

include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${GSL_INCLUDE_DIRS})
include_directories(${ZMQ_INCLUDE_DIRS})
include_directories(${SHMDATA_INCLUDE_DIRS})
include_directories(${GPHOTO_INCLUDE_DIRS})
include_directories(${FFMPEG_INCLUDE_DIRS})
include_directories(${PORTAUDIO_INCLUDE_DIRS})
include_directories(${OPENCV_INCLUDE_DIRS})
include_directories(${SNAPPY_INCLUDE_DIRS})
include_directories(${PYTHON_INCLUDE_DIRS})

link_directories(${GLFW_LIBRARY_DIRS})
link_directories(${GSL_LIBRARY_DIRS})
link_directories(${ZMQ_LIBRARY_DIRS})
link_directories(${SHMDATA_LIBRARY_DIRS})
link_directories(${GPHOTO_LIBRARY_DIRS})
link_directories(${FFMPEG_LIBRARY_DIRS})
link_directories(${PORTAUDIO_LIBRARY_DIRS})
link_directories(${OPENCV_LIBRARY_DIRS})
link_directories(${SNAPPY_LIBRARY_DIRS})
link_directories(${PYTHON_LIBRARY_DIRS})

#
# Targets
#
add_library(splash-${API_VERSION} STATIC world.cpp)
add_executable(splash splash-app.cpp)
add_executable(splash-scene splash-scene.cpp)

#
# Splash library
#
if (APPLE)
    add_dependencies(splash-${API_VERSION} Syphon)
endif()

target_sources(
    splash-${API_VERSION} PRIVATE
    basetypes.cpp
    camera.cpp
    cgUtils.cpp
    controller.cpp
    controller_blender.cpp
    controller_gui.cpp
    factory.cpp
    filter.cpp
    geometry.cpp
    gpuBuffer.cpp
    imageBuffer.cpp
    image.cpp
    link.cpp
    mesh_bezierPatch.cpp
    mesh.cpp
    object.cpp
    queue.cpp
    scene.cpp
    shader.cpp
    texture.cpp
    texture_image.cpp
    threadpool.cpp
    userInput.cpp
    userInput_dragndrop.cpp
    userInput_joystick.cpp
    userInput_keyboard.cpp
    userInput_mouse.cpp
    warp.cpp
    widget.cpp
    widget_control.cpp
    widget_global_view.cpp
    widget_graph.cpp
    widget_node_view.cpp
    widget_media.cpp
    widget_template.cpp
    widget_text_box.cpp
    widget_warp.cpp
    window.cpp
    ../external/imgui/imgui_draw.cpp
    ../external/imgui/imgui.cpp
	../external/jsoncpp/jsoncpp.cpp
	../external/hap/source/hap.c
)

if (APPLE)
    target_sources(
        splash-${API_VERSION} PRIVATE
	    ../external/glad/compatibility/src/glad_compatibility.c
    )
else()
    target_sources(
        splash-${API_VERSION} PRIVATE
	    ../external/glad/core/src/glad_core.c
    )
endif()

if (FFMPEG_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE image_ffmpeg.cpp)
endif()

if (GPHOTO_FOUND)
    target_sources(
        splash-${API_VERSION} PRIVATE
        colorcalibrator.cpp 
        image_gphoto.cpp
    )
endif()

if (OPENCV_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE image_opencv.cpp)
endif()

if (PORTAUDIO_FOUND)
    target_sources(
        splash-${API_VERSION} PRIVATE
        listener.cpp
        ltcclock.cpp
        speaker.cpp
        ../external/libltc/src/decoder.c
        ../external/libltc/src/encoder.c
        ../external/libltc/src/ltc.c
        ../external/libltc/src/timecode.c
    )
endif()

if (SHMDATA_FOUND)
    target_sources(
        splash-${API_VERSION} PRIVATE
        image_shmdata.cpp
        mesh_shmdata.cpp
    )
endif()

if (PYTHONLIBS_FOUND)
    target_sources(
        splash-${API_VERSION} PRIVATE
        controller_pythonEmbedded.cpp
    )
endif()

if (APPLE)
    target_sources(
        splash-${API_VERSION} PRIVATE
        texture_syphon.cpp
        texture_syphon_client.mm
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${CMAKE_CURRENT_SOURCE_DIR}/../external/syphon/build/Release -framework Syphon")
endif()

target_link_libraries(splash-${API_VERSION} ${FFMPEG_LIBRARIES})
target_link_libraries(splash-${API_VERSION} pthread)
target_link_libraries(splash-${API_VERSION} ${Boost_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GLFW_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GSL_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${ZMQ_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${SHMDATA_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GPHOTO_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${PORTAUDIO_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${OPENCV_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${SNAPPY_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${PYTHON_LIBRARIES})

#
# splash and splash-scene executables
#
target_link_libraries(splash splash-${API_VERSION})
target_link_libraries(splash-scene splash-${API_VERSION})

#
# Installation
#
install(TARGETS splash DESTINATION "bin/")
install(TARGETS splash-scene DESTINATION "bin/")

if (APPLE)
    target_link_libraries(splash "-undefined dynamic_lookup")
    target_link_libraries(splash-scene "-undefined dynamic_lookup")
endif()
