AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/external/GLV

bin_PROGRAMS = splash

splash_SOURCES = \
    splash.cpp

noinst_HEADERS = \
    $(top_srcdir)/include/splash.h

splash_CXXFLAGS = \
    $(GLFW_CFLAGS) \
	$(GL_CFLAGS) \
    $(OPENIMAGEIO_CFLAGS) \
    $(OPENMESH_CFLAGS)

splash_LDADD = \
    $(GLFW_LIBS) \
	$(GL_LIBS) \
    $(OPENIMAGEIO_LIBS) \
    $(OPENMESH_LIBS)
