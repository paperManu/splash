#
# Copyright (C) 2015 Emmanuel Durand
#
# This file is part of Splash (http://github.com/paperManu/splash)
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

from bpy.types import Panel
from bpy_extras.io_utils import ExportHelper
from bpy.props import (StringProperty,
                       IntProperty,
                       PointerProperty
                       )
from bpy.types import (Operator,
                       PropertyGroup,
                       )
from bpy import path

import imp
if "operators" in locals():
    imp.reload(operators)


class SplashToolbar:
    bl_label = "Splash"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'

    @classmethod
    def poll(cls, context):
        obj = context.active_object
        return (obj and obj.type == 'MESH')

    def draw(self, context):
        layout = self.layout

        scene = context.scene
        splash = scene.splash

        col = layout.column()
        rowsub = col.row()
        colsub = rowsub.column()
        colsub.operator("splash.activate_send_mesh", text="Output selected mesh")
        rowsub = col.row(align=True)
        rowsub.prop(splash, "targetNames", text="")
        rowsub = col.row(align=True)
        rowsub.operator("splash.stop_selected_mesh", text="Stop selection")
        col = layout.column()
        col.label("Update periods:")
        col = layout.column()
        col.prop(splash, "updatePeriodObject", text="Object mode")
        col.prop(splash, "updatePeriodEdit", text="Edit mode")
        rowsub = col.row(align=True)
        rowsub.label("Mesh output path:")
        rowsub = col.row()
        rowsub.prop(splash, "outputPathPrefix", text="")
        col = layout.column()
        col.label("Texture:")
        col.prop(splash, "textureName", text="")
        col.operator("splash.send_texture", text="Send texture for selected mesh")


# Panel is available in object and editmode
class SplashToolbarObject(Panel, SplashToolbar):
    bl_category = "Splash"
    bl_idname = "MESH_PT_splash_object"
    bl_context = "objectmode"


class SplashToolbarMesh(Panel, SplashToolbar):
    bl_category = "Splash"
    bl_idname = "MESH_PT_splash_mesh"
    bl_context = "mesh_edit"
