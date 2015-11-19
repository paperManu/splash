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
from bpy.props import (StringProperty,
                       BoolProperty,
                       IntProperty,
                       FloatProperty,
                       PointerProperty,
                       )
from math import floor
from mathutils import Vector, Matrix

class Target:
    _object = None
    _meshWriter = None
    _updatePeriodObject = 1.0
    _updatePeriodEdit = 1.0
    _startTime = 0.0
    _frameTimeMesh = 0.0

    _texture = None
    _texWriterPath = ""
    _texWriter = None
    _texSize = [0, 0]
    _updatePeriodTexture = 1.0
    _frameTimeTex = 0.0

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
                        n = loop.vert.normal
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
                            n = mesh.vertices[mesh.loops[idx].vertex_index].normal
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
                except:
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
        except:
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
            except:
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


def export_to_splash(self, context, filepath):
    scene = context.scene

    file = open(filepath, "w", encoding="utf8", newline="\n")
    fw = file.write

    # Header, not dependant of the objects in scene
    fw("// Splash configuration file\n"
       "// Exported with Blender Splash export\n"
       "{\n"
       "    \"encoding\" : \"UTF-8\",\n"
       "\n"
       "    \"world\" : {\n"
       "        \"framerate\" : 60\n"
       "    },\n"
       "\n"
       "    \"scenes\" : [\n"
       "        {\n"
       "            \"name\" : \"local\",\n"
       "            \"address\" : \"localhost\",\n"
       "            \"spawn\" : 1,\n"
       "            \"display\" : 0,\n"
       "            \"swapInterval\" : 1\n"
       "        }\n"
       "    ],\n"
       "\n"
       "    \"local\" : {\n")

    # Add a window for the GUI
    fw("        // Default window for the GUI\n"
       "        \"gui\" : {\n"
       "            \"type\" : \"window\",\n"
       "            \"fullscreen\" : -1,\n"
       "            \"decorated\" : 1,\n"
       "            \"position\" : [0, 0],\n"
       "            \"size\" : [732, 932],\n"
       "            \"srgb\" : [ 1 ]\n"
       "        },\n")
    
    links = []
    cameras = []
    windowIndex = 1

    # Export cameras
    for item in scene.objects.items():
        object = item[1]
        if object.type == 'CAMERA':
            # Get some parameters
            rotMatrix = object.matrix_world.to_3x3()
            targetVec = Vector((0.0, 0.0, -1.0))
            targetVec = rotMatrix * targetVec + object.matrix_world.to_translation()
            upVec = Vector((0.0, 1.0, 0.0))
            upVec = rotMatrix * upVec

            objectData = bpy.data.cameras[object.name]
            if objectData.splash_window_fullscreen is True:
                fullscreen = objectData.splash_fullscreen_index
            else:
                fullscreen = -1

            if objectData.splash_window_decoration is True:
                decoration = 1
            else:
                decoration = 0

            width = objectData.splash_width
            height = objectData.splash_height
            position_x = objectData.splash_position_x
            position_y = objectData.splash_position_y
            shift_x = 0.5 - objectData.shift_x * 2.0
            shift_y = 0.5 - objectData.shift_y * 2.0

            stringArgs = (object.name,
                          int(width), int(height),
                          float(object.matrix_world[0][3]), float(object.matrix_world[1][3]), float(object.matrix_world[2][3]),
                          float(targetVec[0]), float(targetVec[1]), float(targetVec[2]),
                          float(upVec[0]), float(upVec[1]), float(upVec[2]),
                          float(bpy.data.cameras[object.name].angle_y * 180.0 / numpy.pi),
                          float(shift_x), float(shift_y),
                          int(windowIndex),
                          int(fullscreen),
                          int(decoration),
                          int(width), int(height),
                          int(position_x), int(position_y))

            fw("        \"%s\" : {\n"
               "            \"type\" : \"camera\",\n"
               "            \"size\" : [%i, %i],\n"
               "            \"eye\" : [%f, %f, %f],\n"
               "            \"target\" : [%f, %f, %f],\n"
               "            \"up\" : [%f, %f, %f],\n"
               "            \"fov\" : [%f],\n"
               "            \"principalPoint\" : [%f, %f]\n"
               "        },\n"
               "        \"window_%i\" : {\n"
               "            \"type\" : \"window\",\n"
               "            \"fullscreen\" : %i,\n"
               "            \"decorated\" : %i,\n"
               "            \"size\" : [%i, %i],\n"
               "            \"position\" : [%i, %i],\n"
               "            \"srgb\" : [ 1 ]\n"
               "        },\n"
               "\n"
               % stringArgs)
            
            cameras.append(object.name)
            links.append([object.name, "window_%i" % int(windowIndex)])
            windowIndex += 1
        
    # Export meshes
    for item in scene.objects.items():
        object = item[1]
        if object.type == 'MESH':

            # Fill splash configuration
            objectData = bpy.data.meshes[object.name]

            if objectData.splash_mesh_type == "mesh" and objectData.splash_mesh_path == "":
                meshPath = "%s.obj" % object.name
                # Export the selected mesh
                path = os.path.dirname(filepath) + "/" + object.name + ".obj"
                bpy.ops.object.select_pattern(pattern=object.name, extend=False)
                bpy.ops.export_scene.obj(filepath=path, check_existing=False, use_selection=True, use_mesh_modifiers=True, use_materials=False,
                                         use_uvs=True, axis_forward='Y', axis_up='Z')
            else:
                meshPath = objectData.splash_mesh_path

            stringArgs = (object.name,
                          objectData.splash_mesh_type, meshPath,
                          "image_%s" % object.name, objectData.splash_texture_type, objectData.splash_texture_path,
                          "object_%s" % object.name,
                          object.scale[0], object.scale[1], object.scale[2],
                          object.matrix_world[0][3], object.matrix_world[1][3], object.matrix_world[2][3])

            fw("        \"%s\" : {\n"
               "            \"type\" : \"%s\",\n"
               "            \"file\" : \"%s\"\n"
               "        },\n"
               "        \"%s\" : {\n"
               "            \"type\" : \"%s\",\n"
               "            \"file\" : \"%s\",\n"
               "            \"flip\" : [ 0 ],\n"
               "            \"flop\" : [ 0 ],\n"
               "            \"srgb\" : [ 1 ]\n"
               "        },\n"
               "        \"%s\" : {\n"
               "            \"type\" : \"object\",\n"
               "            \"sideness\" : 0,\n"
               "            \"scale\" : [%f, %f, %f],\n"
               "            \"position\" : [%f, %f, %f]\n"
               "        },\n"
               "\n"
               % stringArgs)

            links.append([object.name, "object_%s" % object.name])
            links.append(["image_%s" % object.name, "object_%s" % object.name])
            for cam in cameras:
                links.append(["object_%s" % object.name, cam])


    # Export links
    fw("        \"links\" : [\n")
    for i in range(len(links)):
        link = links[i]
        linkArgs = (link[0], link[1])
        if i == len(links) - 1:
            fw("            [\"%s\", \"%s\"]\n" % linkArgs)
        else:
            fw("            [\"%s\", \"%s\"],\n" % linkArgs)
    fw("        ]\n")

    fw("    }\n"
       "}")
    file.close()

    return {'FINISHED'}


