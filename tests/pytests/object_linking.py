import splash
from time import sleep

description = "Test linking / unlinking various objects"

def link_window_image():
    print("---> Linking window and image")
    splash.set_global("sendAllScenes", ["unlink", "cam1", "win1"])
    splash.set_global("sendAllScenes", ["link", "image", "win1"])
    sleep(1.0)
    splash.set_global("sendAllScenes", ["unlink", "image", "win1"])
    sleep(1.0)
    splash.set_global("sendAllScenes", ["link", "image", "win1"])
    sleep(1.0)
    splash.set_global("sendAllScenes", ["unlink", "image", "win1"])
    sleep(1.0)
    splash.set_global("sendAllScenes", ["link", "cam1", "win1"])
    sleep(1.0)

def link_object_image():
    print("---> Linking object and image")
    splash.set_global("sendAllScenes", ["unlink", "image", "object"])
    sleep(1.0)
    splash.set_global("sendAllScenes", ["link", "image", "object"])
    sleep(1.0)
    splash.set_global("sendAllScenes", ["unlink", "image", "object"])
    sleep(1.0)
    splash.set_global("sendAllScenes", ["link", "image", "object"])
    sleep(1.0)

def run():
    link_window_image()
    link_object_image()
