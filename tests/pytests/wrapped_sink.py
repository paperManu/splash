import splash
import os
from time import sleep

description = "Test the wrapped sink"

def run():
    sink = splash.Sink("image")
    sink.set_size(8, 8)
    sink.set_framerate(15)
    sink.open()
    sleep(0.5)
    image = sink.grab()
    print("Grabbed image:", image)
    sink.close()
