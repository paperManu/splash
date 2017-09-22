import splash

description = "Testing saving and loading a project"

def run():
    projectFilePath = "/tmp/splashProject.json"
    splash.set_world_attribute("saveProject", [projectFilePath])
    splash.set_world_attribute("loadProject", [projectFilePath])
