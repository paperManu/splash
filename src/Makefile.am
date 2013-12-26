AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/external/GLV

bin_PROGRAMS = splash

splash_SOURCES = \
    splash.cpp \
    camera.cpp \
    geometry.cpp \
    image.cpp \
    mesh.cpp \
    object.cpp \
    scene.cpp \
    shader.cpp \
    texture.cpp \
    window.cpp \
    world.cpp

noinst_HEADERS = \
    $(top_srcdir)/include/splash.h \
    $(top_srcdir)/include/camera.h \
    $(top_srcdir)/include/geometry.h \
    $(top_srcdir)/include/image.h \
    $(top_srcdir)/include/mesh.h \
    $(top_srcdir)/include/object.h \
    $(top_srcdir)/include/scene.h \
    $(top_srcdir)/include/shader.h \
    $(top_srcdir)/include/texture.h \
    $(top_srcdir)/include/window.h \
    $(top_srcdir)/include/world.h

splash_CXXFLAGS = \
    $(GLFW_CFLAGS) \
	$(GL_CFLAGS) \
    $(GLM_CFLAGS) \
    $(OPENIMAGEIO_CFLAGS) \
    $(OPENMESH_CFLAGS)

splash_LDADD = \
    $(GLFW_LIBS) \
	$(GL_LIBS) \
    $(OPENIMAGEIO_LIBS) \
    $(OPENMESH_LIBS)
