if HAVE_OSX
install:
	cp $(prefix)/bin/splash $(prefix)/bin/splash-scene ./splash.app/Contents/MacOS/
	cp Info.plist ./splash.app/Contents/Info.plist
	DYLD_LIBRARY_PATH=$(prefix)/lib:DYLD_LIBRARY_PATH dylibbundler -cd -od -b \
		-x ./splash.app/Contents/MacOS/splash \
		-x ./splash.app/Contents/MacOS/splash-scene \
		-d ./splash.app/Contents/libs/
endif
