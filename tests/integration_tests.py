# This script should be loaded inside a python object of Splash.
# To do so, add the following to one of a scene configuration:
#
# "python" : {
#   "type" : "python",
#   "file" : "./httpserver.py"
# }
#
# For this to work,  the configuration file should be in the same
# directory as this script. Otherwise, modify the path accordingly

import splash
from time import sleep

def splash_init():
    runTests()

def splash_loop():
    pass

def splash_stop():
    print("Press a key to quit")

def runTests():
    import imp
    import os

    import pytests # This is to get the path to the tests

    path = pytests.__path__._path[0]
    for name in os.listdir(path):
        if name.find(".py") == -1 or name.find(".py") != len(name) - 3:
            continue

        print("\n==========", name, "==========")
        print("Press a key to continue")
        input()

        filepath = path + "/" + name
        src = imp.load_source("data", path, open(filepath))

        if 'run' not in dir(src):
            print("File " + name + " has no run() method")

        if 'description' not in dir(src):
            print("No description available")
        else:
            print(src.description)

        try:
            src.run()
        except Exception as e:
            print("Exception caught: ", str(e))

        splash.set_world_attribute("loadProject",
                                   os.path.dirname(os.path.realpath(__file__))
                                   + "/integrationTests_project.json")

    splash.set_world_attribute("quit", [])
