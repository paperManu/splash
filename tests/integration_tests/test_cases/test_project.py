import splash

from splash_test import SplashTestCase


class TestProjects(SplashTestCase):
    def test_project(self):
        print("Testing saving and loading a project")

        projectFilePath = "/tmp/splashProject.json"
        splash.set_world_attribute("saveProject", [projectFilePath])
        splash.set_world_attribute("loadProject", [projectFilePath])
