import splash
import os
from time import sleep

description = "Replace the Image object multiple times"

def run():
    for i in range(5):
        filename = os.path.dirname(os.path.abspath(__file__)) + "/../data/color_map.png"
        splash.set_global("replaceObject", ["image", "image", "object"])
        splash.set_object("image", "file", filename)
        sleep(1.0)
        splash.set_global("replaceObject", ["image", "image_ffmpeg", "object"])
        sleep(1.0)
