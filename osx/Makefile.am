if HAVE_OSX
install:
	cp $(prefix)/bin/splash $(prefix)/bin/splash-scene ./splash.app/Contents/MacOS/
	cp $(top_srcdir)/data/2d_marker.obj $(top_srcdir)/data/3d_marker.obj ./splash.app/Contents/Resources/
	cp Info.plist ./splash.app/Contents/Info.plist
	DYLD_LIBRARY_PATH=/usr/local/lib:/opt/local/lib:$(prefix)/lib:$DYLD_LIBRARY_PATH dylibbundler -cd -od -b \
		-x ./splash.app/Contents/MacOS/splash \
		-x ./splash.app/Contents/MacOS/splash-scene \
		-d ./splash.app/Contents/libs/

clean:
	rm -r splash.app/Contents/MacOS/splash \
		splash.app/Contents/MacOS/splash-scene \
		splash.app/Contents/Info.plist \
		splash.app/Contents/libs
endif
