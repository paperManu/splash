#!/bin/bash
cd ../external/syphon
xcodebuild -project Syphon.xcodeproj -configuration Release $@ SYMROOT=./build DSTROOT=/
