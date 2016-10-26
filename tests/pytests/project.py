import splash

description = "Testing saving and loading a project"

def run():
    projectFilePath = "/tmp/splashProject.json"
    splash.set_global("saveProject", [projectFilePath])
    splash.set_global("loadProject", [projectFilePath])
