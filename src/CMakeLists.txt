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
# Enable all warnings as errors
#
add_definitions(-Wall -Wextra -Werror)
add_definitions(-Wno-unknown-pragmas)

# Disable warnings for some specific files
set_source_files_properties(
    ../external/imgui/imgui_demo.cpp
    ../external/imgui/imgui_draw.cpp
    ../external/imgui/imgui.cpp
	../external/jsoncpp/jsoncpp.cpp
	../external/hap/source/hap.c
    PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-all -Wno-extra -Wno-sign-compare -Wno-deprecated-declarations"
)

set_source_files_properties(
    graphics/camera.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-error=strict-aliasing -Wno-strict-aliasing"
)

set_source_files_properties(
    image/image.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-error=implicit-fallthrough= -Wno-implicit-fallthrough"
)

#
# Includes
#
include_directories(./)
include_directories(./core)
include_directories(./controller)
include_directories(./controller/widget)
include_directories(./image)
include_directories(./mesh)
include_directories(./sink)
include_directories(./sound)
include_directories(./userinput)
include_directories(./utils)

include_directories(../external/cppzmq)
include_directories(../external/glm)
include_directories(../external/hap/source)
include_directories(../external/imgui)
include_directories(../external/jsoncpp)
include_directories(../external/libltc/src)
include_directories(../external/piccante/include)
include_directories(../external/stb)
include_directories(../external/syphon/build/Release/Syphon.framework/Headers)

if (HAVE_DATAPATH)
    include_directories(${DATAPATH_SDK_PATH}/include)
    include_directories(${DATAPATH_SDK_PATH}/utils/include)
endif()

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

# Distributed third parties first
link_directories(${FFMPEG_LIBRARY_DIRS})
link_directories(${SNAPPY_LIBRARY_DIRS})
link_directories(${ZMQ_LIBRARY_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})

link_directories(${GSL_LIBRARY_DIRS})
link_directories(${SHMDATA_LIBRARY_DIRS})
link_directories(${GPHOTO_LIBRARY_DIRS})
link_directories(${PORTAUDIO_LIBRARY_DIRS})
link_directories(${OPENCV_LIBRARY_DIRS})
link_directories(${PYTHON_LIBRARY_DIRS})

#
# Targets
#
add_library(splash-${API_VERSION} STATIC core/world.cpp)
add_executable(splash splash-app.cpp)

#
# Splash library
#
if (APPLE)
    add_dependencies(splash-${API_VERSION} Syphon)
endif()

target_sources(splash-${API_VERSION} PRIVATE
    core/attribute.cpp
    core/base_object.cpp
    core/buffer_object.cpp
    core/factory.cpp
    core/imagebuffer.cpp
    core/link.cpp
    core/root_object.cpp
    core/scene.cpp
    controller/controller.cpp
    controller/controller_blender.cpp
    controller/controller_gui.cpp
    controller/widget/widget.cpp
    controller/widget/widget_control.cpp
    controller/widget/widget_filters.cpp
    controller/widget/widget_global_view.cpp
    controller/widget/widget_graph.cpp
    controller/widget/widget_node_view.cpp
    controller/widget/widget_media.cpp
    controller/widget/widget_meshes.cpp
    controller/widget/widget_template.cpp
    controller/widget/widget_text_box.cpp
    controller/widget/widget_textures_view.cpp
    controller/widget/widget_warp.cpp
    graphics/camera.cpp
    graphics/filter.cpp
    graphics/framebuffer.cpp
    graphics/geometry.cpp
    graphics/gpu_buffer.cpp
    graphics/object.cpp
    graphics/shader.cpp
    graphics/texture.cpp
    graphics/texture_image.cpp
    graphics/virtual_probe.cpp
    graphics/warp.cpp
    graphics/window.cpp
    image/image.cpp
    image/image_ffmpeg.cpp
    image/queue.cpp
    mesh/mesh.cpp
    mesh/mesh_bezierpatch.cpp
    sink/sink.cpp
    userinput/userinput.cpp
    userinput/userinput_dragndrop.cpp
    userinput/userinput_joystick.cpp
    userinput/userinput_keyboard.cpp
    userinput/userinput_mouse.cpp
    utils/cgutils.cpp
    ../external/imgui/imgui_demo.cpp
    ../external/imgui/imgui_draw.cpp
    ../external/imgui/imgui.cpp
	../external/jsoncpp/jsoncpp.cpp
	../external/hap/source/hap.c
)

if (APPLE)
    target_sources(splash-${API_VERSION} PRIVATE
	    ../external/glad/compatibility/src/glad_compatibility.c
        texture_syphon.cpp
        texture_syphon_client.mm
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${CMAKE_CURRENT_SOURCE_DIR}/../external/syphon/build/Release -framework Syphon")
else()
    target_sources(splash-${API_VERSION} PRIVATE
	    ../external/glad/core/src/glad_core.c
        image/image_v4l2.cpp
    )
endif()

if (GPHOTO_FOUND)
    set_source_files_properties(
        controller/colorcalibrator.cpp 
        PROPERTIES COMPILE_FLAGS "-Wno-maybe-uninitialized -Wno-unused-result -Wno-int-in-bool-context -Wno-sign-compare -Wno-ignored-attributes -Wno-deprecated-declarations -Wno-delete-non-virtual-dtor -Wno-unused-parameter -Wno-strict-aliasing -Wno-format -Wno-unused-but-set-variable -Wno-misleading-indentation -Wno-switch"
    )

    target_sources(splash-${API_VERSION} PRIVATE
        controller/colorcalibrator.cpp 
        image/image_gphoto.cpp
    )
endif()

if (OPENCV_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE 
        image/image_opencv.cpp
    )
endif()

if (PORTAUDIO_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE
        sound/listener.cpp
        sound/ltcclock.cpp
        sound/speaker.cpp
        sound/sound_engine.cpp
        ../external/libltc/src/decoder.c
        ../external/libltc/src/encoder.c
        ../external/libltc/src/ltc.c
        ../external/libltc/src/timecode.c
    )
endif()

if (SHMDATA_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE
        image/image_shmdata.cpp
        mesh/mesh_shmdata.cpp
        sink/sink_shmdata.cpp
        sink/sink_shmdata_encoded.cpp
    )
endif()

if (PYTHONLIBS_FOUND)
    set_source_files_properties(
        controller/controller_pythonembedded.cpp
        PROPERTIES COMPILE_FLAGS -Wno-missing-field-initializers
    )

    target_sources(splash-${API_VERSION} PRIVATE
        controller/controller_pythonembedded.cpp
    )
endif()

# Distributed third parties first
target_link_libraries(splash-${API_VERSION} ${FFMPEG_LIBRARIES})
target_link_libraries(splash-${API_VERSION} snappy)
target_link_libraries(splash-${API_VERSION} glfw)
target_link_libraries(splash-${API_VERSION} zmq.a)

target_link_libraries(splash-${API_VERSION} pthread)
target_link_libraries(splash-${API_VERSION} ${Boost_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GSL_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${SHMDATA_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GPHOTO_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${PORTAUDIO_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${OPENCV_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${PYTHON_LIBRARIES})

#
# splash executable
#
target_link_libraries(splash splash-${API_VERSION})

#
# Installation
#
install(TARGETS splash DESTINATION "bin/")

if (APPLE)
    target_link_libraries(splash "-undefined dynamic_lookup")
endif()
