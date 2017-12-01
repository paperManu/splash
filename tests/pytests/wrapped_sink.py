import splash
import os
from time import sleep

description = "Test the wrapped sink"

def create_sink():
    sink = splash.Sink()
    sink.set_size(8, 8)
    sink.set_framerate(15)
    sink.link_to("image")
    sink.open()
    return sink

def destroy_sink(sink):
    sink.close()
    sink.unlink()

def run():
    sink = create_sink()
    sleep(0.5)
    image = sink.grab()
    print("Sink linked, grabbed image:", image.hex())
    sink.close()
    sink.unlink()
    sleep(0.5)
    sink.link_to("image")
    sink.open()
    sleep(0.5)
    image = sink.grab()
    print("Sink re-linked, grabbed image:", image.hex())
    destroy_sink(sink)

    other_sink = create_sink()
    sleep(0.5)
    image = other_sink.grab()
    print("Another sink, grabbed image:", image.hex())
    destroy_sink(other_sink)
