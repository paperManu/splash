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

add_custom_command(
    OUTPUT splash.desktop
    COMMAND sed -i 's/Version=.*/Version=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}/g' ${CMAKE_CURRENT_SOURCE_DIR}/share/applications/splash.desktop
    COMMAND sed -i 's!Icon=.*!Icon=${CMAKE_INSTALL_PREFIX}/share/icons/splash.png!g' ${CMAKE_CURRENT_SOURCE_DIR}/share/applications/splash.desktop
)

if(UNIX AND NOT APPLE)
    add_custom_target(splash-launcher ALL DEPENDS splash.desktop)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

install(FILES 
    3d_marker.obj
    2d_marker.obj
    camera.obj
    color_map.png
    cubes.obj
    probe.obj
    splash.json
    share/icons/hicolor/512x512/apps/splash.png
    DESTINATION "share/splash/")
