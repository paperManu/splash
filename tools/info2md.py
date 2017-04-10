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

    objectsDescription = dict()
    objectsAttributes = dict()

    for data in json:
        if type(json[data]) == str:
            objType = data[:data.find("_description")]
            objectsDescription[objType] = json[data]
        elif type(json[data]) == dict:
            attributes = "Attributes:\n"
            for attr in json[data]:
                if json[data][attr] == "":
                    continue
                attributes += "- " + attr + ": " + json[data][attr] + "\n"
            attributes += "\n"
            objectsAttributes[data] = attributes

    for data in json:
        if data.find("_description") != -1:
            continue
        print("### " + data)
        description = objectsDescription.get(data)
        if description is not None:
            print(description)
        print(objectsAttributes[data])
