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
# blobserver is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
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
    bl_region_type = 'TOOLS'

    @classmethod
    def poll(cls, context):
        obj = context.active_object
        return (obj and obj.type == 'MESH')

    def draw(self, context):
        layout = self.layout

        scene = context.scene
        obj = context.object
        splash = scene.splash

        row = layout.row()
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
    bl_category = "Video Mapping"
    bl_idname = "MESH_PT_splash_object"
    bl_context = "objectmode"

class SplashToolbarMesh(Panel, SplashToolbar):
    bl_category = "Video Mapping"
    bl_idname = "MESH_PT_splash_mesh"
    bl_context = "mesh_edit"


# Splash object panel
class SplashObjectPanel(Panel):
    """Displays a Splash panel in the Object properties window"""
    bl_label = "Splash parameters"
    bl_idname = "OBJECT_PT_Splash"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    @classmethod
    def poll(cls, context):
        obj = context.object
        return (obj and (obj.type == 'CAMERA' or obj.type == 'MESH'))

    def draw(self, context):
        import bpy

        layout = self.layout
        if context.object.type == 'CAMERA':
            object = bpy.data.cameras[context.object.name]
            
            row = layout.row()
            row.label("Window size:")
            row = layout.row()
            row.prop(object, "splash_width", text="Width")
            row.prop(object, "splash_height", text="Height")

            row = layout.row()
            row.prop(object, "splash_window_decoration", text="Window decoration")

            row = layout.row()
            row.label("Fullscreen:")
            row = layout.row()
            row.prop(object, "splash_window_fullscreen", text="Activated")
            row.prop(object, "splash_fullscreen_index", text="Screen")

        elif context.object.type == 'MESH':
            object = bpy.data.meshes[context.object.name]

            row = layout.row()
            row.label("Mesh:")
            row = layout.row()
            row.prop(object, "splash_mesh_type", text="")
            row.prop(object, "splash_mesh_path", text="")

            row = layout.row()
            row.label("Texture:")
            row = layout.row()
            row.prop(object, "splash_texture_type", text="")
            row.prop(object, "splash_texture_path", text="")


# Splash export
class SplashExport(Operator, ExportHelper):
    """Exports cameras and meshes to a Splash config file"""
    bl_idname = "splash.export"
    bl_label = "Export to Splash"

    filename_ext = ".json"
    filter_glob = StringProperty(
        default = "*.json",
        options={'HIDDEN'},
        )

    def execute(self, context):
        from . import operators

        filepath = self.filepath
        filepath = path.ensure_ext(filepath, self.filename_ext)

        return operators.export_to_splash(self, context, filepath)

def splash_menu_export(self, context):
    self.layout.operator(SplashExport.bl_idname, SplashExport.bl_label)
