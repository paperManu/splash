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

if (ZIP)
    set(UPDATE_BLENDER_VERSION sed -i 's/\"version\": .*/\"version\": \(${PROJECT_VERSION_MAJOR}, ${PROJECT_VERSION_MINOR}, ${PROJECT_VERSION_PATCH}\),/g' ${CMAKE_CURRENT_SOURCE_DIR}/splash/__init__.py)

    add_custom_command(
        OUTPUT blenderSplash.zip
        COMMAND ${UPDATE_BLENDER_VERSION}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/splash ./splash
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/templates ./templates
        COMMAND zip blenderSplash.zip -r splash/* -x *.swp -x *.pyc splash/Makefile*
        COMMAND ${CMAKE_COMMAND} -E remove -f templates/*.blend1
        COMMAND tar cvjf blender_splash_addon.tar.bz2 --exclude='Makefile*' --exclude='*.swp' blenderSplash.zip templates
        COMMAND ${CMAKE_COMMAND} -E remove_directory splash
        COMMAND ${CMAKE_COMMAND} -E remove_directory templates
    )

    add_custom_target(blenderSplash ALL DEPENDS blenderSplash.zip)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/blenderSplash.zip DESTINATION "share/splash")
endif()
