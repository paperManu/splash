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
import bmesh
import struct
import time
import os
import numpy

from bpy.types import Operator
from bpy.props import StringProperty, BoolProperty
from math import floor
from mathutils import Vector


class Target:
    def __init__(self):
        self._object = None
        self._meshWriter = None
        self._updatePeriodObject = 1.0
        self._updatePeriodEdit = 1.0
        self._startTime = 0.0
        self._frameTimeMesh = 0.0

        self._texture = None
        self._texWriterPath = ""
        self._texWriter = None
        self._texSize = [0, 0]
        self._updatePeriodTexture = 1.0
        self._frameTimeTex = 0.0


class SplashExportNodeTree(Operator):
    """Exports Splash whole configuration, project (only geometry and image data), or 3D models"""
    bl_idname = "splash.export_node_tree"
    bl_label = "Exports the node tree"

    filepath : bpy.props.StringProperty(subtype='FILE_PATH')
    filter_glob : bpy.props.StringProperty(default="*.json", options={'HIDDEN'})

    node_name : StringProperty(name='Node name', description='Name of the calling node', default='')
    export_project : BoolProperty(name='export_project', description='If True, the tree will contain only meshes and images data', default=False)
    export_only_nodes : BoolProperty(name='export_only_nodes', description='If True, the tree is not exported, but nodes are evaluated anyway', default=False)

    world_node = None
    scene_order = []
    scene_lists = {}
    node_links = {}
    project_accepted_types = ['SplashImageNodeType',
                              'SplashMeshNodeType',
                              'SplashObjectNodeType']

    def execute(self, context):
        objectIsEdited = bpy.context.edit_object is not None
        if objectIsEdited is True:
            bpy.ops.object.editmode_toggle()

        self.scene_order.clear()
        self.scene_lists.clear()
        self.node_links.clear()

        for nodeTree in bpy.data.node_groups:
            nodeIndex = nodeTree.nodes.find(self.node_name)
            if nodeIndex != -1:
                node = nodeTree.nodes[nodeIndex]
                break

        self.world_node = node

        connectedScenes = [socket.links[0].from_node for socket in node.inputs if socket.is_linked]
        for scene in connectedScenes:
            scene_list = {}
            node_links = []
            tree_valid, tree_error = self.parseTree(scene, scene_list, node_links, self.export_project)
            if not tree_valid:
                message = "Splash tree exporting error: " + tree_error
                print(message)
                bpy.context.window_manager.popup_menu(
                    lambda self, context: self.layout.label(text=tree_error),
                    title="Splash export error",
                    icon='INFO'
                )
                return {'CANCELLED'}
            print(tree_error)

            self.scene_order.append(scene.name)
            self.scene_lists[scene.name] = scene_list
            self.node_links[scene.name] = node_links

        # If we only wanted to export meshes, the previous loop did the job
        if self.export_only_nodes is True:
            for scene in self.scene_lists:
                for node in self.scene_lists[scene]:
                    self.scene_lists[scene][node].exportProperties(self.filepath)
            return {'FINISHED'}

        # Merge scenes info if exporting a project
        if self.export_project and len(self.scene_order) > 0:
            masterSceneName = self.scene_order[0]
            for sceneId in range(1, len(self.scene_order)):
                sceneName = self.scene_order[sceneId]

                for node in self.scene_lists[sceneName]:
                    if node not in self.scene_lists[masterSceneName]:
                        self.scene_lists[masterSceneName][node] = self.scene_lists[sceneName][node]
                for link in self.node_links[sceneName]:
                    self.node_links[masterSceneName].append(link)

            self.scene_order = [self.scene_order[0]]
            self.scene_lists = {masterSceneName: self.scene_lists[masterSceneName]}

        self.export(self.export_project)
        return {'FINISHED'}

    def parseTree(self, node, scene_list, node_links, export_project=False):
        node_valid, node_error = node.validate()
        if not node_valid:
            return node_valid, node_error

        if not export_project or node.bl_idname in self.project_accepted_types:
            scene_list[node.name] = node

        connectedNodes = [socket.links[0].from_node for socket in node.inputs if socket.is_linked]
        for connectedNode in connectedNodes:
            newLink = [connectedNode.name, node.name]
            if newLink not in node_links:
                node_links.append([connectedNode.name, node.name])
            node_valid, node_error = self.parseTree(connectedNode, scene_list, node_links, export_project)
            if not node_valid:
                return node_valid, node_error

        return True, "Splash tree parsing successful"

    def export(self, export_project=False):
        file = open(self.filepath, "w", encoding="utf8", newline="\n")
        fw = file.write

        worldArgs = self.world_node.exportProperties(self.filepath)
        fw("// Splash configuration file\n"
           "// Exported with Blender Splash add-on\n"
           "{\n")

        if export_project:
            fw("    \"description\" : \"splashProject\",\n")
        else:
            fw("    \"description\" : \"splashConfiguration\",\n")
            # World informations
            fw("    \"world\" : {\n"
               "        \"framerate\" : %i\n"
               "    },\n" % (worldArgs['framerate']))

            # Scenes list
            fw("    \"scenes\" : [\n")
            sceneIndex = 0
            for scene in self.scene_order:
                # Find the Scene nodes
                for node in self.scene_lists[scene]:
                    if self.scene_lists[scene][node].bl_idname == "SplashSceneNodeType":
                        args = self.scene_lists[scene][node].exportProperties(self.filepath)
                        fw("        {\n")
                        valueIndex = 0
                        for values in args:
                            fw("            \"%s\" : %s" % (values, args[values]))

                            if valueIndex < len(args) - 1:
                                fw(",\n")
                            else:
                                fw("\n")
                            valueIndex = valueIndex + 1
                        fw("        }")

                        if sceneIndex < len(self.scene_lists) - 1:
                            fw(",\n")
                        else:
                            fw("\n")
                        sceneIndex = sceneIndex + 1
            fw("    ],\n")

        # Scenes information
        sceneIndex = 0
        for scene in self.scene_order:
            if not export_project:
                fw("    \"%s\" : {\n" % scene)

            for node in self.scene_lists[scene]:
                if self.scene_lists[scene][node].bl_idname != "SplashSceneNodeType":
                    args = self.scene_lists[scene][node].exportProperties(self.filepath)
                    fw("        \"%s\" : {\n" % node)
                    valueIndex = 0
                    for values in args:
                        fw("            \"%s\" : %s" % (values, args[values]))

                        if valueIndex < len(args) - 1:
                            fw(",\n")
                        else:
                            fw("\n")
                        valueIndex = valueIndex + 1
                    fw("        },\n")

            # Links
            fw("        \"links\" : [\n")
            linkIndex = 0
            for link in self.node_links[scene]:
                fw("            [\"%s\", \"%s\"]" % (link[0], link[1]))
                if linkIndex < len(self.node_links[scene]) - 1:
                    fw(",\n")
                else:
                    fw("\n")
                linkIndex = linkIndex + 1
            fw("        ]\n")

            if sceneIndex < len(self.scene_lists) - 1:
                fw("    },\n")
            else:
                fw("    }\n")
            sceneIndex = sceneIndex + 1

        if not export_project:
            fw("}")

    def invoke(self, context, event):
        self.filepath = os.path.splitext(bpy.data.filepath)[0] + ".json"
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class SplashSelectFilePath(Operator):
    """Select a file path"""
    bl_idname = "splash.select_file_path"
    bl_label = "Select a file path"

    filepath : bpy.props.StringProperty(subtype='FILE_PATH')

    node_name : StringProperty(name='Node name', description='Name of the calling node', default='')
    current_node = None

    def execute(self, context):
        for nodeTree in bpy.data.node_groups:
            nodeIndex = nodeTree.nodes.find(self.node_name)
            if nodeIndex != -1:
                self.current_node = nodeTree.nodes[nodeIndex]

        if self.current_node is not None:
            self.current_node.inputs['File'].default_value = self.filepath
            self.current_node.inputs['File'].enabled = True
            self.current_node.inputs['Object'].default_value = ""
            self.current_node.inputs['Object'].enabled = False

        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class SplashSelectObject(Operator):
    """Select an Object"""
    bl_idname = "splash.select_object"
    bl_label = "Select an Object"

    node_name : StringProperty(name='Node name', description='Name of the calling node', default='')
    current_node = None

    def execute(self, context):
        for nodeTree in bpy.data.node_groups:
            nodeIndex = nodeTree.nodes.find(self.node_name)
            if nodeIndex != -1:
                self.current_node = nodeTree.nodes[nodeIndex]

        if self.current_node is not None and context.active_object is not None and (isinstance(context.active_object.data, bpy.types.Mesh) or context.active_object.data is None):
            file_input = self.current_node.inputs.get('File')
            if file_input:
                file_input.default_value = ""
                file_input.enabled = False
            object_input = self.current_node.inputs.get('Object')
            if object_input:
                object_input.default_value = context.active_object.name
                object_input.enabled = True

        return {'FINISHED'}


class SplashSelectCamera(Operator):
    """Select a Camera"""
    bl_idname = "splash.select_camera"
    bl_label = "Select a camera"

    node_name : StringProperty(name='Node name', description='Name of the calling node', default='')
    current_node = None

    def execute(self, context):
        for nodeTree in bpy.data.node_groups:
            nodeIndex = nodeTree.nodes.find(self.node_name)
            if nodeIndex != -1:
                self.current_node = nodeTree.nodes[nodeIndex]

        if self.current_node is not None and context.active_object is not None and isinstance(context.active_object.data, bpy.types.Camera):
            self.current_node.sp_objectProperty = context.active_object.name

        return {'FINISHED'}
