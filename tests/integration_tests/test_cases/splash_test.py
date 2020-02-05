import os
import splash

from time import sleep
from unittest import TestCase


class SplashTestCase(TestCase):
    test_dir = os.path.dirname(os.path.realpath(__file__))

    @classmethod
    def setUpClass(cls):
        print("\n================")

    @classmethod
    def tearDownClass(cls):
        """
        Reload the default configuration
        """
        splash.set_world_attribute(
            "loadProject",
            f"{SplashTestCase.test_dir}/../integrationTests_project.json"
        )
        sleep(2.0)
