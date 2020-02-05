# This should should be ran within Splash:
# splash -P integration_tests.py

import os
import splash

from unittest import TestLoader, TestResult


def splash_init():
    runTests()


def splash_loop():
    pass


def splash_stop():
    print("Press a key to quit")


def runTests():
    current_path = os.path.dirname(os.path.abspath(__file__))
    test_loader = TestLoader()
    tests = test_loader.discover(start_dir=f"{current_path}/test_cases")

    test_results = TestResult()
    tests(result=test_results)

    splash.set_world_attribute("quit", [])
