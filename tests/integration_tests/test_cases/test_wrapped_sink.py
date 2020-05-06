import splash

from splash_test import SplashTestCase
from time import sleep


class TestWrappedSink(SplashTestCase):
    def test_wrapped_sink(self):
        print("Test the wrapped sink")

        sink = splash.Sink()
        sink.link_to("image")
        sink.set_size(8, 8)
        sink.set_framerate(15)
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
