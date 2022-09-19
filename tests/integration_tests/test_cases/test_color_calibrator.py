import splash

from splash_test import SplashTestCase
from time import sleep


class TestColorCalibrator(SplashTestCase):
    def test_color_calibrator(self):
        print("Testing color calibrator")

        # Check that the GeometricCalibrator has been compiled
        if "colorCalibrator" not in splash.get_object_list():
            print("Splash was not compiled with color calibration support.")
            return

        # Add a camera to scene
        splash.set_world_attribute("addObject", ["camera", "cam2"])
        splash.set_world_attribute("link", ["object", "cam2"])
        splash.set_world_attribute("link", ["cam2", "window_1"])
        sleep(1)        

        # Link a "camera" to the color calibrator
        splash.set_world_attribute("addObject", ["image_list", "image_list"])
        splash.set_object_attribute("colorCalibrator","colorSamples",3)
        splash.set_world_attribute("link", ["image_list", "colorCalibrator"])
        sleep(1)
        splash.set_object_attribute("image_list", "file", f"{SplashTestCase.test_dir}/../../assets/color_calibration/")
        sleep(1)

        # Calibrate camera response
        print("---------------- Compute Camera Response Function")
        splash.set_world_attribute("sendToMasterScene","calibrateColorResponseFunction")
        sleep(10)
        print("---------------- Calibrate displays")
        print("White, red, green then blue should be displayed on each side on the display window")
        sleep(2)
        # Calibrate displays/projectors
        splash.set_world_attribute("sendToMasterScene","calibrateColor")
        sleep(10)

        print("---------------- Color Calibration Applied")
        print("You can check that the applied calibration makes sense")
        sleep(10)
