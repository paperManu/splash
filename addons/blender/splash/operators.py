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


class Splash:
    _targets = {}

    @staticmethod
    def sendMesh(scene):
        context = bpy.context

        if bpy.context.edit_object is not None:
            currentObject = context.edit_object.name
        else:
            currentObject = context.active_object.name

        for name, target in Splash._targets.items():
            currentTime = time.clock_gettime(time.CLOCK_REALTIME) - target._startTime
            worldMatrix = target._object.matrix_world

            normalMatrix = worldMatrix.copy()
            normalMatrix.invert()
            normalMatrix.transpose()

            if currentObject == name and bpy.context.edit_object is not None:
                if currentTime - target._frameTimeMesh < target._updatePeriodEdit:
                    continue
                target._frameTimeMesh = currentTime

                mesh = bmesh.from_edit_mesh(target._object.data)
                bufferVert = bytearray()
                bufferPoly = bytearray()
                buffer = bytearray()

                vertNbr = 0
                polyNbr = 0

                uv_layer = mesh.loops.layers.uv.active
                if uv_layer is None:
                    bpy.ops.uv.smart_project()
                    uv_layer = mesh.loops.layers.uv.active

                for face in mesh.faces:
                    polyNbr += 1
                    bufferPoly += struct.pack("i", len(face.verts))
                    for loop in face.loops:
                        bufferPoly += struct.pack("i", vertNbr)

                        v = loop.vert.co
                        tmpVector = Vector((v[0], v[1], v[2], 1.0))
                        tmpVector = worldMatrix * tmpVector
                        v = Vector((tmpVector[0], tmpVector[1], tmpVector[2]))

                        n = loop.vert.normal
                        tmpVector = Vector((n[0], n[1], n[2], 0.0))
                        tmpVector = normalMatrix * tmpVector
                        n = Vector((tmpVector[0], tmpVector[1], tmpVector[2]))

                        if uv_layer is None:
                            uv = Vector((0, 0))
                        else:
                            uv = loop[uv_layer].uv
                        bufferVert += struct.pack("ffffffff", v[0], v[1], v[2], uv[0], uv[1], n[0], n[1], n[2])
                        vertNbr += 1

                buffer += struct.pack("ii", vertNbr, polyNbr)
                buffer += bufferVert
                buffer += bufferPoly
                target._meshWriter.push(buffer, floor(currentTime * 1e9))
            else:
                if currentTime - target._frameTimeMesh < target._updatePeriodObject:
                    continue
                target._frameTimeMesh = currentTime

                if type(target._object.data) is bpy.types.Mesh:

                    # Look for UV coords, create them if needed
                    if len(target._object.data.uv_layers) == 0:
                        bpy.ops.object.editmode_toggle()
                        bpy.ops.uv.smart_project()
                        bpy.ops.object.editmode_toggle()

                    # Apply the modifiers to the object
                    mesh = target._object.to_mesh(context.scene, True, 'PREVIEW')

                    bufferVert = bytearray()
                    bufferPoly = bytearray()
                    buffer = bytearray()

                    vertNbr = 0
                    polyNbr = 0

                    for poly in mesh.polygons:
                        polyNbr += 1
                        bufferPoly += struct.pack("i", len(poly.loop_indices))
                        for idx in poly.loop_indices:
                            bufferPoly += struct.pack("i", vertNbr)

                            v = mesh.vertices[mesh.loops[idx].vertex_index].co
                            tmpVector = Vector((v[0], v[1], v[2], 1.0))
                            tmpVector = worldMatrix * tmpVector
                            v = Vector((tmpVector[0], tmpVector[1], tmpVector[2]))

                            n = mesh.vertices[mesh.loops[idx].vertex_index].normal
                            tmpVector = Vector((n[0], n[1], n[2], 0.0))
                            tmpVector = normalMatrix * tmpVector
                            n = Vector((tmpVector[0], tmpVector[1], tmpVector[2]))

                            if len(mesh.uv_layers) != 0:
                                uv = mesh.uv_layers[0].data[idx].uv
                            else:
                                uv = Vector((0, 0))
                            bufferVert += struct.pack("ffffffff", v[0], v[1], v[2], uv[0], uv[1], n[0], n[1], n[2])
                            vertNbr += 1

                    buffer += struct.pack("ii", vertNbr, polyNbr)
                    buffer += bufferVert
                    buffer += bufferPoly
                    target._meshWriter.push(buffer, floor(currentTime * 1e9))

                    bpy.data.meshes.remove(mesh)

    @staticmethod
    def sendTexture(scene):
        for name, target in Splash._targets.items():
            if target._texture is None or target._texWriterPath is None:
                return

            buffer = bytearray()
            pixels = [pix for pix in target._texture.pixels]
            pixels = numpy.array(pixels)
            pixels = (pixels * 255.0).astype(numpy.ubyte)
            buffer += pixels.tostring()
            currentTime = time.clock_gettime(time.CLOCK_REALTIME) - target._startTime

            if target._texWriter is None or target._texture.size[0] != target._texSize[0] or target._texture.size[1] != target._texSize[1]:
                try:
                    from pyshmdata import Writer
                    os.remove(target._texWriterPath)
                except os.error as e:
                    pass

                if target._texWriter is not None:
                    del target._texWriter

                target._texSize[0] = target._texture.size[0]
                target._texSize[1] = target._texture.size[1]
                target._texWriter = Writer(path=target._texWriterPath, datatype="video/x-raw, format=(string)RGBA, width=(int){0}, height=(int){1}, framerate=(fraction)1/1".format(target._texSize[0], target._texSize[1]), framesize=len(buffer))
            target._texWriter.push(buffer, floor(currentTime * 1e9))

            # We send twice to pass through the Splash input buffer
            time.sleep(0.1)
            currentTime = time.clock_gettime(time.CLOCK_REALTIME) - target._startTime
            target._texWriter.push(buffer, floor(currentTime * 1e9))


