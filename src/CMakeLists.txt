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

#
# Enable all warnings as errors
# This is disable for anything but Linux, as it messes up some targets
# for other platforms. More particularly the icon setting on Windows.
# To solve this, the generation of the splash binary should be done in
# another directory...
#
if(UNIX)
    add_definitions(-Wall -Wextra -Werror)
    add_definitions(-Wno-unknown-pragmas)
endif()

if (WITH_SANITIZE)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
        add_definitions(-Wno-array-bounds -Wno-stringop-overflow -Wno-maybe-uninitialized)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fsanitize=address -fsanitize=undefined -fsanitize=integer)
        add_link_options(-fsanitize=address -fsanitize=undefined -fsanitize=integer)
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLM_ENABLE_EXPERIMENTAL")

# Disable warnings for some specific files
set_source_files_properties(
    ../external/imgui/imgui_demo.cpp
    ../external/imgui/imgui_draw.cpp
    ../external/imgui/imgui_tables.cpp
    ../external/imgui/imgui_widgets.cpp
    ../external/imgui/imgui.cpp
    ../external/hap/source/hap.c
    PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-all -Wno-extra -Wno-sign-compare -Wno-deprecated-declarations"
)

set_source_files_properties(
    ../external/tracy/public/TracyClient.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-all -Wno-extra -Wno-unused-parameter -Wno-unused-result"
)

