AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/external/GLV/GLV \
    -I$(top_srcdir)/external/bandit \
    $(JSONCPP_CFLAGS) \
    $(SHMDATA_CFLAGS) \
    -DGLV_GLFW

AM_LDFLAGS = \
    $(GLFW_LIBS) \
	$(GL_LIBS) \
    $(GLEW_LIBS) \
	$(GSL_LIBS) \
    $(JSONCPP_LIBS) \
    $(OPENIMAGEIO_LIBS) \
    $(OPENMESH_LIBS) \
    $(SHMDATA_LIBS) \
	$(top_srcdir)/src/libsplash-@LIBSPLASH_API_VERSION@.la

if HAVE_TESTS
check_PROGRAMS = \
    check_image \
	check_mesh \
    check_scene \
    check_object \
    check_world \
    check_gui

check_image_SOURCES = check_image.cpp

check_mesh_SOURCES = check_mesh.cpp

check_scene_SOURCES = check_scene.cpp

check_object_SOURCES = check_object.cpp

check_world_SOURCES = check_world.cpp

check_gui_SOURCES = check_gui.cpp

TESTS = $(check_PROGRAMS)
endif
