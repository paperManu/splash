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
    OUTPUT splash-launcher.desktop
    COMMAND sed -i 's/Version=.*/Version=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}/g' ${CMAKE_CURRENT_SOURCE_DIR}/config/splash-launcher.desktop
    COMMAND sed -i 's!Icon=.*!Icon=${CMAKE_INSTALL_PREFIX}/share/splash/splash-icon-512.png!g' ${CMAKE_CURRENT_SOURCE_DIR}/config/splash-launcher.desktop
)

if(UNIX AND NOT APPLE)
    add_custom_target(splash-launcher ALL DEPENDS splash-launcher.desktop)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/splash-launcher.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
endif()

install(FILES 
    3d_marker.obj
    2d_marker.obj
    camera.obj
    color_map.png
    cubes.obj
    plane.obj
    probe.obj
    sphere.obj
    splash.json
    splash-icon-512.png
    templates.txt
    DESTINATION "share/splash/")

install(FILES
    fonts/Roboto-Medium.ttf
    DESTINATION "share/splash/fonts")

install(FILES 
    templates/single_plane.json
    templates/single_plane.png
    templates/single_dome.json
    templates/single_dome.png
    templates/four_plane.json
    templates/four_plane.png
    templates/dual_dome.json
    templates/dual_dome.png
    templates/two_scenes_four_cameras.json
    templates/two_scenes_four_cameras.png
    DESTINATION "share/splash/templates")
