import splash
from time import sleep

description = "Test V4L2 capture"

def run():
    splash.set_world_attribute("replaceObject", ["image", "image_v4l2", "image", "object"])
    splash.set_object_attribute("image", "doCapture", 1)
    sleep(1.0)
    splash.set_object_attribute("image", "captureSize", [2048, 2048])
    sleep(1.0)
    splash.set_object_attribute("image", "captureSize", [640, 480])
    sleep(1.0)
    splash.set_object_attribute("image", "doCapture", 0)
    splash.set_world_attribute("replaceObject", ["image", "image", "image", "object"])
