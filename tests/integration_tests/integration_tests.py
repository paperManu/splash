# This should should be ran within Splash:
# splash -P integration_tests.py

import argparse
import os
import splash
import sys

from unittest import TestLoader, TestResult


TEST_CASES_PATH = f"{os.path.dirname(os.path.abspath(__file__))}/test_cases"


def splash_init() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--pattern', dest='pattern', type=str, default="test*.py")
    parser.add_argument('--list', dest='list', action='store_true', default=False)
    args = parser.parse_args()

    if args.list is True:
        files = [
            file for file in os.listdir(TEST_CASES_PATH)
            if os.path.isfile(os.path.join(TEST_CASES_PATH, file)) and file.startswith("test_")
        ]
        print("\n=========================")
        print("List of Python test files")
        for file in files:
            print(f"- {file}")
        print("\n")
    else:
        runTests(pattern=args.pattern)

    splash.set_world_attribute("quit", [])


def splash_loop() -> None:
    pass


def splash_stop() -> None:
    print("Press a key to quit")


def runTests(pattern: str) -> None:
    test_loader = TestLoader()
    tests = test_loader.discover(
        start_dir=TEST_CASES_PATH,
        pattern=pattern
    )

    test_results = TestResult()
    tests(result=test_results)
