AM_CPPFLAGS = \
	-funroll-loops -ftree-vectorize \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/external/cppzmq \
	-I$(top_srcdir)/external/glm \
	-I$(top_srcdir)/external/hap/source \
	-I$(top_srcdir)/external/imgui \
	-I$(top_srcdir)/external/jsoncpp \
	-I$(top_srcdir)/external/libltc/src \
	-I$(top_srcdir)/external/piccante/include \
	-I$(top_srcdir)/external/stb \
	-DGLV_GLFW \
	-DGLM_FORCE_RADIANS \
	-DDATADIR=\"$(datadir)/splash/\" \
	-DSPLASHPREFIX=\"$(prefix)\"

if HAVE_OSX
AM_CPPFLAGS += \
	-I$(top_srcdir)/external/glad/compatibility/include
else
AM_CPPFLAGS += \
	-I$(top_srcdir)/external/glad/core/include
endif

if HAVE_GPHOTO
bin_PROGRAMS = \
	splash-check-calibration

splash_check_calibration_SOURCES = splash-check-calibration.cpp

splash_check_calibration_LDFLAGS = \
	$(top_builddir)/src/libsplash-@LIBSPLASH_API_VERSION@.la

if HAVE_OSX
splash_check_calibration_LDFLAGS += \
	-F$(top_srcdir)/external/syphon/build/Release \
	-framework Syphon

splash_check_calibration_OBJCXXFLAGS = \
	-F$(top_srcdir)/external/syphon/build/Release \
	-framework Syphon
endif
endif # HAVE_GPHOTO
