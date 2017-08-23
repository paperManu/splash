
import splash
import os
from time import sleep

description = "Replace the Image object multiple times"

def run():
    splash.set_global("addObject", ["virtual_probe", "probe"])
    splash.set_global("sendAllScenes", ["unlink", "object", "camera"])
    splash.set_global("sendAllScenes", ["link", "object", "probe"])
    splash.set_global("sendAllScenes", ["link", "probe", "camera"])
