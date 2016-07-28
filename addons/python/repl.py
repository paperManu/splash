# This script should be loaded inside a python object of Splash.
# To do so, add the following to one of a scene configuration:
#
# "python" : {
#   "type" : "python",
#   "file" : "./httpServer.py"
# }
#
# For this to work,  the configuration file should be in the same
# directory as this script. Otherwise, modify the path accordingly

import splash
import code
import threading

def repl():
    import splash
    global console
    console = code.InteractiveConsole(locals=locals())
    console.interact()

replThread = threading.Thread(target=repl)

def splash_init():
    replThread.start()

def splash_loop():
    pass

def splash_stop():
    print("Press a key to quit")
    console.push("quit()")
    pass
