#!/usr/bin/env python3

# This script convert a json string received from
# 'splash --info' through pipe to Markdown

import json
import sys
from io import StringIO

#*************#
if __name__ == "__main__":
    jsonString = ""
    firstLine = True
    for line in sys.stdin:
        if firstLine:
            if line[0] != "{":
                continue
            firstLine = False
        jsonString += line
    
    io = StringIO(jsonString)
    json = json.load(io)

    for data in json:
        print("### " + data)
        print("Attributes:")
        for attr in json[data]:
            if json[data][attr] == "":
                continue
            print("- " + attr + ": " + json[data][attr])
        print("")
