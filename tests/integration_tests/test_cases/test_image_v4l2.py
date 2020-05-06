import splash

from splash_test import SplashTestCase
from time import sleep


class TestImageV4L2(SplashTestCase):
    def test_image_v4l2(self):
        print("Test V4L2 capture")

        splash.set_world_attribute("replaceObject", ["image", "image_v4l2", "image", "object"])
        splash.set_object_attribute("image", "doCapture", 1)
        sleep(1.0)
        splash.set_object_attribute("image", "captureSize", [2048, 2048])
        sleep(1.0)
        splash.set_object_attribute("image", "captureSize", [640, 480])
        sleep(1.0)
        splash.set_object_attribute("image", "doCapture", 0)
