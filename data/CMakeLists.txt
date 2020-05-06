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
    OUTPUT update_version
    COMMAND sed -i 's/Version=.*/Version=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}/g' ${CMAKE_CURRENT_SOURCE_DIR}/share/applications/splash.desktop
)

add_custom_target(splash-launcher ALL DEPENDS update_version)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES share/icons/hicolor/512x512/apps/splash.png DESTINATION "share/splash/")
