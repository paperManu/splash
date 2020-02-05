import os
import splash

from splash_test import SplashTestCase


class TestSample(SplashTestCase):
    def test_nothing(self):
        print("This tests nothing")
        self.assertEqual(True, True)
