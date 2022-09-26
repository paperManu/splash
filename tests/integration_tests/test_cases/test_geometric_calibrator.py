import splash

from splash_test import SplashTestCase
from time import sleep


class TestGeometricCalibrator(SplashTestCase):
    def test_geometric_calibrator(self):
        print("Testing geometric calibrator")

        # Check that the GeometricCalibrator has been compiled
        if "geometricCalibrator" not in splash.get_object_list():
            print("Splash was not compiled with geometric calibration support.")
            return

        # Link a "camera" to the geometric calibrator
        print("-----------------")
        splash.set_world_attribute("addObject", ["image_list", "image_list"])
        print("-----------------")
        splash.set_world_attribute("link", ["image_list", "geometricCalibrator"])
        print("-----------------")
        sleep(1)

        splash.set_object_attribute("image_list", "file", f"{SplashTestCase.test_dir}/../assets/captured_patterns/")
        print("-----------------")

        # Start the calibration
        splash.set_object_attribute("geometricCalibrator", "calibrate", True)
        print("-----------------")
        sleep(1)
        # Position 1
        splash.set_object_attribute("geometricCalibrator", "nextPosition", True)
        print("-----------------")
        sleep(20)
        # Position 2
        splash.set_object_attribute("geometricCalibrator", "nextPosition", True)
        print("-----------------")
        sleep(20)
        # Position 3
        splash.set_object_attribute("geometricCalibrator", "nextPosition", True)
        print("-----------------")
        sleep(20)

        splash.set_object_attribute("geometricCalibrator", "finalizeCalibration", True)
        print("-----------------")
        # Wait for the calibration process to execute
        sleep(30)
