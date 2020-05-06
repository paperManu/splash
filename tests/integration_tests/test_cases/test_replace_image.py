import splash

from splash_test import SplashTestCase
from time import sleep


class TestReplaceImage(SplashTestCase):
    def test_replace_image(self):
        print("Replace the Image object multiple times")

        image_file = f"{SplashTestCase.test_dir}/../../../data/share/splash/color_map.png"
        video_file = f"{SplashTestCase.test_dir}/../../assets/tornado_h264.mov"
        for i in range(5):
            splash.set_world_attribute("replaceObject", ["image", "image", "image", "object"])
            sleep(0.5)
            splash.set_object_attribute("image", "file", image_file)
            sleep(2.0)
            splash.set_world_attribute("replaceObject", ["image", "image_ffmpeg", "image", "object"])
            sleep(0.5)
            splash.set_object_attribute("image", "file", video_file)
            sleep(2.0)
