AM_CPPFLAGS = \
	-funroll-loops -ftree-vectorize \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/external/GLV/GLV \
	-I$(top_srcdir)/external/libsimdpp \
	-I$(top_srcdir)/external/hap/source \
	-DGLV_GLFW \
	-DGLM_FORCE_RADIANS \
	-DDATADIR=\"$(datadir)/splash/\" \
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
	mesh_shmdata.cpp \
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

# Hap related sources
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
	$(top_srcdir)/external/hap/source/hap.c

libsplash_@LIBSPLASH_API_VERSION@_la_CXXFLAGS = \
	-I$(top_srcdir)/external/GLV/GLV \
	-I$(top_srcdir)/external/libsimdpp/simdpp \
	-I$(top_srcdir)/external/bandit/bandit \
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
	-lsnappy \
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

#*************#
noinst_HEADERS = \
	$(top_srcdir)/include/camera.h \
	$(top_srcdir)/include/coretypes.h \
	$(top_srcdir)/include/geometry.h \
	$(top_srcdir)/include/gui.h \
	$(top_srcdir)/include/image.h \
	$(top_srcdir)/include/image_shmdata.h \
	$(top_srcdir)/include/link.h \
	$(top_srcdir)/include/log.h \
	$(top_srcdir)/include/mesh.h \
	$(top_srcdir)/include/mesh_shmdata.h \
	$(top_srcdir)/include/object.h \
	$(top_srcdir)/include/scene.h \
	$(top_srcdir)/include/shader.h \
	$(top_srcdir)/include/shaderSources.h \
	$(top_srcdir)/include/splash.h \
	$(top_srcdir)/include/texture.h \
	$(top_srcdir)/include/threadpool.h \
	$(top_srcdir)/include/timer.h \
	$(top_srcdir)/include/widgets.h \
	$(top_srcdir)/include/window.h \
	$(top_srcdir)/include/world.h \
	\
	$(top_srcdir)/external/libsimdpp/simdpp/null/bitwise.h \
	$(top_srcdir)/external/libsimdpp/simdpp/null/math.h \
	$(top_srcdir)/external/libsimdpp/simdpp/null/mask.h \
	$(top_srcdir)/external/libsimdpp/simdpp/null/foreach.h \
	$(top_srcdir)/external/libsimdpp/simdpp/null/shuffle.h \
	$(top_srcdir)/external/libsimdpp/simdpp/null/transpose.h \
	$(top_srcdir)/external/libsimdpp/simdpp/null/memory.h \
	$(top_srcdir)/external/libsimdpp/simdpp/null/set.h \
	$(top_srcdir)/external/libsimdpp/simdpp/null/compare.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd.h \
	$(top_srcdir)/external/libsimdpp/simdpp/dispatch/get_arch_linux_cpuinfo.h \
	$(top_srcdir)/external/libsimdpp/simdpp/dispatch/get_arch_gcc_builtin_cpu_supports.h \
	$(top_srcdir)/external/libsimdpp/simdpp/dispatch/dispatcher.h \
	$(top_srcdir)/external/libsimdpp/simdpp/dispatch/macros.h \
	$(top_srcdir)/external/libsimdpp/simdpp/dispatch/arch.h \
	$(top_srcdir)/external/libsimdpp/simdpp/altivec/load1.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/memory_load.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/shuffle_generic.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/float32x4.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/bitwise.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int8x32.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int32x4.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/math_fp_adv.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/make_shuffle_bytes_mask.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/math_fp_basic.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/float64x2.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/cast.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/math_int_adv.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int64x4.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/cache.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/word_size.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/mem_unpack.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/shuffle_emul.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/shuffle128.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/mem_pack.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/align.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/transpose.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/mem_block.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/not_implemented.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/width.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/detail/move_signed.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/shuffle.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int128.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/transpose.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/insert.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/extract.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int256.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/convert.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/shuffle_bytes.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/float32x8.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/math_int_basic.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/types.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int64x2.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int16x8.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int16x16.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/fwd.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/float64x4.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/memory_store.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/math_shift.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int32x8.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/traits.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/compare.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int8x16.h \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/this_compile_arch.h \
	$(top_srcdir)/external/libsimdpp/simdpp/neon/math_int.h \
	$(top_srcdir)/external/libsimdpp/simdpp/neon/detail/shuffle_int16x8.h \
	$(top_srcdir)/external/libsimdpp/simdpp/neon/detail/shuffle.h \
	$(top_srcdir)/external/libsimdpp/simdpp/neon/detail/shuffle_int64x2.h \
	$(top_srcdir)/external/libsimdpp/simdpp/neon/detail/math_shift.h \
	$(top_srcdir)/external/libsimdpp/simdpp/neon/detail/shuffle_int32x4.h \
	$(top_srcdir)/external/libsimdpp/simdpp/neon/shuffle.h \
	$(top_srcdir)/external/libsimdpp/simdpp/neon/memory_store.h \
	$(top_srcdir)/external/libsimdpp/simdpp/sse/math_fp.h \
	$(top_srcdir)/external/libsimdpp/simdpp/sse/memory_load.h \
	$(top_srcdir)/external/libsimdpp/simdpp/sse/math_int.h \
	$(top_srcdir)/external/libsimdpp/simdpp/sse/extract_half.h \
	$(top_srcdir)/external/libsimdpp/simdpp/sse/cache.h \
	$(top_srcdir)/external/libsimdpp/simdpp/sse/shuffle.h \
	$(top_srcdir)/external/libsimdpp/simdpp/sse/convert.h \
	$(top_srcdir)/external/libsimdpp/simdpp/sse/memory_store.h \
	$(top_srcdir)/external/libsimdpp/simdpp/sse/compare.h \
	$(top_srcdir)/external/libsimdpp/test/tests.h \
	$(top_srcdir)/external/libsimdpp/test/tests/dispatcher.h \
	$(top_srcdir)/external/libsimdpp/test/test_helpers.h \
	$(top_srcdir)/external/libsimdpp/test/test_case.h \
	$(top_srcdir)/external/libsimdpp/test/common/masks.h \
	$(top_srcdir)/external/libsimdpp/test/common/vectors.h \
	$(top_srcdir)/external/libsimdpp/test/common/aligned_allocator.h \
	$(top_srcdir)/external/libsimdpp/test/test_results.h \
	$(top_srcdir)/external/libsimdpp/test/arch_registration.h \
	$(top_srcdir)/external/libsimdpp/perf/perf.h \
	$(top_srcdir)/external/GLV/test/test_glv.h \
	$(top_srcdir)/external/GLV/example/example.h \
	$(top_srcdir)/external/GLV/GLV/glv_color.h \
	$(top_srcdir)/external/GLV/GLV/glv_core.h \
	$(top_srcdir)/external/GLV/GLV/glv_sliders.h \
	$(top_srcdir)/external/GLV/GLV/glv_draw.h \
	$(top_srcdir)/external/GLV/GLV/glv_font.h \
	$(top_srcdir)/external/GLV/GLV/glv_texture.h \
	$(top_srcdir)/external/GLV/GLV/glv_color_controls.h \
	$(top_srcdir)/external/GLV/GLV/glv_sono.h \
	$(top_srcdir)/external/GLV/GLV/glv_rect.h \
	$(top_srcdir)/external/GLV/GLV/glv_binding.h \
	$(top_srcdir)/external/GLV/GLV/glv_model.h \
	$(top_srcdir)/external/GLV/GLV/glv_notification.h \
	$(top_srcdir)/external/GLV/GLV/glv_buttons.h \
	$(top_srcdir)/external/GLV/GLV/glv_conf.h \
	$(top_srcdir)/external/GLV/GLV/glv_view3D.h \
	$(top_srcdir)/external/GLV/GLV/glv_layout.h \
	$(top_srcdir)/external/GLV/GLV/glv_preset_controls.h \
	$(top_srcdir)/external/GLV/GLV/glv_behavior.h \
	$(top_srcdir)/external/GLV/GLV/glv_plots.h \
	$(top_srcdir)/external/GLV/GLV/glv_textview.h \
	$(top_srcdir)/external/GLV/GLV/glv_widget.h \
	$(top_srcdir)/external/GLV/GLV/glv.h \
	$(top_srcdir)/external/GLV/GLV/glv_icon.h \
	$(top_srcdir)/external/GLV/GLV/glv_util.h \
	$(top_srcdir)/external/GLV/GLV/glv_grid.h \
	\
	$(top_srcdir)/external/libsimdpp/simdpp/simd/float64x2.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int32x4.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int32x8.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int16x16.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/float32x4.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int16x8.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int8x16.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int64x2.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int64x4.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int256.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/float64x4.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/float32x8.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int128.inl \
	$(top_srcdir)/external/libsimdpp/simdpp/simd/int8x32.inl \
	\
	$(top_srcdir)/external/hap/source/hap.h
