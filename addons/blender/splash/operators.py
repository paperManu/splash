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

class Splash:
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

    @staticmethod
    def sendMesh(scene):
        context = bpy.context
    
        currentTime = time.clock_gettime(time.CLOCK_REALTIME) - Splash._startTime
    
        if bpy.context.edit_object is not None:
            if currentTime - Splash._frameTimeMesh < Splash._updatePeriodEdit:
                return
            Splash._frameTimeMesh = currentTime
    
            mesh = bmesh.from_edit_mesh(Splash._object.data)
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
            Splash._meshWriter.push(buffer, floor(currentTime * 1e9))
        else:
            if currentTime - Splash._frameTimeMesh < Splash._updatePeriodObject:
                return
            Splash._frameTimeMesh = currentTime
    
            if type(Splash._object.data) is bpy.types.Mesh:
                # Look for UV coords, create them if needed
                if len(Splash._object.data.uv_layers) == 0:
                    bpy.ops.object.editmode_toggle()
                    bpy.ops.uv.smart_project()
                    bpy.ops.object.editmode_toggle()
    
                # Apply the modifiers to the object
                mesh = Splash._object.to_mesh(context.scene, True, 'PREVIEW')
    
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
                Splash._meshWriter.push(buffer, floor(currentTime * 1e9))
    

    @staticmethod
    def sendTexture(scene):
        if Splash._texture is None or Splash._texWriterPath is None:
            return
    
        if Splash._texWriter is None or Splash._texture.size[0] != Splash._texSize[0] or Splash._texture.size[1] != Splash._texSize[1]:
            try:
                os.remove(Splash._texWriterPath)
            except:
                pass
    
            if Splash._texWriter is not None:
                del Splash._texWriter
    
            Splash._texSize[0] = Splash._texture.size[0]
            Splash._texSize[1] = Splash._texture.size[1]
            Splash._texWriter = Writer(path=Splash._texWriterPath, datatype="video/x-raw-rgb, bpp=(int)24, endianness=(int)4321, depth=(int)24, red_mask=(int)16711680, green_mask=(int)65280, blue_mask=(int)255, width=(int){0}, height=(int){1}, framerate=(fraction)1/1".format(Splash._texSize[0], Splash._texSize[1]))
    
        buffer = bytearray()
        pixels = [int(pix * 255) for pix in Splash._texture.pixels]
        for pix in range(Splash._texture.size[0] * Splash._texture.size[1]):
            buffer += struct.pack("BBB", pixels[pix*4], pixels[pix*4+1], pixels[pix*4+2])
        currentTime = time.clock_gettime(time.CLOCK_REALTIME) - Splash._startTime
        Splash._texWriter.push(buffer, floor(currentTime * 1e9))

        # We send twice to pass through the Splash input buffer
        time.sleep(0.1)
        currentTime = time.clock_gettime(time.CLOCK_REALTIME) - Splash._startTime
        Splash._texWriter.push(buffer, floor(currentTime * 1e9)) 


class SplashActivateSendMesh(Operator):
    """Activate the mesh output to Splash"""
    bl_idname = "splash.activate_send_mesh"
    bl_label = "Activate mesh output to Splash"

    def execute(self, context):
        scene = bpy.context.scene
        splash = scene.splash

        Splash._updatePeriodObject = splash.updatePeriodObject
        Splash._updatePeriodEdit = splash.updatePeriodEdit
        
        # Texture shmdata writer is also handled here
        Splash._texWriterPath = splash.outputPathPrefix + "_texture"

        # Mesh sending stuff
        if Splash.sendMesh not in bpy.app.handlers.scene_update_post:
            Splash._startTime = time.clock_gettime(time.CLOCK_REALTIME)
            Splash._frameTimeMesh = 0
            Splash._frameTimeTex = 0

            context = bpy.context
            if bpy.context.edit_object is not None:
                Splash._object = context.edit_object
            else:
                Splash._object = context.active_object
            splash.targetObject = Splash._object.name

            path = splash.outputPathPrefix + "_mesh"
            try:
                os.remove(path)
            except:
                pass

            Splash._meshWriter = Writer(path=path, datatype="application/x-polymesh")

            bpy.app.handlers.scene_update_post.append(Splash.sendMesh)
            splash.outputActive = True

        else:
            bpy.app.handlers.scene_update_post.remove(Splash.sendMesh)
            del Splash._meshWriter
            Splash._meshWriter = None

            try:
                if Splash._texWriter is not None:
                    del Splash._texWriter
                    Splash._texWriter = None
            except AttributeError:
                pass

            splash.targetObject = ""
            splash.outputActive = False

        return {'FINISHED'}

class SplashSendTexture(Operator):
    """Send the texture to Splash"""
    bl_idname = "splash.send_texture"
    bl_label = "Send the texture to Splash"

    def execute(self, context):
        scene = bpy.context.scene
        splash = scene.splash

        Splash._updatePeriodTexture = splash.updatePeriodTexture
        Splash._texture = bpy.data.images[splash.textureName]

        Splash.sendTexture(scene)

        return {'FINISHED'}