set_source_files_properties(
    graphics/shader.cpp
    graphics/object.cpp
    graphics/camera.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-type-limits"
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_source_files_properties(
        network/channel_zmq.cpp
        PROPERTIES COMPILE_FLAGS "-Wno-maybe-uninitialized"
    )

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
        set_source_files_properties(
            image/image.cpp
            PROPERTIES COMPILE_FLAGS "-Wno-missing-field-initializers -Wno-stringop-overflow"
        )
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # This needs to be set because of stb_image.h
  set_source_files_properties(
      image/image.cpp
      PROPERTIES COMPILE_FLAGS "-Wno-missing-field-initializers"
  )
endif()

#
# Includes
#
include_directories(./)

if (NOT USE_SYSTEM_LIBS)
    include_directories(../external/glm)
endif()

include_directories(../external/cppzmq)
include_directories(../external/datapath/include)
include_directories(../external/glad/core/include)
include_directories(../external/hap/source)
include_directories(../external/imgui)
include_directories(../external/libltc/src)
include_directories(../external/ndi-sdk/include)
include_directories(../external/stb)
include_directories(../external/tracy/public)
include_directories(../external/stduuid/include)
include_directories(../external/stduuid)

include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${GSL_INCLUDE_DIRS})
include_directories(${ZMQ_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${SH4LT_INCLUDE_DIRS})
include_directories(${SHMDATA_INCLUDE_DIRS})
include_directories(${GPHOTO_INCLUDE_DIRS})
include_directories(${FFMPEG_INCLUDE_DIRS})
include_directories(${PORTAUDIO_INCLUDE_DIRS})
include_directories(${SNAPPY_INCLUDE_DIRS})
include_directories(${Python3_INCLUDE_DIRS})
include_directories(${CALIMIRO_INCLUDE_DIRS})

# Distributed third parties first
link_directories(${FFMPEG_LIBRARY_DIRS})
link_directories(${SNAPPY_LIBRARY_DIRS})
link_directories(${ZMQ_LIBRARY_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})

link_directories(${JSONCPP_LIBRARY_DIRS})
link_directories(${GSL_LIBRARY_DIRS})
link_directories(${SH4LT_LIBRARY_DIRS})
link_directories(${SHMDATA_LIBRARY_DIRS})
link_directories(${GPHOTO_LIBRARY_DIRS})
link_directories(${PORTAUDIO_LIBRARY_DIRS})
link_directories(${Python3_LIBRARY_DIRS})
link_directories(${UUID_LIBRARY_DIRS})
link_directories(${CALIMIRO_LIBRARY_DIRS})

#
# Targets
#
add_library(splash-${API_VERSION} STATIC core/world.cpp)
add_executable(splash splash-app.cpp)

if(${WITH_LTO})
    set_property(TARGET splash-${API_VERSION}
        PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE
    )

    set_property(TARGET splash
        PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()

#
# Splash library
#
target_sources(splash-${API_VERSION} PRIVATE
    core/attribute.cpp
    core/base_object.cpp
    core/buffer_object.cpp
    core/factory.cpp
    core/graph_object.cpp
    core/imagebuffer.cpp
    core/name_registry.cpp
    core/root_object.cpp
    core/scene.cpp
    core/tree/tree_branch.cpp
    core/tree/tree_leaf.cpp
    core/tree/tree_root.cpp
    controller/controller.cpp
    controller/controller_blender.cpp
    controller/controller_gui.cpp
    controller/widget/widget.cpp
    controller/widget/widget_attributes.cpp
    controller/widget/widget_calibration.cpp
    controller/widget/widget_control.cpp
    controller/widget/widget_filters.cpp
    controller/widget/widget_camera.cpp
    controller/widget/widget_graph.cpp
    controller/widget/widget_node_view.cpp
    controller/widget/widget_media.cpp
    controller/widget/widget_meshes.cpp
    controller/widget/widget_text_box.cpp
    controller/widget/widget_textures_view.cpp
    controller/widget/widget_tree.cpp
    controller/widget/widget_warp.cpp
    graphics/api/gpu_buffer.cpp
    graphics/camera.cpp
    graphics/filter.cpp
    graphics/filter_black_level.cpp
    graphics/filter_color_curves.cpp
    graphics/filter_custom.cpp
    graphics/geometry.cpp
    graphics/object.cpp
    graphics/object_library.cpp
    graphics/rendering_context.cpp
    graphics/shader.cpp
    graphics/texture.cpp
    graphics/texture_image.cpp
    graphics/virtual_probe.cpp
    graphics/warp.cpp
    graphics/window.cpp
    image/image.cpp
    image/image_ffmpeg.cpp
    image/image_list.cpp
    image/image_ndi.cpp
    image/queue.cpp
    mesh/mesh.cpp
    mesh/mesh_bezierpatch.cpp
    mesh/mesh_depthmap.cpp
    network/channel_zmq.cpp
    network/link.cpp
    sink/sink.cpp
    userinput/userinput.cpp
    userinput/userinput_dragndrop.cpp
    userinput/userinput_joystick.cpp
    userinput/userinput_keyboard.cpp
    userinput/userinput_mouse.cpp
    utils/cgutils.cpp
    utils/jsonutils.cpp
    utils/uuid.cpp

    # OpenGL ES API
    graphics/api/renderer.cpp
    graphics/api/gles/framebuffer_gfx_impl.cpp
    graphics/api/gles/geometry_gfx_impl.cpp
    graphics/api/gles/gl_utils.cpp
    graphics/api/gles/gpu_buffer.cpp
    graphics/api/gles/compute_shader_gfx_impl.cpp
    graphics/api/gles/feedback_shader_gfx_impl.cpp
    graphics/api/gles/graphic_shader_gfx_impl.cpp
    graphics/api/gles/gui_gfx_impl.cpp
    graphics/api/gles/pbo_gfx_impl.cpp
    graphics/api/gles/renderer.cpp
    graphics/api/gles/shader_gfx_impl.cpp
    graphics/api/gles/shader_stage.cpp
    graphics/api/gles/shader_program.cpp
    graphics/api/gles/texture_image_gfx_impl.cpp
    graphics/api/gles/window_gfx_impl.cpp

    # OpenGL API
    graphics/api/opengl/framebuffer_gfx_impl.cpp
    graphics/api/opengl/geometry_gfx_impl.cpp
    graphics/api/opengl/gl_utils.cpp
    graphics/api/opengl/gpu_buffer.cpp
    graphics/api/opengl/compute_shader_gfx_impl.cpp
    graphics/api/opengl/feedback_shader_gfx_impl.cpp
    graphics/api/opengl/graphic_shader_gfx_impl.cpp
    graphics/api/opengl/gui_gfx_impl.cpp
    graphics/api/opengl/pbo_gfx_impl.cpp
    graphics/api/opengl/renderer.cpp
    graphics/api/opengl/shader_gfx_impl.cpp
    graphics/api/opengl/shader_stage.cpp
    graphics/api/opengl/shader_program.cpp
    graphics/api/opengl/texture_image_gfx_impl.cpp
    graphics/api/opengl/window_gfx_impl.cpp

    # External sources
    ../external/glad/core/src/glad_core.c
    ../external/imgui/imgui_demo.cpp
    ../external/imgui/imgui_draw.cpp
    ../external/imgui/imgui_tables.cpp
    ../external/imgui/imgui_widgets.cpp
    ../external/imgui/imgui.cpp
    ../external/hap/source/hap.c
    ../external/tracy/public/TracyClient.cpp
)

if (UNIX)
    target_sources(splash-${API_VERSION} PRIVATE
    	utils/subprocess.cpp
        image/image_v4l2.cpp
    )

endif()

if (GPHOTO_FOUND AND OPENCV_FOUND)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set_source_files_properties(
            controller/colorcalibrator.cpp
            PROPERTIES COMPILE_FLAGS "-Wno-maybe-uninitialized -Wno-type-limits"
        )
    endif()

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

if (CALIMIRO_FOUND AND OPENCV_FOUND)
    target_sources(splash-${API_VERSION} PRIVATE
        controller/geometriccalibrator.cpp
        controller/texcoordgenerator.cpp
    )
endif()

if (HAVE_SH4LT)
    target_sources(splash-${API_VERSION} PRIVATE
        image/image_sh4lt.cpp
        sink/sink_sh4lt.cpp
        sink/sink_sh4lt_encoded.cpp
    )
endif()

if (HAVE_SHMDATA)
    target_sources(splash-${API_VERSION} PRIVATE
        image/image_shmdata.cpp
        mesh/mesh_shmdata.cpp
        network/channel_shmdata.cpp
        sink/sink_shmdata.cpp
        sink/sink_shmdata_encoded.cpp
    )
endif()

if (HAVE_PYTHON)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set_source_files_properties(
            controller/controller_pythonembedded.cpp
            controller/python/python_sink.cpp
        PROPERTIES COMPILE_FLAGS "-Wno-missing-field-initializers -Wno-cast-function-type"
        )
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set_source_files_properties(
            controller/controller_pythonembedded.cpp
            controller/python/python_sink.cpp
        PROPERTIES COMPILE_FLAGS "-Wno-missing-field-initializers"
        )
    endif()

    target_sources(splash-${API_VERSION} PRIVATE
        controller/controller_pythonembedded.cpp
        controller/python/python_sink.cpp
    )
endif()

# Stdlib
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0.0")
    target_link_libraries(splash-${API_VERSION} stdc++fs)
   endif()
else()
  target_link_libraries(splash-${API_VERSION} stdc++fs)
endif()

# Distributed third parties
target_link_libraries(splash-${API_VERSION} snappy)
target_link_libraries(splash-${API_VERSION} glfw)

if (UNIX)
    target_link_libraries(splash-${API_VERSION} dl) # Needed for Tracy
endif()

if (USE_SYSTEM_LIBS)
    target_link_libraries(splash-${API_VERSION} zmq)
    if (HAVE_SH4LT)
        target_link_libraries(splash-${API_VERSION} sh4lt-0.1)
    endif()
else()
    target_link_libraries(splash-${API_VERSION} zmq.a)
    if (HAVE_SH4LT)
        target_link_libraries(splash-${API_VERSION} sh4lt-0.1_static.a)
    endif()
endif()

# System libs
target_link_libraries(splash-${API_VERSION} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(splash-${API_VERSION} ${JSONCPP_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${FFMPEG_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GSL_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${SH4LT_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${SHMDATA_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${GPHOTO_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${PORTAUDIO_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${OpenCV_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${Python3_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${UUID_LIBRARIES})
target_link_libraries(splash-${API_VERSION} ${CALIMIRO_LIBRARIES})

#
# splash executable
#
target_link_libraries(splash splash-${API_VERSION})

if(WIN32)
  set_icon(splash "data/share/icons/splash.ico")
endif()

#
# Installation
#
install(TARGETS splash DESTINATION "bin/")

if (WIN32)
  get_filename_component(MINGW_PATH ${CMAKE_CXX_COMPILER} PATH)
  install(TARGETS splash
    RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES
      "api-ms-"
    POST_EXCLUDE_REGEXES
      ".*system32/.*\\.dll"
      ".*/msys64/mingw64/bin.*"
    DIRECTORIES
      ${MINGW_PATH}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

install(FILES
    shaders/geometric_calibration_filter.frag
    shaders/mesh_preview_shader.frag
    shaders/mesh_preview_shader.geom
    DESTINATION "share/splash/shaders")
