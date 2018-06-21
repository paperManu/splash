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
include_directories(../external/jsoncpp)
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

# Unit tests (executed through 'make check')
add_custom_command(OUTPUT update_assets
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/data
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/data/*.json ${CMAKE_CURRENT_BINARY_DIR}/data/
    )
add_custom_target(assets DEPENDS update_assets)

add_executable(unitTests unitTests.cpp)
target_sources(unitTests PRIVATE
    check_attributefunctor.cpp
    check_base_object.cpp
    check_resizablearray.cpp
    check_value.cpp
    check_upgrade_configuration.cpp
)
add_dependencies(unitTests assets)
target_link_libraries(unitTests splash-${API_VERSION})

add_test(NAME unitTests COMMAND unitTests)

add_custom_command(OUTPUT tests COMMAND unitTests)
add_custom_target(check DEPENDS update_assets tests)

# Integration tests (executed by launching Splash and checking its behavior)
add_custom_command(OUTPUT integration_tests
    COMMAND if [ ! -d ${CMAKE_CURRENT_SOURCE_DIR}/assets ]; then $(git clone https://gitlab.com/sat-metalab/splash-assets ${CMAKE_CURRENT_SOURCE_DIR}/assets); fi
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/assets && git pull origin master
    COMMAND ../src/splash ${CMAKE_CURRENT_SOURCE_DIR}/integrationTests.json
    DEPENDS splash
    )
add_custom_target(check_integration DEPENDS integration_tests)

#
# Static analysis through CppCheck
#
add_custom_command(OUTPUT cppcheck_analysis
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build_cppcheck.sh
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/cppcheck/cppcheck --enable=warning,performance,unusedFunction --project=${CMAKE_CURRENT_BINARY_DIR}/../compile_commands.json 2> ${CMAKE_CURRENT_BINARY_DIR}/static_analysis_result.txt
)
add_custom_target(static_analysis DEPENDS cppcheck_analysis)
