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


import numpy
import bpy
from bpy.types import NodeTree, Node, NodeSocket
from math import floor
from mathutils import Vector, Matrix

import imp
if "operators" in locals():
    imp.reload(operators)


class SplashTree(NodeTree):
    '''Tree used to feed the Splash video mapping software'''
    bl_idname = 'SplashTreeType'
    bl_label = 'Splash Tree'
    bl_icon = 'NODETREE'


class SplashLinkSocket(NodeSocket):
    '''Socket representing Splash links'''
    bl_idname = 'SplashLinkSocket'
    bl_label = 'Splash Link Socket'

    def draw(self, context, layout, node, text):
        if not self.is_output and self.is_linked:
            layout.label(self.links[0].from_node.name)
        else:
            layout.label(text)

    def draw_color(self, context, node):
        return (1.0, 0.5, 0.0, 1.0)


class SplashTreeNode:
    sp_nodeID = 0

    @classmethod
    def pool(cls, ntree):
        return ntree.bl_idname == 'SplashTreeType'

    @classmethod
    def getNewID(cls):
        newID = SplashTreeNode.sp_nodeID
        SplashTreeNode.sp_nodeID = SplashTreeNode.sp_nodeID + 1
        return newID


class SplashBaseNode(Node, SplashTreeNode):
    '''Base Splash node'''
    bl_idname = 'SplashBaseNodeType'
    bl_label = 'Base Node'
    bl_icon = 'SOUND'

    sp_acceptedLinks = []

    def draw_label(self):
        return self.name + " (" + self.bl_label + ")"

    def updateSockets(self):
        linksInput = [socket for socket in self.inputs if socket.bl_idname == 'SplashLinkSocket']
        links = [socket for socket in linksInput if socket.is_linked is True]

        for socket in links:
            for link in socket.links:
                if link.from_node.bl_idname not in self.sp_acceptedLinks:
                    self.inputs.remove(socket)

        for socket in [socket for socket in self.outputs if socket.bl_idname == 'SplashLinkSocket' and socket.is_linked is True]:
            for link in socket.links:
                if not isinstance(link.to_socket, SplashLinkSocket):
                    self.outputs.remove(self.outputs["Output link"])
                    self.outputs.new("SplashLinkSocket", "Output link")

        if len(links) < len(linksInput) - 1:
            for socket in linksInput:
                if not socket.is_linked:
                    self.inputs.remove(socket)
                    break

        if len(linksInput) == len(links):
            self.inputs.new('SplashLinkSocket', "Input link")

    def exportProperties(self, exportPath):
        pass


