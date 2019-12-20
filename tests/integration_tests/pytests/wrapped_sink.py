import splash
import os
from time import sleep

description = "Test the wrapped sink"

def run():
    sink = splash.Sink()
    sink.set_size(8, 8)
    sink.set_framerate(15)
    sink.link_to("image")
    sink.link_to("image")
    sink.open()
    sleep(0.5)
    image = sink.grab()
    print("Sink linked, grabbed image:", image.hex())
    sink.close()
    sink.unlink()

    for i in range(10):
        sink.link_to("image")
        sink.open()
        image = sink.grab()
        print("Sink re-linked, grabbed image size:", len(image))
        sink.close()
        sink.unlink()

    other_sink = splash.Sink("image", 64, 64)
    other_sink.set_framerate(15)
    other_sink.open()
    image = other_sink.grab()
    print("Another sink, grabbed image size:", len(image))

    other_sink = splash.Sink(some_unknown_arg="oupsy")
