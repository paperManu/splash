#!/usr/bin/env python3

# This script convert a json string received from
# 'splash --info' through pipe to Markdown

import json
import sys
from io import StringIO

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

    objects_descriptions = dict()
    objects_short_descriptions = dict()
    objects_attributes = dict()

    for data in json:
        if type(json[data]) == str:
            if data.find("_short_description") != -1:
                objType = data[:data.find("_short_description")]
                objects_short_descriptions[objType] = json[data]
            elif data.find("_description") != -1:
                objType = data[:data.find("_description")]
                objects_descriptions[objType] = json[data]
        elif type(json[data]) == dict:
            attributes = "Attributes:\n\n"
            for attr in json[data]:
                if json[data][attr] == "":
                    continue
                attributes += "- " + attr + ": " + json[data][attr] + "\n"
            attributes += "\n"
            objects_attributes[data] = attributes

    for data in json:
        if data.find("_description") != -1 or data.find("_short_description") != -1:
            continue
        description = objects_descriptions.get(data)
        short_description = objects_short_descriptions.get(data)
        print("#### {} - {}\n".format(data, short_description))
        if description is not None:
            print("{}\n".format(description))
        print(objects_attributes[data])