class SplashCameraNode(SplashBaseNode):
    '''Splash Camera node'''
    bl_idname = 'SplashCameraNodeType'
    bl_label = 'Camera'

    sp_acceptedLinks = [
        'SplashObjectNodeType',
        ]

    sp_objectProperty = bpy.props.StringProperty(name="Source object",
                                         description="Object holding the camera",
                                         default="",
                                         maxlen=1024)

    def draw_buttons(self, context, layout):
        layout.prop(self, "name")
        row = layout.row()
        operator = row.operator("splash.select_camera", text="Select the active Camera")
        operator.node_name = self.name
        row = layout.row()
        row.prop(self, "sp_objectProperty")

    def init(self, context):
        self.inputs.new('NodeSocketFloat', 'Black level').default_value = 0.0
        self.inputs.new('NodeSocketFloat', 'Blending width').default_value = 0.1
        self.inputs.new('NodeSocketFloat', 'Blending precision').default_value = 0.1
        self.inputs.new('NodeSocketFloat', 'Brightness').default_value = 1.0
        self.inputs.new('NodeSocketFloat', 'Color temperature').default_value = 6500.0
        self.inputs.new('NodeSocketInt', 'Width').default_value = 1920
        self.inputs.new('NodeSocketInt', 'Height').default_value = 1080

        self.inputs.new('SplashLinkSocket', "Input link")
        self.outputs.new('SplashLinkSocket', "Output link")

    def exportProperties(self, exportPath):
        values = {}
        values['type'] = "\"camera\""

        if bpy.data.objects.find(self.sp_objectProperty) != -1 and bpy.data.cameras.find(bpy.data.objects[self.sp_objectProperty].data.name) != -1:
            object = bpy.data.objects[self.sp_objectProperty]
            camera = bpy.data.objects[self.sp_objectProperty].data
            # Get some parameters
            rotMatrix = object.matrix_world.to_3x3()
            targetVec = Vector((0.0, 0.0, -1.0))
            targetVec = rotMatrix * targetVec + object.matrix_world.to_translation()
            upVec = Vector((0.0, 1.0, 0.0))
            upVec = rotMatrix * upVec

            values['eye'] = [float(object.matrix_world[0][3]), float(object.matrix_world[1][3]), float(object.matrix_world[2][3])]
            values['target'] = [targetVec[0], targetVec[1], targetVec[2]]
            values['up'] = [upVec[0], upVec[1], upVec[2]]
            values['fov'] = camera.angle_y * 180.0 / numpy.pi
            values['principalPoint'] = [0.5 - camera.shift_x * 2.0, 0.5 - camera.shift_y * 2.0]

        values['blackLevel'] = self.inputs['Black level'].default_value
        values['blendWidth'] = self.inputs['Blending width'].default_value
        values['blendPrecision'] = self.inputs['Blending precision'].default_value
        values['brightness'] = self.inputs['Brightness'].default_value
        values['colorTemperature'] = self.inputs['Color temperature'].default_value
        values['size'] = [self.inputs['Width'].default_value, self.inputs['Height'].default_value]

        return values

    def update(self):
        self.updateSockets()


class SplashGuiNode(SplashBaseNode):
    '''Splash Gui node'''
    bl_idname = 'SplashGuiNodeType'
    bl_label = 'Gui'
    name = 'Gui'

    sp_acceptedLinks = [
        ]

    def init(self, context):
        self.inputs.new('NodeSocketBool', 'Decorated').default_value = True
        self.inputs.new('NodeSocketVector', 'Position').default_value = [0.0, 0.0, 0.0]
        self.inputs.new('NodeSocketInt', 'Width').default_value = 732
        self.inputs.new('NodeSocketInt', 'Height').default_value = 932

        self.outputs.new('SplashLinkSocket', "Output link")

    def exportProperties(self, exportPath):
        values = {}
        values['type'] = "\"window\""
        values['decorated'] = int(self.inputs['Decorated'].default_value)
        values['position'] = [self.inputs['Position'].default_value[0], self.inputs['Position'].default_value[1]]
        values['size'] = [self.inputs['Width'].default_value, self.inputs['Height'].default_value]

        return values

    def update(self):
        pass


