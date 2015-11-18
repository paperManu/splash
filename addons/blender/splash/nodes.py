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


import bpy
from bpy.types import NodeTree, Node, NodeSocket

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

    def updateSockets(self):
        linksInput = [socket for socket in self.inputs if socket.bl_idname == 'SplashLinkSocket']
        links = [socket for socket in linksInput if socket.is_linked is True]

        for socket in links:
            for link in socket.links:
                if link.from_node.bl_idname not in self.sp_acceptedLinks:
                    self.inputs.remove(socket)

        if len(links) < len(linksInput) - 1:
            for socket in linksInput:
                if not socket.is_linked:
                    self.inputs.remove(socket)
                    break

        if len(linksInput) == len(links):
            self.inputs.new('SplashLinkSocket', "Input link")

    def socketsToDict(self):
        pass


class SplashCameraNode(SplashBaseNode):
    '''Splash Camera node'''
    bl_idname = 'SplashCameraNodeType'
    bl_label = 'Camera'

    sp_acceptedLinks = [
        'SplashObjectNodeType',
        ]

    def draw_buttons(self, context, layout):
        layout.prop(self, "name")

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

    def socketsToDict(self):
        values = {}
        values['blackLevel'] = self.inputs['Black level'].default_value
        values['blendWidth'] = self.inputs['Blending width'].default_value
        values['blendPrecision'] = self.inputs['Blending precision'].default_value
        values['brightness'] = self.inputs['Brightness'].default_value
        values['colorTemperature'] = self.inputs['Color temperature'].default_value
        values['size'] = [self.inputs['Width'].default_value, self.inputs['Height'].default_value]

        return values

    def update(self):
        self.updateSockets()


class SplashImageNode(SplashBaseNode):
    '''Splash Image node'''
    bl_idname = 'SplashImageNodeType'
    bl_label = 'Image'

    sp_acceptedLinks = []

    def draw_buttons(self, context, layout):
        layout.prop(self, "name")

    def init(self, context):
        self.inputs.new('NodeSocketString', 'File').default_value = ""
        self.inputs.new('NodeSocketBool', 'Flip').default_value = False
        self.inputs.new('NodeSocketBool', 'Flop').default_value = False
        self.inputs.new('NodeSocketBool', 'sRGB').default_value = True

        self.outputs.new('SplashLinkSocket', "Output link")

    def socketsToDict(self):
        values = {}
        values['file'] = "\"" + self.inputs['File'].default_value + "\""
        values['flip'] = int(self.inputs['Flip'].default_value)
        values['flip'] = int(self.inputs['Flop'].default_value)
        values['srgb'] = int(self.inputs['sRGB'].default_value)

        return values

    def update(self):
        pass


class SplashMeshNode(SplashBaseNode):
    '''Splash Mesh node'''
    bl_idname = 'SplashMeshNodeType'
    bl_label = 'Mesh'

    sp_acceptedLinks = []

    def draw_buttons(self, context, layout):
        layout.prop(self, "name")

    def init(self, context):
        self.inputs.new('NodeSocketString', 'File').default_value = ""

        self.outputs.new('SplashLinkSocket', "Output link")

    def socketsToDict(self):
        values = {}
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
        self.inputs.new('NodeSocketInt', 'Side').default_value = 0

        self.inputs.new('SplashLinkSocket', "Input link")
        self.outputs.new('SplashLinkSocket', "Output link")

    def socketsToDict(self):
        values = {}
        values['color'] = [self.inputs['Color'].default_value[0],
                           self.inputs['Color'].default_value[1],
                           self.inputs['Color'].default_value[2]]
        values['position'] = [self.inputs['Position'].default_value[0],
                              self.inputs['Position'].default_value[1],
                              self.inputs['Position'].default_value[2]]
        values['scale'] = [self.inputs['Scale'].default_value[0],
                           self.inputs['Scale'].default_value[1],
                           self.inputs['Scale'].default_value[2]]
        values['side'] = self.inputs['Side'].default_value

        return values

    def update(self):
        self.updateSockets()


class SplashSceneNode(SplashBaseNode):
    '''Splash Scene node'''
    bl_idname = 'SplashSceneNodeType'
    bl_label = 'Scene'

    sp_acceptedLinks = [
        'SplashWindowNodeType',
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

    def socketsToDict(self):
        values = {}
        values['address'] = self.inputs['Address'].default_value
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

    def socketsToDict(self):
        values = {}
        values['decorated'] = int(self.inputs['Decorated'].default_value)
        if self.inputs['Fullscreen'].default_value:
            values['fullscreen'] = self.inputs['Screen'].default_value
        else:
            values['fullscreen'] = -1
        values['position'] = [self.inputs['Position'].default_value[0], self.inputs['Position'].default_value[1]]
        values['size'] = [self.inputs['Width'].default_value, self.inputs['Height'].default_value]
        values['srgb'] = int(self.inputs['sRGB'].default_value)

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

    def socketsToDict(self):
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
        NodeItem("SplashImageNodeType"),
        NodeItem("SplashMeshNodeType"),
        NodeItem("SplashObjectNodeType"),
        NodeItem("SplashSceneNodeType"),
        NodeItem("SplashWindowNodeType"),
        NodeItem("SplashWorldNodeType"),
        ]),
    ]
