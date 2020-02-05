import splash

from splash_test import SplashTestCase
from time import sleep


class TestReplaceImage(SplashTestCase):
    def test_replace_image(self):
        print("Replace the Image object multiple times")

        for i in range(5):
            filename = f"{SplashTestCase.test_dir}/../../../data/share/splash/color_map.png"
            splash.set_world_attribute("replaceObject", ["image", "image", "image", "object"])
            splash.set_object_attribute("image", "file", filename)
            sleep(1.0)
            splash.set_world_attribute("replaceObject", ["image", "image_ffmpeg", "image", "object"])
            sleep(1.0)
            splash.set_world_attribute("replaceObject", ["image", "image", "image", "object"])