class SplashImageNode(SplashBaseNode):
    '''Splash Image node'''
    bl_idname = 'SplashImageNodeType'
    bl_label = 'Image'

    sp_acceptedLinks = []

    def update_image_type(self, context):
        if self.sp_imageTypeProperty == "image_ffmpeg":
            self.inputs['Clock'].enabled = True
        else:
            self.inputs['Clock'].enabled = False

        if self.sp_imageTypeProperty == "texture_syphon":
            self.inputs['Server name'].enabled = True
            self.inputs['Application name'].enabled = True
            self.inputs['File'].enabled = False
            self.inputs['Flip'].enabled = False
            self.inputs['Flop'].enabled = False
            self.inputs['sRGB'].enabled = False
        else:
            self.inputs['Server name'].enabled = False
            self.inputs['Application name'].enabled = False
            self.inputs['File'].enabled = True
            self.inputs['Flip'].enabled = True
            self.inputs['Flop'].enabled = True
            self.inputs['sRGB'].enabled = True
            

    sp_imageTypes = [
        ("image", "image", "Static image"),
        ("image_ffmpeg", "video", "Video file"),
        ("image_shmdata", "shared memory", "Video through shared memory"),
        ("texture_syphon", "syphon", "Video through Syphon (only on OSX)"),
    ]
    sp_imageTypeProperty = bpy.props.EnumProperty(name="Type",
                                                  description="Image source type",
                                                  items=sp_imageTypes,
                                                  default="image",
                                                  update=update_image_type)

    def draw_buttons(self, context, layout):
        row = layout.row()
        layout.prop(self, "name")
        row = layout.row()
        row.prop(self, "sp_imageTypeProperty")
        row = layout.row()
        operator = row.operator("splash.select_file_path", text="Select file path")
        operator.node_name = self.name

    def init(self, context):
        self.inputs.new('NodeSocketString', 'File').default_value = ""
        self.inputs.new('NodeSocketBool', 'Flip').default_value = False
        self.inputs.new('NodeSocketString', 'Object').default_value = ""
        self.inputs['Object'].enabled = False
        self.inputs.new('NodeSocketBool', 'Flop').default_value = False
        self.inputs.new('NodeSocketBool', 'sRGB').default_value = True
        self.inputs.new('NodeSocketBool', 'Clock').default_value = False
        self.inputs['Clock'].enabled = False
        self.inputs.new('NodeSocketString', 'Server name').default_value = ""
        self.inputs.new('NodeSocketString', 'Application name').default_value = ""
        self.inputs['Server name'].enabled = False
        self.inputs['Application name'].enabled = False

        self.outputs.new('SplashLinkSocket', "Output link")

    def exportProperties(self, exportPath):
        values = {}
        values['type'] = "\"" + self.sp_imageTypeProperty + "\""
        if self.sp_imageTypeProperty == "texture_syphon":
            values['servername'] = "\"" + self.inputs['Server name'].default_value + "\""
            values['appname'] = "\"" + self.inputs['Application name'].default_value + "\""
        else:
            values['file'] = "\"" + self.inputs['File'].default_value + "\""
            values['flip'] = int(self.inputs['Flip'].default_value)
            values['flop'] = int(self.inputs['Flop'].default_value)
            values['srgb'] = int(self.inputs['sRGB'].default_value)
            if self.sp_imageTypeProperty == "image_ffmpeg":
                values['clock'] = int(self.inputs['Clock'].default_value)

        return values

    def update(self):
        pass


class SplashMeshNode(SplashBaseNode):
    '''Splash Mesh node'''
    bl_idname = 'SplashMeshNodeType'
    bl_label = 'Mesh'

    sp_acceptedLinks = []

    def update_mesh_type(self, context):
        if self.sp_meshTypeProperty == "mesh_shmdata":
            self.inputs['File'].enabled = True
            self.inputs['Object'].enabled = False

    sp_meshTypes = [
        ("mesh", "OBJ file", "Mesh from OBJ file"),
        ("mesh_shmdata", "Shared memory", "Mesh from shared memory")
    ]
    sp_meshTypeProperty = bpy.props.EnumProperty(name="Type",
                                                  description="Mesh source type",
                                                  items=sp_meshTypes,
                                                  default="mesh",
                                                  update=update_mesh_type)

    def draw_buttons(self, context, layout):
        row = layout.row()
        layout.prop(self, "name")
        row = layout.row()
        row.prop(self, "sp_meshTypeProperty")
        row = layout.row()
        operator = row.operator("splash.select_file_path", text="Select file path")
        operator.node_name = self.name
        if self.sp_meshTypeProperty == "mesh":
            row = layout.row()
            operator = row.operator("splash.select_object", text="Select the active Object")
            operator.node_name = self.name

    def init(self, context):
        self.inputs.new('NodeSocketString', 'File').default_value = ""
        self.inputs.new('NodeSocketString', 'Object').default_value = ""
        self.inputs['Object'].enabled = False

        self.outputs.new('SplashLinkSocket', "Output link")

    def exportProperties(self, exportPath):
        values = {}
        values['type'] = "\"" + self.sp_meshTypeProperty + "\""

        if self.inputs['Object'].enabled:
            import os

            if bpy.context.edit_object is not None:
                editedObject = context.edit_object.name
                bpy.ops.object.editmode_toggle()

            objectName = self.inputs['Object'].default_value

            bpy.ops.object.select_all(action='DESELECT')
            bpy.data.objects[objectName].select = True
            path = os.path.dirname(exportPath) + "/splash_" + objectName + ".obj"
            bpy.ops.export_scene.obj(filepath=path, check_existing=False, use_selection=True, use_mesh_modifiers=True, use_materials=False,
                                     use_uvs=True, axis_forward='Y', axis_up='Z')

            values['file'] = "\"" + "splash_" + objectName + ".obj" + "\""
        else:
            values['file'] = "\"" + self.inputs['File'].default_value + "\""

        return values

    def update(self):
        pass