class SplashActivateSendMesh(Operator):
    """Activate the mesh output to Splash"""
    bl_idname = "splash.activate_send_mesh"
    bl_label = "Activate mesh output to Splash"

    def execute(self, context):
        try:
            from pyshmdata import Writer
        except os.error as e:
            print("Module pyshmdata was not found")
            return {'FINISHED'}

        scene = bpy.context.scene
        splash = scene.splash

        splashTarget = Target()
        splashTarget._updatePeriodObject = splash.updatePeriodObject
        splashTarget._updatePeriodEdit = splash.updatePeriodEdit

        context = bpy.context
        if bpy.context.edit_object is not None:
            splashTarget._object = context.edit_object
        else:
            splashTarget._object = context.active_object

        # Mesh sending stuff
        if splashTarget._object.name not in Splash._targets:
            splashTarget._startTime = time.clock_gettime(time.CLOCK_REALTIME)
            splashTarget._frameTimeMesh = 0
            splashTarget._frameTimeTex = 0

            splash.targetObject = splashTarget._object.name

            path = splash.outputPathPrefix + "_mesh_" + splash.targetObject
            try:
                os.remove(path)
            except os.error as e:
                pass

            splashTarget._meshWriter = Writer(path=path, datatype="application/x-polymesh")

            Splash._targets[splashTarget._object.name] = splashTarget

            if Splash.sendMesh not in bpy.app.handlers.scene_update_post:
                bpy.app.handlers.scene_update_post.append(Splash.sendMesh)

            splash.outputActive = True

        return {'FINISHED'}


class SplashSendTexture(Operator):
    """Send the texture to Splash"""
    bl_idname = "splash.send_texture"
    bl_label = "Send the texture to Splash"

    def execute(self, context):
        scene = bpy.context.scene
        splash = scene.splash

        targetName = splash.targetNames

        if targetName not in Splash._targets:
            return {'FINISHED'}

        Splash._targets[targetName]._texWriterPath = splash.outputPathPrefix + "_texture_" + splash.targetObject

        textureName = splash.textureName
        if textureName == "":
            return {'FINISHED'}

        Splash._targets[targetName]._updatePeriodTexture = splash.updatePeriodTexture
        Splash._targets[targetName]._texture = bpy.data.images[textureName]

        Splash.sendTexture(scene)

        return {'FINISHED'}


class SplashStopSelected(Operator):
    """Stop output for mesh selected in dropdown"""
    bl_idname = "splash.stop_selected_mesh"
    bl_label = "Stop output for mesh selected in the dropdown menu"

    def execute(self, context):
        scene = bpy.context.scene
        splash = scene.splash

        selection = splash.targetNames
        if selection in Splash._targets:
            Splash._targets.pop(selection)

        return {'FINISHED'}


class SplashExportNodeTree(Operator):
    """Exports Splash whole configuration, project (only geometry and image data), or 3D models"""
    bl_idname = "splash.export_node_tree"
    bl_label = "Exports the node tree"

    filepath = bpy.props.StringProperty(subtype='FILE_PATH')
    filter_glob = bpy.props.StringProperty(default="*.json", options={'HIDDEN'})

    node_name = StringProperty(name='Node name', description='Name of the calling node', default='')
    export_project = BoolProperty(name='export_project', description='If True, the tree will contain only meshes and images data', default=False)
    export_only_nodes = BoolProperty(name='export_only_nodes', description='If True, the tree is not exported, but nodes are evaluated anyway', default=False)

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

    filepath = bpy.props.StringProperty(subtype='FILE_PATH')

    node_name = StringProperty(name='Node name', description='Name of the calling node', default='')
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

    node_name = StringProperty(name='Node name', description='Name of the calling node', default='')
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

    node_name = StringProperty(name='Node name', description='Name of the calling node', default='')
    current_node = None

    def execute(self, context):
        for nodeTree in bpy.data.node_groups:
            nodeIndex = nodeTree.nodes.find(self.node_name)
            if nodeIndex != -1:
                self.current_node = nodeTree.nodes[nodeIndex]

        if self.current_node is not None and context.active_object is not None and isinstance(context.active_object.data, bpy.types.Camera):
            self.current_node.sp_objectProperty = context.active_object.name

        return {'FINISHED'}
