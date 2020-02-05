import splash

from splash_test import SplashTestCase


class TestVirtualProbe(SplashTestCase):
    def test_virtual_probe(self):
        print("Test VirtualProbe object")

        splash.set_world_attribute("addObject", ["virtual_probe", "probe"])
        splash.set_world_attribute("sendAllScenes", ["unlink", "object", "camera"])
        splash.set_world_attribute("sendAllScenes", ["link", "object", "probe"])
        splash.set_world_attribute("sendAllScenes", ["link", "probe", "camera"])