class SplashObjectNode(SplashBaseNode):
    '''Splash Object node'''
    bl_idname = 'SplashObjectNodeType'
    bl_label = 'Object'

    sp_acceptedLinks = [
        'SplashImageNodeType',
        'SplashMeshNodeType',
        ]

    def draw_buttons(self, context, layout):
        layout.prop(self, "name")

    def init(self, context):
        self.inputs.new('NodeSocketColor', 'Color').default_value = [1.0, 1.0, 1.0, 1.0]
        self.inputs.new('NodeSocketVector', 'Position').default_value = [0.0, 0.0, 0.0]
        self.inputs.new('NodeSocketVector', 'Scale').default_value = [1.0, 1.0, 1.0]
        self.inputs.new('NodeSocketInt', 'Sideness').default_value = 0

        self.inputs.new('SplashLinkSocket', "Input link")
        self.outputs.new('SplashLinkSocket', "Output link")

    def exportProperties(self, exportPath):
        values = {}
        values['type'] = "\"object\""
        values['color'] = [self.inputs['Color'].default_value[0],
                           self.inputs['Color'].default_value[1],
                           self.inputs['Color'].default_value[2],
                           self.inputs['Color'].default_value[3]]
        values['position'] = [self.inputs['Position'].default_value[0],
                              self.inputs['Position'].default_value[1],
                              self.inputs['Position'].default_value[2]]
        values['scale'] = [self.inputs['Scale'].default_value[0],
                           self.inputs['Scale'].default_value[1],
                           self.inputs['Scale'].default_value[2]]
        values['sideness'] = self.inputs['Side'].default_value

        return values

    def update(self):
        self.updateSockets()


class SplashSceneNode(SplashBaseNode):
    '''Splash Scene node'''
    bl_idname = 'SplashSceneNodeType'
    bl_label = 'Scene'

    sp_acceptedLinks = [
        'SplashWindowNodeType',
        'SplashGuiNodeType'
        ]

    def draw_buttons(self, context, layout):
        layout.prop(self, "name")

    def init(self, context):
        self.inputs.new('NodeSocketString', 'Address').default_value = 'localhost'
        self.inputs.new('NodeSocketInt', 'Blending resolution').default_value = 2048
        self.inputs.new('NodeSocketInt', 'Display').default_value = 0
        self.inputs.new('NodeSocketBool', 'Spawn').default_value = True
        self.inputs.new('NodeSocketInt', 'Swap interval').default_value = 1

        self.inputs.new('SplashLinkSocket', "Input link")
        self.outputs.new('SplashLinkSocket', "Output link")

    def exportProperties(self, exportPath):
        values = {}
        values['name'] = "\"" + self.name + "\""
        values['address'] = "\"" + self.inputs['Address'].default_value + "\""
        values['blendingResolution'] = self.inputs['Blending resolution'].default_value
        values['display'] = self.inputs['Display'].default_value
        values['spawn'] = int(self.inputs['Spawn'].default_value)
        values['swapInterval'] = self.inputs['Swap interval'].default_value

        return values

    def update(self):
        self.updateSockets()