class SplashExportNodeTree(Operator):
    """Exports the Splash node tree from the calling node"""
    bl_idname = "splash.export_node_tree"
    bl_label = "Exports the node tree"

    filepath = bpy.props.StringProperty(subtype='FILE_PATH')

    node_name = StringProperty(name='Node name', description='Name of the calling node', default='')
    world_node = None
    scene_lists = {}
    node_links = {}

    def execute(self, context):
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
            self.parseTree(scene, scene_list, node_links)

            self.scene_lists[scene.name] = scene_list
            self.node_links[scene.name] = node_links

        return self.export()

    def parseTree(self, node, scene_list, node_links):
        scene_list[node.name] = node

        connectedNodes = [socket.links[0].from_node for socket in node.inputs if socket.is_linked]
        for connectedNode in connectedNodes:
            node_links.append([connectedNode.name, node.name])
            self.parseTree(connectedNode, scene_list, node_links)

    def export(self):
        file = open(self.filepath, "w", encoding="utf8", newline="\n")
        fw = file.write

        # World informations
        worldArgs =  self.world_node.exportProperties(self.filepath)
        fw("// Splash configuration file\n"
           "// Exported with Blender Splash add-on\n"
           "{\n"
           "    \"encoding\" : \"UTF-8\",\n"
           "\n"
           "    \"world\" : {\n"
           "        \"framerate\" : %i\n"
           "    },\n" % (worldArgs['framerate']))

        # Scenes list
        fw("    \"scenes\" : [\n")
        sceneIndex = 0
        for scene in self.scene_lists:
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
        for scene in self.scene_lists:
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

        fw("}")

        return {'FINISHED'}

    def invoke(self, context, event):
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
            self.current_node.inputs['Object'].enabled = False

        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class SplashSelectObject(Operator):
    """Select an Object"""
    bl_idname = "splash.select_object"
    bl_label = "Select an Object"

    filepath = bpy.props.StringProperty(subtype='FILE_PATH')

    node_name = StringProperty(name='Node name', description='Name of the calling node', default='')
    current_node = None

    def execute(self, context):
        for nodeTree in bpy.data.node_groups:
            nodeIndex = nodeTree.nodes.find(self.node_name)
            if nodeIndex != -1:
                self.current_node = nodeTree.nodes[nodeIndex]

        if self.current_node is not None and context.active_object is not None and isinstance(context.active_object.data, bpy.types.Mesh):
            self.current_node.inputs['File'].enabled = False
            self.current_node.inputs['Object'].default_value = context.active_object.data.name
            self.current_node.inputs['Object'].enabled = True

        return {'FINISHED'}


class SplashSelectCamera(Operator):
    """Select a Camera"""
    bl_idname = "splash.select_camera"
    bl_label = "Select a camera"

    filepath = bpy.props.StringProperty(subtype='FILE_PATH')

    node_name = StringProperty(name='Node name', description='Name of the calling node', default='')
    current_node = None

    def execute(self, context):
        for nodeTree in bpy.data.node_groups:
            nodeIndex = nodeTree.nodes.find(self.node_name)
            if nodeIndex != -1:
                self.current_node = nodeTree.nodes[nodeIndex]

        if self.current_node is not None and context.active_object is not None and isinstance(context.active_object.data, bpy.types.Camera):
            self.current_node.sp_objectProperty = context.active_object.name
            self.current_node.sp_cameraProperty = context.active_object.data.name

        return {'FINISHED'}
