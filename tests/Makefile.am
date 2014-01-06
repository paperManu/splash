AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/external/GLV \
    -I$(top_srcdir)/external/bandit

AM_LDFLAGS = \
    $(GLFW_LIBS) \
	$(GL_LIBS) \
    $(OPENIMAGEIO_LIBS) \
    $(OPENMESH_LIBS) \
	$(top_srcdir)/src/libsplash.la

check_PROGRAMS = \
	check_mesh \
    check_scene

check_mesh_SOURCES = check_mesh.cpp
check_mesh_CXXFLAGS = $(OPENMESH_CFLAGS)

check_scene_SOURCES = check_scene.cpp

TESTS = $(check_PROGRAMS)
