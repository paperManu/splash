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

if(APPLE)
    set(PACKAGE "${PROJECT_NAME}")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in" "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
    
    set(DYLD_LIBRARY_PATH "/usr/local/lib:/opt/local/lib:$(prefix)/lib:${DYLD_LIBRARY_PATH}")
    add_custom_command(
        OUTPUT splashApp
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/splash.app ./splash.app
        COMMAND cp ../src/splash ../src/splash-scene ./splash.app/Contents/MacOS/
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/../data/2d_marker.obj ${CMAKE_CURRENT_SOURCE_DIR}/../data/3d_marker.obj ./splash.app/Contents/Resources/
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/../data/templates.txt ./splash.app/Contents/Resources/
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../data/fonts ./splash.app/Contents/Resources/fonts
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../data/templates ./splash.app/Contents/Resources/templates
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist ./splash.app/Contents/Info.plist
        COMMAND mkdir -p ./splash.app/Contents/Frameworks
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../external/syphon/build/Release/Syphon.framework ./splash.app/Contents/Frameworks/Syphon.framework
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../external/macdylibbundler/dylibbundler -cd -od -b -x ./splash.app/Contents/MacOS/splash -x ./splash.app/Contents/MacOS/splash-scene -d ./splash.app/Contents/libs/
    )
    
    add_custom_target(package_osx DEPENDS macdylibbundler splashApp)
endif()
