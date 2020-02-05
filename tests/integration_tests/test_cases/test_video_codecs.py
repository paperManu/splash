import splash

from splash_test import SplashTestCase
from time import sleep


class TestVideoCodecs(SplashTestCase):
    def test_codecs(self):
        print("Testing video codecs")

        splash.set_world_attribute("replaceObject", ["image", "image_ffmpeg", "image", "object"])
        sleep(2.0)
        splash.set_object_attribute("image", "file", f"{SplashTestCase.test_dir}/../../assets/tornado_h264.mov")
        sleep(10.0)
        splash.set_object_attribute("image", "file", f"{SplashTestCase.test_dir}/../../assets/tornado_hap.mov")
        sleep(10.0)
        splash.set_object_attribute("image", "file", f"{SplashTestCase.test_dir}/../../assets/tornado_hap_alpha.mov")
        sleep(10.0)
        splash.set_object_attribute("image", "file", f"{SplashTestCase.test_dir}/../../assets/tornado_hap_q.mov")
        sleep(10.0)