class SplashWindowNode(SplashBaseNode):
    '''Splash Window node'''
    bl_idname = 'SplashWindowNodeType'
    bl_label = 'Window'

    sp_acceptedLinks = [
        'SplashCameraNodeType',
        'SplashImageNodeType',
        ]

    def draw_buttons(self, context, layout):
        layout.prop(self, "name")

    def init(self, context):
        self.inputs.new('NodeSocketBool', 'Decorated').default_value = True
        self.inputs.new('NodeSocketBool', 'Fullscreen').default_value = False
        self.inputs.new('NodeSocketInt', 'Screen').default_value = 0
        self.inputs.new('NodeSocketFloat', 'Gamma').default_value = 2.2
        self.inputs.new('NodeSocketVector', 'Position').default_value = [0.0, 0.0, 0.0]
        self.inputs.new('NodeSocketInt', 'Width').default_value = 1920
        self.inputs.new('NodeSocketInt', 'Height').default_value = 1080
        self.inputs.new('NodeSocketBool', 'sRGB').default_value = True

        self.inputs.new('SplashLinkSocket', "Input link")
        self.outputs.new('SplashLinkSocket', "Output link")

    def exportProperties(self, exportPath):
        values = {}
        values['type'] = "\"window\""
        values['decorated'] = int(self.inputs['Decorated'].default_value)
        if self.inputs['Fullscreen'].default_value:
            values['fullscreen'] = self.inputs['Screen'].default_value
        else:
            values['fullscreen'] = -1
        values['position'] = [self.inputs['Position'].default_value[0], self.inputs['Position'].default_value[1]]
        values['size'] = [self.inputs['Width'].default_value, self.inputs['Height'].default_value]
        values['srgb'] = int(self.inputs['sRGB'].default_value)

        inputCounts = 0
        for input in self.inputs:
            if input.name == "Input link" and input.is_linked:
                inputCounts += 1

        values['layout'] = []
        for i in range(0, inputCounts):
            values['layout'].append(i)

        return values

    def update(self):
        self.updateSockets()


class SplashWorldNode(SplashBaseNode):
    '''Splash World node'''
    bl_idname = 'SplashWorldNodeType'
    bl_label = 'World'

    sp_acceptedLinks = [
        'SplashSceneNodeType',
        ]

    def draw_buttons(self, context, layout):
        row = layout.row()
        row.prop(self, "name")
        row = layout.row()
        operator = row.operator("splash.export_node_tree", text="Export tree")
        operator.node_name = self.name

    def init(self, context):
        self.inputs.new('NodeSocketInt', 'Refresh rate').default_value = 60

        self.inputs.new('SplashLinkSocket', "Input link")

    def exportProperties(self, exportPath):
        values = {}
        values['framerate'] = self.inputs['Refresh rate'].default_value

        return values

    def update(self):
        self.updateSockets()


import nodeitems_utils
from nodeitems_utils import NodeCategory, NodeItem

class SplashNodeCategory(NodeCategory):
    @classmethod
    def poll(cls, context):
        return context.space_data.tree_type == "SplashTreeType"


node_categories = [
    SplashNodeCategory("SPLASHNODES", "Splash Nodes", items = [
        NodeItem("SplashCameraNodeType"),
        NodeItem("SplashGuiNodeType"),
        NodeItem("SplashImageNodeType"),
        NodeItem("SplashMeshNodeType"),
        NodeItem("SplashObjectNodeType"),
        NodeItem("SplashSceneNodeType"),
        NodeItem("SplashWindowNodeType"),
        NodeItem("SplashWorldNodeType"),
        ]),
    ]
