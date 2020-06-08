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
import code
import threading

try:
    import readline
except ImportError:
    pass
else:
    import rlcompleter
    readline.parse_and_bind("tab: complete")
    readline.set_completer(rlcompleter.Completer(globals()).complete)

def repl():
    global console
    console = code.InteractiveConsole(locals=globals())
    console.interact()

replThread = threading.Thread(target=repl)

def splash_init() -> None:
    replThread.start()

def splash_loop() -> bool:
    return True

def splash_stop() -> None:
    print("Press a key to quit")
    console.push("quit()")
