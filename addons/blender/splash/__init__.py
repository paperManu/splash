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

bl_info = {
    "name": "Splash output",
    "author": "Emmanuel Durand",
    "version": (0, 7, 17),
    "blender": (2, 72, 0),
    "location": "3D View > Toolbox, File > Export",
    "description": "Utility tools to connect Blender to the Splash videomapper",
    "wiki_url": "https://github.com/paperManu/splash",
    "category": "Object",
}

import sys
sys.path.insert(0, '/usr/local/lib/python3/dist-packages')


if "bpy" in locals():
    import imp
    imp.reload(ui)
    imp.reload(operators)
    imp.reload(nodes)
else:
    import bpy
    from bpy.props import (StringProperty,
                           BoolProperty,
                           EnumProperty,
                           IntProperty,
                           FloatProperty,
                           PointerProperty,
                           )
    from bpy.types import (Operator,
                           AddonPreferences,
                           PropertyGroup,
                           )
    from . import ui
    from . import operators
    from . import nodes

def getImageList(scene, context):
    images = []
    for image in bpy.data.images:
        images.append((image.name, image.name, ""))
    return images

def getSplashOutputList(scene, context):
    targets = []
    for name, obj in operators.Splash._targets.items():
        targets.append((name, name, ""))
    return targets

class SplashSettings(PropertyGroup):
    outputActive = BoolProperty(
        name="Splash output active",
        description="True if data is being sent to Splash",
        options={'SKIP_SAVE'},
        default=False
        )
    sendTexture = BoolProperty(
        name="Texture is sent if active",
        description="Set to true to send the texture along with the mesh",
        default=False
        )
    targetObject = StringProperty(
        name="Object name",
        description="Name of the object being sent",
        default="", maxlen=1024,
        )
    targetNames = EnumProperty(
        name="Targets",
        description="Current mesh outputs",
        items=getSplashOutputList
        )
    outputPathPrefix = StringProperty(
        name="Splash output path prefix",
        description="Path prefix to the shared memory where the selected object is sent",
        default="/tmp/blenderToSplash", maxlen=1024, subtype="FILE_PATH",
        )
    updatePeriodObject = FloatProperty(
        name="Update period in Object mode",
        description="Time between updates of the shared memory while in Object mode",
        subtype='TIME',
        default=0.5,
        min=0.01, max=1.0,
        )
    updatePeriodEdit = FloatProperty(
        name="Update period in Edit mode",
        description="Time between updates of the shared memory while in Edit mode",
        subtype='TIME',
        default=0.05,
        min=0.01, max=1.0,
        )
    textureName = EnumProperty(
        name="Images",
        description="Available images to send",
        items=getImageList
        )
    updatePeriodTexture = FloatProperty(
        name="Update period for the texture",
        description="Time between updates of the texture",
        subtype='TIME',
        default=1.0,
        min=0.1, max=100.0,
        )

classes = (
    ui.SplashToolbarObject,
    ui.SplashToolbarMesh,

    operators.SplashActivateSendMesh,
    operators.SplashSendTexture,
    operators.SplashStopSelected,

    nodes.SplashTree,
    nodes.SplashLinkSocket,
    nodes.SplashBaseNode,
    nodes.SplashCameraNode,
    nodes.SplashGuiNode,
    nodes.SplashImageNode,
    nodes.SplashMeshNode,
    nodes.SplashProbeNode,
    nodes.SplashObjectNode,
    nodes.SplashSceneNode,
    nodes.SplashWindowNode,
    nodes.SplashWorldNode,

    operators.SplashExportNodeTree,
    operators.SplashSelectFilePath,
    operators.SplashSelectCamera,
    operators.SplashSelectObject,

    SplashSettings
    )


def getTextureTypes(scene, context):
    items = [('image', 'Image', ""),
             ('image_ffmpeg', 'Video', ""),
             ('image_shmdata', 'Shmdata', ""),
             ('texture_syphon', 'Syphon', "")]
    return items

def getMeshTypes(scene, context):
    items = [('mesh', 'Mesh', ""),
             ('mesh_shmdata', 'Shmdata', "")]
    return items


def register():
    for cls in classes:
        bpy.utils.register_class(cls)

    bpy.types.Scene.splash = PointerProperty(type=SplashSettings)

    import nodeitems_utils
    nodeitems_utils.register_node_categories("SPLASH_NODES", nodes.node_categories)


def unregister():
    for cls in classes:
        bpy.utils.unregister_class(cls)

    del bpy.types.Scene.splash

    import nodeitems_utils
    nodeitems_utils.unregister_node_categories("SPLASH_NODES")
