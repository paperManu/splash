import splash
from time import sleep

description = "Test various supported video codecs"

def run():
    splash.set_world_attribute("replaceObject", ["image", "image_ffmpeg", "image", "object"])
    splash.set_object_attribute("image", "file", "./assets/tornado_h264.mov")
    sleep(10.0)
    splash.set_object_attribute("image", "file", "./assets/tornado_hap.mov")
    sleep(10.0)
    splash.set_object_attribute("image", "file", "./assets/tornado_hap_alpha.mov")
    sleep(10.0)
    splash.set_object_attribute("image", "file", "./assets/tornado_hap_q.mov")
    sleep(10.0)
    splash.set_world_attribute("replaceObject", ["image", "image", "image", "object"])
