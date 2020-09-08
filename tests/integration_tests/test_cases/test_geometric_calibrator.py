import splash

from splash_test import SplashTestCase
from time import sleep


class TestGeometricCalibrator(SplashTestCase):
    def test_geometric_calibrator(self):
        print("Testing geometric calibrator")

        # Link a "camera" to the geometric calibrator
        splash.set_world_attribute("addObject", ["image_list", "image_list"])
        splash.set_world_attribute("link", ["image_list", "geometricCalibrator"])
        sleep(1)

        splash.set_object_attribute("image_list", "file", f"{SplashTestCase.test_dir}/../../assets/captured_patterns/")

        # Start the calibration
        splash.set_object_attribute("geometricCalibrator", "calibrate", "true")
        sleep(1)
         # Position 1
        splash.set_object_attribute("geometricCalibrator", "nextPosition", "true")
        sleep(20)
        # Position 2
        splash.set_object_attribute("geometricCalibrator", "nextPosition", "true")
        sleep(20)
        # Position 3
        splash.set_object_attribute("geometricCalibrator", "nextPosition", "true")
        sleep(20)

        splash.set_object_attribute("geometricCalibrator", "finalizeCalibration", "true")
        # Wait for the calibration process to execute
        sleep(30)
