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

include_directories(../src/)

include_directories(../external/doctest/doctest/)
include_directories(../external/cppzmq)
include_directories(../external/glm)
include_directories(../external/hap/source)
include_directories(../external/imgui)
include_directories(../external/libltc/src)
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
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${SHMDATA_INCLUDE_DIRS})
include_directories(${GPHOTO_INCLUDE_DIRS})
include_directories(${FFMPEG_INCLUDE_DIRS})
include_directories(${PORTAUDIO_INCLUDE_DIRS})
include_directories(${OPENCV_INCLUDE_DIRS})
include_directories(${SNAPPY_INCLUDE_DIRS})
include_directories(${Python3_INCLUDE_DIRS})
include_directories(${CALIMIRO_INCLUDE_DIRS})

# Distributed third parties first
link_directories(${FFMPEG_LIBRARY_DIRS})
link_directories(${SNAPPY_LIBRARY_DIRS})
link_directories(${ZMQ_LIBRARY_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})

link_directories(${GSL_LIBRARY_DIRS})
link_directories(${JSONCPP_LIBRARY_DIRS})
link_directories(${SHMDATA_LIBRARY_DIRS})
link_directories(${GPHOTO_LIBRARY_DIRS})
link_directories(${PORTAUDIO_LIBRARY_DIRS})
link_directories(${OPENCV_LIBRARY_DIRS})
link_directories(${Python3_LIBRARY_DIRS})

