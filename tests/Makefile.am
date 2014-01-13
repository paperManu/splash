AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/external/GLV \
    -I$(top_srcdir)/external/bandit

AM_LDFLAGS = \
    $(GLFW_LIBS) \
	$(GL_LIBS) \
    $(JSONCPP_LIBS) \
    $(OPENIMAGEIO_LIBS) \
    $(OPENMESH_LIBS) \
	$(top_srcdir)/src/libsplash.la

check_PROGRAMS = \
    check_image \
	check_mesh \
    check_scene \
    check_object \
    check_world

check_image_SOURCES = check_image.cpp

check_mesh_SOURCES = check_mesh.cpp

check_scene_SOURCES = check_scene.cpp

check_object_SOURCES = check_object.cpp

check_world_SOURCES = check_world.cpp
check_world_CXXFLAGS = $(JSONCPP_CFLAGS)

TESTS = $(check_PROGRAMS)
