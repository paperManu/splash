AM_CPPFLAGS = \
	-funroll-loops -ftree-vectorize \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/external/bandit \
	-I$(top_srcdir)/external/GLV/GLV \
	-I$(top_srcdir)/external/libsimdpp \
	-DGLV_GLFW \
	-DDATADIR=\"$(datadir)\splash\" \
	-DSPLASHPREFIX=\"$(prefix)\"

#*************#
lib_LTLIBRARIES = \
	libsplash-@LIBSPLASH_API_VERSION@.la

libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES = \
	camera.cpp \
	geometry.cpp \
	gui.cpp \
	image.cpp \
	image_shmdata.cpp \
	link.cpp \
	log.cpp \
	mesh.cpp \
	object.cpp \
	scene.cpp \
	shader.cpp \
	texture.cpp \
	threadpool.cpp \
	timer.cpp \
	window.cpp \
	widgets.cpp \
	world.cpp

# GLV related sources
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
	$(top_srcdir)/external/GLV/src/glv_buttons.cpp \
	$(top_srcdir)/external/GLV/src/glv_color_controls.cpp \
	$(top_srcdir)/external/GLV/src/glv_color.cpp \
	$(top_srcdir)/external/GLV/src/glv_core.cpp \
	$(top_srcdir)/external/GLV/src/glv_draw.cpp \
	$(top_srcdir)/external/GLV/src/glv_font.cpp \
	$(top_srcdir)/external/GLV/src/glv_glv.cpp \
	$(top_srcdir)/external/GLV/src/glv_grid.cpp \
	$(top_srcdir)/external/GLV/src/glv_inputdevice.cpp \
	$(top_srcdir)/external/GLV/src/glv_layout.cpp \
	$(top_srcdir)/external/GLV/src/glv_model.cpp \
	$(top_srcdir)/external/GLV/src/glv_notification.cpp \
	$(top_srcdir)/external/GLV/src/glv_plots.cpp \
	$(top_srcdir)/external/GLV/src/glv_preset_controls.cpp \
	$(top_srcdir)/external/GLV/src/glv_sliders.cpp \
	$(top_srcdir)/external/GLV/src/glv_sono.cpp \
	$(top_srcdir)/external/GLV/src/glv_texture.cpp \
	$(top_srcdir)/external/GLV/src/glv_textview.cpp \
	$(top_srcdir)/external/GLV/src/glv_view3D.cpp \
	$(top_srcdir)/external/GLV/src/glv_view.cpp \
	$(top_srcdir)/external/GLV/src/glv_widget.cpp

libsplash_@LIBSPLASH_API_VERSION@_la_CXXFLAGS = \
	$(GLFW_CFLAGS) \
	$(GL_CFLAGS) \
	$(GLM_CFLAGS) \
	$(GLEW_CFLAGS) \
	$(GSL_CFLAGS) \
	$(JSONCPP_CFLAGS) \
	$(OPENMESH_CFLAGS) \
	$(SHMDATA_CFLAGS) \
    $(SIMD_FLAGS) \
	$(SIMDPP_FLAGS) \
	$(ZMQ_CFLAGS)

libsplash_@LIBSPLASH_API_VERSION@_la_LDFLAGS = \
	-lOpenImageIO \
	$(GLFW_LIBS) \
	$(GL_LIBS) \
	$(GLEW_LIBS) \
	$(GSL_LIBS) \
	$(JSONCPP_LIBS) \
	$(OPENMESH_LIBS) \
	$(SHMDATA_LIBS) \
	$(ZMQ_LIBS)

#*************#
bin_PROGRAMS = splash \
	splash-scene

splash_DEPENDENCIES = libsplash-@LIBSPLASH_API_VERSION@.la

splash_SOURCES = \
	splash-app.cpp

splash_CXXFLAGS = \
	$(GLFW_CFLAGS) \
	$(GL_CFLAGS) \
	$(GLM_CFLAGS) \
	$(GSL_CFLAGS) \
	$(JSONCPP_CFLAGS) \
	$(OPENIMAGEIO_CFLAGS) \
	$(OPENMESH_CFLAGS) \
	$(SHMDATA_CFLAGS)

splash_LDFLAGS = \
	$(top_builddir)/src/libsplash-@LIBSPLASH_API_VERSION@.la

#*************#
splash_scene_DEPENDENCIES = libsplash-@LIBSPLASH_API_VERSION@.la

splash_scene_SOURCES = \
	splash-scene.cpp

splash_scene_CXXFLAGS = \
	$(GLFW_CFLAGS) \
	$(GL_CFLAGS) \
	$(GLM_CFLAGS) \
	$(GSL_CFLAGS) \
	$(JSONCPP_CFLAGS) \
	$(OPENIMAGEIO_CFLAGS) \
	$(OPENMESH_CFLAGS) \
	$(SHMDATA_CFLAGS)

splash_scene_LDFLAGS = \
	$(top_builddir)/src/libsplash-@LIBSPLASH_API_VERSION@.la