#
# Custom target to download the splash-assets repository
#
add_custom_command(OUTPUT clone_splash_assets
    COMMAND if [ ! -d ${CMAKE_CURRENT_SOURCE_DIR}/assets ]; then $(git clone https://gitlab.com/sat-metalab/splash-assets ${CMAKE_CURRENT_SOURCE_DIR}/assets); fi
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/assets && git pull origin master
)
add_custom_target(download_splash_assets DEPENDS clone_splash_assets)

#
# Unit tests (executed through 'make check' or 'make test')
#
add_executable(unitTests unit_tests/unitTests.cpp)
target_sources(unitTests PRIVATE
    unit_tests/all_attributes.cpp
    unit_tests/core/attribute.cpp
    unit_tests/core/base_object.cpp
    unit_tests/core/factory.cpp
    unit_tests/core/graph_object.cpp
    unit_tests/core/imagebuffer.cpp
    unit_tests/core/name_registry.cpp
    unit_tests/core/root_object.cpp
    unit_tests/core/buffer_object.cpp
    unit_tests/core/scene.cpp
    unit_tests/core/serializer.cpp
    unit_tests/core/tree.cpp
    unit_tests/core/value.cpp
    unit_tests/core/world.cpp
    unit_tests/core/serialize/serialize_imagebuffer.cpp
    unit_tests/core/serialize/serialize_mesh.cpp
    unit_tests/image/image.cpp
    unit_tests/image/image_list.cpp
    unit_tests/utils/dense_deque.cpp
    unit_tests/utils/dense_map.cpp
    unit_tests/utils/dense_set.cpp
    unit_tests/utils/file_access.cpp
    unit_tests/utils/jsonutils.cpp
    unit_tests/utils/resizable_array.cpp
    unit_tests/utils/scope_guard.cpp
    unit_tests/utils/subprocess.cpp
    unit_tests/controller/colorcalibrator.cpp
)

if (HAVE_CALIMIRO)
target_sources(unitTests PRIVATE
    unit_tests/controller/geometriccalibrator.cpp
)
endif()

if (HAVE_SHMDATA)
target_sources(unitTests PRIVATE
    unit_tests/network/channel_shmdata.cpp
)
endif()

if(${WITH_LTO})
    set_property(TARGET unitTests
        PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()

if (NOT "${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    add_custom_command(OUTPUT update_assets
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/data
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/data/*.json ${CMAKE_CURRENT_BINARY_DIR}/data/
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/data/*.obj ${CMAKE_CURRENT_BINARY_DIR}/data/
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/../data/share/splash/color_map.png ${CMAKE_CURRENT_BINARY_DIR}/data/

        )
    add_custom_target(assets DEPENDS update_assets)
    add_dependencies(unitTests assets)
endif()

add_dependencies(unitTests download_splash_assets)
target_link_libraries(unitTests splash-${API_VERSION})

add_test(NAME unitTests COMMAND unitTests)

add_custom_command(OUTPUT tests COMMAND unitTests)
add_custom_target(check DEPENDS tests)

#
# Code coverage
#
if (TEST_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_custom_command(OUTPUT test_coverage_initial
            COMMAND mkdir -p ${CMAKE_BINARY_DIR}/coverage
            COMMAND lcov --no-external --capture --initial
                --directory ${CMAKE_SOURCE_DIR}
                --exclude '${CMAKE_SOURCE_DIR}/external/*'
                --exclude '${CMAKE_SOURCE_DIR}/tests/*'
                --output-file coverage/splash_base.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            )

        add_custom_command(OUTPUT test_coverage
            COMMAND lcov --no-external --capture
                --directory ${CMAKE_SOURCE_DIR}
                --exclude '${CMAKE_SOURCE_DIR}/external/*'
                --exclude '${CMAKE_SOURCE_DIR}/tests/*'
                --output-file coverage/splash.info
            COMMAND lcov --add-tracefile ${CMAKE_BINARY_DIR}/coverage/splash_base.info
                --add-tracefile ${CMAKE_BINARY_DIR}/coverage/splash.info
                --output-file ${CMAKE_BINARY_DIR}/coverage/splash_total.info
            COMMAND genhtml --output-directory ${CMAKE_BINARY_DIR}/coverage ${CMAKE_BINARY_DIR}/coverage/splash_total.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            )

        add_custom_target(check_coverage DEPENDS test_coverage_initial tests test_coverage)
    endif()
endif()

#
# Integration tests (executed by launching Splash and checking its behavior)
#
add_custom_command(OUTPUT integration_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../src/splash -P ${CMAKE_CURRENT_SOURCE_DIR}/integration_tests/integration_tests.py ${CMAKE_CURRENT_SOURCE_DIR}/integration_tests/integrationTests.json
    DEPENDS splash
    )
add_custom_target(check_integration DEPENDS integration_tests download_splash_assets)

#
# Static analysis through CppCheck
#
add_custom_command(OUTPUT cppcheck_analysis
    COMMAND cppcheck --enable=warning,performance,unusedFunction --project=${CMAKE_CURRENT_BINARY_DIR}/../compile_commands.json 2> ${CMAKE_CURRENT_BINARY_DIR}/static_analysis_result.txt
)
add_custom_target(static_analysis DEPENDS cppcheck_analysis)

#
# Performance tests
#
add_executable(perf_dense_map performance_tests/perf_dense_map.cpp)
target_link_libraries(perf_dense_map splash-${API_VERSION})
add_custom_command(OUTPUT run_perf_dense_map COMMAND ./perf_dense_map DEPENDS perf_dense_map)

if (HAVE_SHMDATA)
    add_executable(perf_shmdata performance_tests/perf_shmdata.cpp)
    target_link_libraries(perf_shmdata splash-${API_VERSION})
    add_custom_command(OUTPUT run_perf_shmdata COMMAND ./perf_shmdata DEPENDS perf_shmdata)
endif()

add_executable(perf_zmq_inproc performance_tests/perf_zmq_inproc.cpp)
target_link_libraries(perf_zmq_inproc splash-${API_VERSION})
add_custom_command(OUTPUT run_perf_zmq_inproc COMMAND ./perf_zmq_inproc DEPENDS perf_zmq_inproc)

add_custom_target(check_perf DEPENDS
    run_perf_dense_map
    run_perf_shmdata
    run_perf_zmq_inproc
)
