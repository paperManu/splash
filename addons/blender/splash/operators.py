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
from mathutils import Vector
from pyshmdata import Writer

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
