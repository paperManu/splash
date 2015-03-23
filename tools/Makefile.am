AM_CPPFLAGS = \
	-funroll-loops -ftree-vectorize \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/external/GLV/GLV \
	-I$(top_srcdir)/external/hap/source \
	-I$(top_srcdir)/external/piccante/include \
	-DGLV_GLFW \
	-DGLM_FORCE_RADIANS \
	-DDATADIR=\"$(datadir)/splash/\" \
	-DSPLASHPREFIX=\"$(prefix)\"

bin_PROGRAMS = \
	splash-check-calibration

splash_check_calibration_SOURCES = splash-check-calibration.cpp

splash_check_calibration_LDFLAGS = \
	$(top_builddir)/src/libsplash-@LIBSPLASH_API_VERSION@.la
