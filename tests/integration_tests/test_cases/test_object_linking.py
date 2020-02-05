import splash

from splash_test import SplashTestCase
from time import sleep


class TestObjectLinking(SplashTestCase):
    def setUp(self):
        print("Test linking / unlinking various objects")

    def test_link_window_image(self):
        print("---> Linking window and image")
        splash.set_world_attribute("unlink", ["cam1", "win1"])
        splash.set_world_attribute("link", ["image", "win1"])
        sleep(1.0)
        splash.set_world_attribute("unlink", ["image", "win1"])
        sleep(1.0)
        splash.set_world_attribute("link", ["image", "win1"])
        sleep(1.0)
        splash.set_world_attribute("unlink", ["image", "win1"])
        sleep(1.0)
        splash.set_world_attribute("link", ["cam1", "win1"])
        sleep(1.0)

    def test_link_object_image(self):
        print("---> Linking object and image")
        splash.set_world_attribute("unlink", ["image", "object"])
        sleep(1.0)
        splash.set_world_attribute("link", ["image", "object"])
        sleep(1.0)
        splash.set_world_attribute("unlink", ["image", "object"])
        sleep(1.0)
        splash.set_world_attribute("link", ["image", "object"])
        sleep(1.0)
