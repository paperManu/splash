
import splash
import os
from time import sleep

description = "Replace the Image object multiple times"

def run():
    splash.set_world_attribute("addObject", ["virtual_probe", "probe"])
    splash.set_world_attribute("sendAllScenes", ["unlink", "object", "camera"])
    splash.set_world_attribute("sendAllScenes", ["link", "object", "probe"])
    splash.set_world_attribute("sendAllScenes", ["link", "probe", "camera"])
