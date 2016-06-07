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

#*************#
if HAVE_OSX
AM_CPPFLAGS += \
	-I$(top_srcdir)/external/syphon/build/Release/Syphon.framework/Headers/Syphon.h

clean: clean-am
	rm -rf $(top_srcdir)/external/syphon/build

#install: install-am
#	sh $(top_srcdir)/external/build_syphon.sh install
#	sh $(top_srcdir)/external/build_syphon.sh clean

BUILT_SOURCES = $(top_srcdir)/external/syphon/build/Release/Syphon.framework/Headers/Syphon.h

$(top_srcdir)/external/syphon/build/Release/Syphon.framework/Headers/Syphon.h: Makefile
	sh $(top_srcdir)/external/build_syphon.sh

export LIBTOOL = glibtool --tag=CXX
else
export LIBTOOL = $(top_builddir)/libtool --tag=CXX
endif

#*************#
lib_LTLIBRARIES = \
	libsplash-@LIBSPLASH_API_VERSION@.la

libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES = \
	camera.cpp \
	cgUtils.cpp \
	filter.cpp \
	geometry.cpp \
	gpuBuffer.cpp \
	gui.cpp \
	httpServer.cpp \
	image.cpp \
	imageBuffer.cpp \
	link.cpp \
	mesh.cpp \
	mesh_bezierPatch.cpp \
	object.cpp \
	queue.cpp \
	scene.cpp \
	shader.cpp \
	texture.cpp \
	texture_image.cpp \
	threadpool.cpp \
	warp.cpp \
	window.cpp \
    widget.cpp \
    widget_control.cpp \
    widget_global_view.cpp \
    widget_graph.cpp \
    widget_node_view.cpp \
    widget_media.cpp \
    widget_template.cpp \
    widget_text_box.cpp \
    widget_warp.cpp \
	world.cpp

# ImGui sources
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
	$(top_srcdir)/external/imgui/imgui_draw.cpp

# GLAD, a GL loader
if HAVE_OSX
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
	$(top_srcdir)/external/glad/compatibility/src/glad_compatibility.c
else
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
	$(top_srcdir)/external/glad/core/src/glad_core.c
endif

# JsonCpp sources
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
	$(top_srcdir)/external/jsoncpp/jsoncpp.cpp

# GUI related sources
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
	$(top_srcdir)/external/imgui/imgui.cpp

# Hap related sources
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
	$(top_srcdir)/external/hap/source/hap.c

libsplash_@LIBSPLASH_API_VERSION@_la_CXXFLAGS = \
	-I$(top_srcdir)/external/bandit/bandit \
	$(BOOST_CPPFLAGS) \
	$(GLFW_CFLAGS) \
	$(GL_CFLAGS) \
	$(GSL_CFLAGS) \
	$(JSONCPP_CFLAGS) \
	$(SIMD_FLAGS) \
	$(SHMDATA_CFLAGS) \
	$(ZMQ_CFLAGS)

libsplash_@LIBSPLASH_API_VERSION@_la_LDFLAGS = \
	-lsnappy \
	$(BOOST_SYSTEM_LIBS) \
	$(GLFW_LIBS) \
	$(GL_LIBS) \
	$(GSL_LIBS) \
	$(JSONCPP_LIBS) \
	$(SHMDATA_LIBS) \
	$(ZMQ_LIBS)

# GPhoto related
if HAVE_GPHOTO
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += colorcalibrator.cpp image_gphoto.cpp
libsplash_@LIBSPLASH_API_VERSION@_la_CXXFLAGS += $(GPHOTO_CFLAGS) $(GLIB_CFLAGS)
libsplash_@LIBSPLASH_API_VERSION@_la_LDFLAGS += $(GPHOTO_LIBS) $(GLIB_LIBS)
endif

## FFmpeg related
if HAVE_FFMPEG
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += image_ffmpeg.cpp
libsplash_@LIBSPLASH_API_VERSION@_la_CXXFLAGS += $(FFMPEG_CFLAGS)
libsplash_@LIBSPLASH_API_VERSION@_la_LDFLAGS += $(FFMPEG_LIBS)
endif

## PortAudio related
if HAVE_PORTAUDIO
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
    speaker.cpp \
    listener.cpp \
    ltcclock.cpp \
    $(top_srcdir)/external/libltc/src/decoder.c \
    $(top_srcdir)/external/libltc/src/encoder.c \
    $(top_srcdir)/external/libltc/src/ltc.c \
    $(top_srcdir)/external/libltc/src/timecode.c
libsplash_@LIBSPLASH_API_VERSION@_la_CXXFLAGS += $(PORTAUDIO_CFLAGS)
libsplash_@LIBSPLASH_API_VERSION@_la_LDFLAGS += $(PORTAUDIO_LIBS)
endif

## OpenCV related
if HAVE_OPENCV
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += image_opencv.cpp
libsplash_@LIBSPLASH_API_VERSION@_la_CXXFLAGS += $(OPENCV_CFLAGS)
libsplash_@LIBSPLASH_API_VERSION@_la_LDFLAGS += $(OPENCV_LIBS)
endif

## Shmdata related
if HAVE_SHMDATA
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += image_shmdata.cpp mesh_shmdata.cpp
libsplash_@LIBSPLASH_API_VERSION@_la_CXXFLAGS += $(SHMDATA_CFLAGS)
libsplash_@LIBSPLASH_API_VERSION@_la_LDFLAGS += $(SHMDATA_LIBS)
endif

# OSX
if HAVE_OSX
libsplash_@LIBSPLASH_API_VERSION@_la_SOURCES += \
	texture_syphon.cpp \
	texture_syphon_client.mm

libsplash_@LIBSPLASH_API_VERSION@_la_CXXFLAGS += \
	-F$(top_srcdir)/external/syphon/build/Release \
	-framework Syphon

libsplash_@LIBSPLASH_API_VERSION@_la_LDFLAGS += \
	-F$(top_srcdir)/external/syphon/build/Release\
	-framework Syphon

libsplash_@LIBSPLASH_API_VERSION@_la_OBJCXXFLAGS = \
	-F$(top_srcdir)/external/syphon/build/Release \
	-framework Syphon
endif

#*************#
bin_PROGRAMS = splash \
	splash-scene

splash_DEPENDENCIES = libsplash-@LIBSPLASH_API_VERSION@.la

splash_SOURCES = \
	splash-app.cpp

splash_CXXFLAGS = \
	$(GLFW_CFLAGS) \
	$(GL_CFLAGS) \
	$(GSL_CFLAGS) \
	$(JSONCPP_CFLAGS) \
	$(OPENIMAGEIO_CFLAGS) \
	$(SHMDATA_CFLAGS)

splash_LDFLAGS = \
	$(top_builddir)/src/libsplash-@LIBSPLASH_API_VERSION@.la \
	$(BOOST_SYSTEM_LIBS)

## FFmpeg related
if HAVE_FFMPEG
splash_CXXFLAGS += $(FFMPEG_CFLAGS)
splash_LDFLAGS += $(FFMPEG_LIBS)
endif

## OpenCV related
if HAVE_OPENCV
splash_CXXFLAGS += $(OPENCV_CFLAGS)
splash_LDFLAGS += $(OPENCV_LIBS)
endif

## Shmdata related
if HAVE_SHMDATA
splash_CXXFLAGS += $(SHMDATA_CFLAGS)
splash_LDFLAGS += $(SHMDATA_LIBS)
endif

if HAVE_OSX
splash_LDFLAGS += \
	-F$(top_srcdir)/external/syphon/build/Release \
	-framework Syphon

splash_OBJCXXFLAGS = \
	-F$(top_srcdir)/external/syphon/build/Release \
	-framework Syphon
endif

#*************#
splash_scene_DEPENDENCIES = libsplash-@LIBSPLASH_API_VERSION@.la

splash_scene_SOURCES = \
	splash-scene.cpp

splash_scene_CXXFLAGS = \
	$(GLFW_CFLAGS) \
	$(GL_CFLAGS) \
	$(GSL_CFLAGS) \
	$(JSONCPP_CFLAGS) \
	$(OPENIMAGEIO_CFLAGS) \
	$(SHMDATA_CFLAGS)

splash_scene_LDFLAGS = \
	$(top_builddir)/src/libsplash-@LIBSPLASH_API_VERSION@.la \
	$(BOOST_SYSTEM_LIBS)

## FFmpeg related
if HAVE_FFMPEG
splash_scene_CXXFLAGS += $(FFMPEG_CFLAGS)
splash_scene_LDFLAGS += $(FFMPEG_LIBS)
endif

## OpenCV related
if HAVE_OPENCV
splash_CXXFLAGS += $(OPENCV_CFLAGS)
splash_LDFLAGS += $(OPENCV_LIBS)
endif

## Shmdata related
if HAVE_SHMDATA
splash_CXXFLAGS += $(SHMDATA_CFLAGS)
splash_LDFLAGS += $(SHMDATA_LIBS)
endif

if HAVE_OSX
splash_scene_LDFLAGS += \
	-F$(top_srcdir)/external/syphon/build/Release \
	-framework Syphon

splash_scene_OBJCXXFLAGS = \
	-F$(top_srcdir)/external/syphon/build/Release \
	-framework Syphon
endif

#*************#
noinst_HEADERS = \
	$(top_srcdir)/include/basetypes.h \
	$(top_srcdir)/include/camera.h \
	$(top_srcdir)/include/cgUtils.h \
	$(top_srcdir)/include/colorcalibrator.h \
	$(top_srcdir)/include/coretypes.h \
	$(top_srcdir)/include/filter.h \
	$(top_srcdir)/include/geometry.h \
	$(top_srcdir)/include/gpuBuffer.h \
	$(top_srcdir)/include/gui.h \
	$(top_srcdir)/include/httpServer.h \
	$(top_srcdir)/include/image.h \
	$(top_srcdir)/include/imageBuffer.h \
	$(top_srcdir)/include/image_ffmpeg.h \
	$(top_srcdir)/include/image_gphoto.h \
	$(top_srcdir)/include/image_opencv.h \
	$(top_srcdir)/include/image_shmdata.h \
	$(top_srcdir)/include/ltcclock.h \
	$(top_srcdir)/include/link.h \
	$(top_srcdir)/include/listener.h \
	$(top_srcdir)/include/log.h \
	$(top_srcdir)/include/mesh.h \
	$(top_srcdir)/include/mesh_bezierPatch.h \
	$(top_srcdir)/include/mesh_shmdata.h \
	$(top_srcdir)/include/meshLoader.h \
	$(top_srcdir)/include/object.h \
	$(top_srcdir)/include/osUtils.h \
	$(top_srcdir)/include/queue.h \
	$(top_srcdir)/include/scene.h \
	$(top_srcdir)/include/shader.h \
	$(top_srcdir)/include/shaderSources.h \
	$(top_srcdir)/include/speaker.h \
	$(top_srcdir)/include/splash.h \
	$(top_srcdir)/include/texture.h \
	$(top_srcdir)/include/texture_image.h \
	$(top_srcdir)/include/texture_syphon.h \
	$(top_srcdir)/include/texture_syphon_client.h \
	$(top_srcdir)/include/threadpool.h \
	$(top_srcdir)/include/timer.h \
	$(top_srcdir)/include/warp.h \
    $(top_srcdir)/include/widget.h \
    $(top_srcdir)/include/widget_control.h \
    $(top_srcdir)/include/widget_global_view.h \
    $(top_srcdir)/include/widget_graph.h \
    $(top_srcdir)/include/widget_node_view.h \
    $(top_srcdir)/include/widget_media.h \
    $(top_srcdir)/include/widget_template.h \
    $(top_srcdir)/include/widget_text_box.h \
    $(top_srcdir)/include/widget_warp.h \
	$(top_srcdir)/include/window.h \
	$(top_srcdir)/include/world.h \
	\
	$(top_srcdir)/external/cppzmq/zmq.hpp \
	\
	$(top_srcdir)/external/glad/core/include/glad/glad.h \
	$(top_srcdir)/external/glad/core/include/KHR/khrplatform.h \
	$(top_srcdir)/external/glad/compatibility/include/glad/glad.h \
	$(top_srcdir)/external/glad/compatibility/include/KHR/khrplatform.h \
	\
	$(top_srcdir)/external/imgui/imgui.h \
	$(top_srcdir)/external/imgui/imgui_internal.h \
	$(top_srcdir)/external/imgui/imconfig.h \
	$(top_srcdir)/external/imgui/stb_rect_pack.h \
	$(top_srcdir)/external/imgui/stb_textedit.h \
	$(top_srcdir)/external/imgui/stb_truetype.h \
	\
	$(top_srcdir)/external/hap/source/hap.h \
	\
	$(top_srcdir)/external/jsoncpp/json/json.h \
    \
    $(top_srcdir)/external/libltc/src/decoder.h \
    $(top_srcdir)/external/libltc/src/encoder.h \
    $(top_srcdir)/external/libltc/src/ltc.h \
	\
	$(top_srcdir)/external/stb/stb_image.h \
	$(top_srcdir)/external/stb/stb_image_write.h \
	\
	$(top_srcdir)/external/piccante/include/image_samplers/image_sampler.hpp \
	$(top_srcdir)/external/piccante/include/image_samplers/image_sampler_gaussian.hpp \
	$(top_srcdir)/external/piccante/include/image_samplers/image_sampler_bilinear.hpp \
	$(top_srcdir)/external/piccante/include/image_samplers/image_sampler_bsplines.hpp \
	$(top_srcdir)/external/piccante/include/image_samplers/image_sampler_nearest.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/harris_corner_detector.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/poisson_descriptor.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/lucid_descriptor.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/patch_comp.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/canny_edge_detector.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/brief_descriptor.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/fast_corner_detector.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/motion_estimation.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/dense_sift.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/orb_descriptor.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/transform_data.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/susan_corner_detector.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/general_corner_detector.hpp \
	$(top_srcdir)/external/piccante/include/features_matching/ward_alignment.hpp \
	$(top_srcdir)/external/piccante/include/colors/color_3.hpp \
	$(top_srcdir)/external/piccante/include/colors/saturation.hpp \
	$(top_srcdir)/external/piccante/include/colors/color_conv_xyz_to_hdrlab.hpp \
	$(top_srcdir)/external/piccante/include/colors/color_conv.hpp \
	$(top_srcdir)/external/piccante/include/colors/color_conv_xyz_to_cielab.hpp \
	$(top_srcdir)/external/piccante/include/colors/color_conv_rgb_to_xyz.hpp \
	$(top_srcdir)/external/piccante/include/colors/color_conv_xyz_to_logluv.hpp \
	$(top_srcdir)/external/piccante/include/colors/color_conv_rgb_to_srgb.hpp \
	$(top_srcdir)/external/piccante/include/colors/color_conv_xyz_to_cieluv.hpp \
	$(top_srcdir)/external/piccante/include/colors/rgbe.hpp \
	$(top_srcdir)/external/piccante/include/util.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_absolute_difference.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_drago_tmo.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_bilateral_2das.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_demosaic.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_down_pp.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_guided.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_luminance.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_mosaic.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_exposure_fusion_weights.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_divergence.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_anisotropic_diffusion.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_min.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_combine.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_conv_1d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_normal.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_reconstruct.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_gaussian_3d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_crop.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_gradient.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_bilateral_2dsp.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_gaussian_1d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_nswe.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_wls.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_sampler_2dsub.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_simple_tmo.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_local_extrema.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_color_distance.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_channel.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_med.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_gaussian_2d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_color_conv.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_backward_difference.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_bilateral_1d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_remove_negative.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_dct_1d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_dct_2d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_iterative.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_assemble_hdr.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_warp_2d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_bilateral_2dg.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_sampling_map.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_integral_image.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_sigmoid_tmo.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_sampler_1d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_npasses.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_bilateral_2ds.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_kuwahara.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_laplacian.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_downsampler_2d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_up_pp.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_bilateral_2df.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_remove_nuked.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_sampler_3d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_conv_2dsp.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_diff_gauss_2d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_max.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_sampler_2dadd.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_mean.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_linear_color_space.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_conv_2d.hpp \
	$(top_srcdir)/external/piccante/include/filtering/filter_sampler_2d.hpp \
	$(top_srcdir)/external/piccante/include/io.hpp \
	$(top_srcdir)/external/piccante/include/piccante.hpp \
	$(top_srcdir)/external/piccante/include/metrics/psnr.hpp \
	$(top_srcdir)/external/piccante/include/metrics/m_psnr.hpp \
	$(top_srcdir)/external/piccante/include/metrics/maximum_error.hpp \
	$(top_srcdir)/external/piccante/include/metrics/mse.hpp \
	$(top_srcdir)/external/piccante/include/metrics/log_rmse.hpp \
	$(top_srcdir)/external/piccante/include/metrics/snr.hpp \
	$(top_srcdir)/external/piccante/include/metrics/base.hpp \
	$(top_srcdir)/external/piccante/include/metrics/relative_error.hpp \
	$(top_srcdir)/external/piccante/include/metrics/mae.hpp \
	$(top_srcdir)/external/piccante/include/util/image_sampler.hpp \
	$(top_srcdir)/external/piccante/include/util/tile_list.hpp \
	$(top_srcdir)/external/piccante/include/util/bbox.hpp \
	$(top_srcdir)/external/piccante/include/util/cached_table.hpp \
	$(top_srcdir)/external/piccante/include/util/low_dynamic_range.hpp \
	$(top_srcdir)/external/piccante/include/util/io.hpp \
	$(top_srcdir)/external/piccante/include/util/math.hpp \
	$(top_srcdir)/external/piccante/include/util/rasterizer.hpp \
	$(top_srcdir)/external/piccante/include/util/precomputed_gaussian.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/formats.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/redux.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/buffer_op.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/fbo.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/tone.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/mask.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/redux_ops.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/buffer_ops.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/quad.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/stroke.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/buffer_allocation.hpp \
	$(top_srcdir)/external/piccante/include/util/gl/timings.hpp \
	$(top_srcdir)/external/piccante/include/util/warp_square_circle.hpp \
	$(top_srcdir)/external/piccante/include/util/precomputed_difference_of_gaussians.hpp \
	$(top_srcdir)/external/piccante/include/util/nelder_mead_opt_test_function.hpp \
	$(top_srcdir)/external/piccante/include/util/nelder_mead_opt_triangulation.hpp \
	$(top_srcdir)/external/piccante/include/util/compability.hpp \
	$(top_srcdir)/external/piccante/include/util/nelder_mead_opt_fundamental.hpp \
	$(top_srcdir)/external/piccante/include/util/nelder_mead_opt_gordon_lowe.hpp \
	$(top_srcdir)/external/piccante/include/util/array.hpp \
	$(top_srcdir)/external/piccante/include/util/eigen_util.hpp \
	$(top_srcdir)/external/piccante/include/util/matrix_3_x_3.hpp \
	$(top_srcdir)/external/piccante/include/util/mask.hpp \
	$(top_srcdir)/external/piccante/include/util/buffer.hpp \
	$(top_srcdir)/external/piccante/include/util/raw.hpp \
	$(top_srcdir)/external/piccante/include/util/nelder_mead_opt_base.hpp \
	$(top_srcdir)/external/piccante/include/util/indexed_array.hpp \
	$(top_srcdir)/external/piccante/include/util/nelder_mead_opt_homography.hpp \
	$(top_srcdir)/external/piccante/include/util/convert_raw_to_images.hpp \
	$(top_srcdir)/external/piccante/include/util/vec.hpp \
	$(top_srcdir)/external/piccante/include/util/string.hpp \
	$(top_srcdir)/external/piccante/include/util/tile.hpp \
	$(top_srcdir)/external/piccante/include/util/computer_vision_functions.hpp \
	$(top_srcdir)/external/piccante/include/util/file_lister.hpp \
	$(top_srcdir)/external/piccante/include/util/point_samplers.hpp \
	$(top_srcdir)/external/piccante/include/qt.hpp \
	$(top_srcdir)/external/piccante/include/gl/colors/color_conv_rgb_to_hsl.hpp \
	$(top_srcdir)/external/piccante/include/gl/colors/color_conv.hpp \
	$(top_srcdir)/external/piccante/include/gl/colors/color_conv_xyz_to_cielab.hpp \
	$(top_srcdir)/external/piccante/include/gl/colors/color_conv_rgb_to_xyz.hpp \
	$(top_srcdir)/external/piccante/include/gl/colors/color_conv_rgb_to_srgb.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_drago_tmo.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_2das.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_down_pp.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_luminance.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_exposure_fusion_weights.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_anisotropic_diffusion.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_min.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_remapping.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_conv_1d.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_gaussian_3d.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_3dsp.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_gradient.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_2dsp.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_gaussian_1d.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_non_linear_1d.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_blend.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_slicer.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_disp.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_simple_tmo.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_op.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_channel.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_gaussian_2d.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_color_conv.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_2ds_e.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_hsl_replace.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_1d.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_scatter.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_iterative.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_warp_2d.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_2dg.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_sampling_map.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_sigmoid_tmo.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_3das.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_npasses.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_2ds.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_3ds.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_laplacian.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_up_pp.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_bilateral_2df.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_remove_nuked.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_thresholding.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_anaglyph.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_max.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_mean.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_sampler_2d.hpp \
	$(top_srcdir)/external/piccante/include/gl/filtering/filter_1d.hpp \
	$(top_srcdir)/external/piccante/include/gl/algorithms/color_to_gray.hpp \
	$(top_srcdir)/external/piccante/include/gl/algorithms/pushpull.hpp \
	$(top_srcdir)/external/piccante/include/gl/algorithms/pyramid.hpp \
	$(top_srcdir)/external/piccante/include/gl/point_samplers/sampler_random_m.hpp \
	$(top_srcdir)/external/piccante/include/gl/image_vec.hpp \
	$(top_srcdir)/external/piccante/include/gl/tone_mapping/reinhard_tmo.hpp \
	$(top_srcdir)/external/piccante/include/gl/tone_mapping/get_all_exposures.hpp \
	$(top_srcdir)/external/piccante/include/gl/tone_mapping/segmentation_tmo_approx.hpp \
	$(top_srcdir)/external/piccante/include/gl/tone_mapping/exposure_fusion.hpp \
	$(top_srcdir)/external/piccante/include/gl/tone_mapping/drago_tmo.hpp \
	$(top_srcdir)/external/piccante/include/gl/tone_mapping/hybrid_tmo.hpp \
	$(top_srcdir)/external/piccante/include/gl/image.hpp \
	$(top_srcdir)/external/piccante/include/externals.hpp \
	$(top_srcdir)/external/piccante/include/filtering.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/superpixels_slic.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/color_to_gray.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/camera_response_function.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/poisson_image_editing.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/connected_components.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/quadtree.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/calculate_divergence.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/region_border.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/pushpull.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/poisson_solver_iterative.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/poisson_filling.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/discrete_cosine_transform.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/edge_enhancement.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/richardson_lucy_deconvolution.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/poisson_solver.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/flash_photography.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/superpixels_oracle.hpp \
	$(top_srcdir)/external/piccante/include/algorithms/pyramid.hpp \
	$(top_srcdir)/external/piccante/include/histogram.hpp \
	$(top_srcdir)/external/piccante/include/point_samplers/sampler_monte_carlo.hpp \
	$(top_srcdir)/external/piccante/include/point_samplers/sampler_dart_throwing.hpp \
	$(top_srcdir)/external/piccante/include/point_samplers/sampler_bridson.hpp \
	$(top_srcdir)/external/piccante/include/point_samplers/sampler_random.hpp \
	$(top_srcdir)/external/piccante/include/point_samplers/sampler_random_m.hpp \
	$(top_srcdir)/external/piccante/include/features_matching.hpp \
	$(top_srcdir)/external/piccante/include/base.hpp \
	$(top_srcdir)/external/piccante/include/io/bmp.hpp \
	$(top_srcdir)/external/piccante/include/io/pgm.hpp \
	$(top_srcdir)/external/piccante/include/io/vol.hpp \
	$(top_srcdir)/external/piccante/include/io/pfm.hpp \
	$(top_srcdir)/external/piccante/include/io/ppm.hpp \
	$(top_srcdir)/external/piccante/include/io/exr.hpp \
	$(top_srcdir)/external/piccante/include/io/tmp.hpp \
	$(top_srcdir)/external/piccante/include/io/tga.hpp \
	$(top_srcdir)/external/piccante/include/io/hdr.hpp \
	$(top_srcdir)/external/piccante/include/image_vec.hpp \
	$(top_srcdir)/external/piccante/include/algorithms.hpp \
	$(top_srcdir)/external/piccante/include/colors.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping.hpp \
	$(top_srcdir)/external/piccante/include/gl.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/reinhard_tmo.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/histogram_tmo.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/get_all_exposures.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/ward_histogram_tmo.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/segmentation_tmo_approx.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/lischinski_tmo.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/find_best_exposure.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/exposure_fusion.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/drago_tmo.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/lischinski_minimization.hpp \
	$(top_srcdir)/external/piccante/include/tone_mapping/hybrid_tmo.hpp \
	$(top_srcdir)/external/piccante/include/image_samplers.hpp \
	$(top_srcdir)/external/piccante/include/metrics.hpp \
	$(top_srcdir)/external/piccante/include/externals/glw/program.hpp \
	$(top_srcdir)/external/piccante/include/externals/glw/base.hpp \
	$(top_srcdir)/external/piccante/include/externals/glw/utility.hpp \
	$(top_srcdir)/external/piccante/include/image.hpp \
	$(top_srcdir)/external/piccante/include/point_samplers.hpp \
	\
	$(top_srcdir)/external/piccante/include/externals/Eigen/Array \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Cholesky \
	$(top_srcdir)/external/piccante/include/externals/Eigen/CholmodSupport \
	$(top_srcdir)/external/piccante/include/externals/Eigen/CMakeLists.txt \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Core \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Dense \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Eigen \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Eigen2Support \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Eigenvalues \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Geometry \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Householder \
	$(top_srcdir)/external/piccante/include/externals/Eigen/IterativeLinearSolvers \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Jacobi \
	$(top_srcdir)/external/piccante/include/externals/Eigen/LeastSquares \
	$(top_srcdir)/external/piccante/include/externals/Eigen/LU \
	$(top_srcdir)/external/piccante/include/externals/Eigen/MetisSupport \
	$(top_srcdir)/external/piccante/include/externals/Eigen/OrderingMethods \
	$(top_srcdir)/external/piccante/include/externals/Eigen/PardisoSupport \
	$(top_srcdir)/external/piccante/include/externals/Eigen/PaStiXSupport \
	$(top_srcdir)/external/piccante/include/externals/Eigen/QR \
	$(top_srcdir)/external/piccante/include/externals/Eigen/QtAlignedMalloc \
	$(top_srcdir)/external/piccante/include/externals/Eigen/Sparse \
	$(top_srcdir)/external/piccante/include/externals/Eigen/SparseCholesky \
	$(top_srcdir)/external/piccante/include/externals/Eigen/SparseCore \
	$(top_srcdir)/external/piccante/include/externals/Eigen/SparseLU \
	$(top_srcdir)/external/piccante/include/externals/Eigen/SparseQR \
	$(top_srcdir)/external/piccante/include/externals/Eigen/SPQRSupport \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src \
	$(top_srcdir)/external/piccante/include/externals/Eigen/StdDeque \
	$(top_srcdir)/external/piccante/include/externals/Eigen/StdList \
	$(top_srcdir)/external/piccante/include/externals/Eigen/StdVector \
	$(top_srcdir)/external/piccante/include/externals/Eigen/SuperLUSupport \
	$(top_srcdir)/external/piccante/include/externals/Eigen/SVD \
	$(top_srcdir)/external/piccante/include/externals/Eigen/UmfPackSupport \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/MathFunctions.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Transpositions.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/NoAlias.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Fuzzy.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/VectorwiseOp.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/EigenBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/GenericPacketMath.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Ref.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Dot.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/CwiseUnaryView.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/XprHelper.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/DisableStupidWarnings.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/MKL_support.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/NonMPL2.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/ForwardDeclarations.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/ReenableStupidWarnings.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/Constants.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/Memory.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/StaticAssert.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/BlasUtil.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/Meta.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/util/Macros.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/DiagonalProduct.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/CoreIterators.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/NestByValue.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/StableNorm.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Redux.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Block.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/PlainObjectBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/CwiseNullaryOp.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Matrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Random.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/CwiseBinaryOp.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/MatrixBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/MapBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Replicate.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/PermutationMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Transpose.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/DenseStorage.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/DenseBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/GeneralBlockPanelKernel.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/GeneralMatrixVector_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/TriangularMatrixMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/TriangularSolverVector.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/GeneralMatrixMatrixTriangular_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/GeneralMatrixMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/TriangularMatrixVector.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/SelfadjointMatrixVector_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/SelfadjointMatrixMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/SelfadjointMatrixMatrix_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/GeneralMatrixMatrixTriangular.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/TriangularMatrixVector_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/SelfadjointProduct.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/TriangularMatrixMatrix_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/GeneralMatrixMatrix_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/Parallelizer.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/TriangularSolverMatrix_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/CoeffBasedProduct.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/SelfadjointMatrixVector.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/GeneralMatrixVector.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/TriangularSolverMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/products/SelfadjointRank2Update.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/SelfCwiseBinaryOp.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Visitor.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/arch/Default/Settings.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/arch/SSE/MathFunctions.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/arch/SSE/Complex.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/arch/SSE/PacketMath.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/arch/AltiVec/Complex.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/arch/AltiVec/PacketMath.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/arch/NEON/Complex.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/arch/NEON/PacketMath.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/BandMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Select.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/GlobalFunctions.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Array.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/CwiseUnaryOp.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Stride.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Assign.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/SelfAdjointView.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/NumTraits.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/TriangularMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/DenseCoeffsBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Flagged.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/CommaInitializer.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/IO.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/BooleanRedux.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/SolveTriangular.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/ArrayWrapper.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/ReturnByValue.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/GeneralProduct.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Diagonal.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Reverse.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Assign_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/VectorBlock.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/ArrayBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/ForceAlignedAccess.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/ProductBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Map.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/DiagonalMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Functors.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Core/Swap.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/CholmodSupport/CholmodSupport.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/RealSchur.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/SelfAdjointEigenSolver_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/SelfAdjointEigenSolver.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/RealQZ.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/MatrixBaseEigenvalues.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/Tridiagonalization.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/RealSchur_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/ComplexSchur.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/GeneralizedEigenSolver.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/ComplexSchur_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/EigenSolver.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/ComplexEigenSolver.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/GeneralizedSelfAdjointEigenSolver.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigenvalues/HessenbergDecomposition.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/IterativeLinearSolvers/BasicPreconditioners.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/IterativeLinearSolvers/BiCGSTAB.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/IterativeLinearSolvers/IterativeSolverBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/IterativeLinearSolvers/ConjugateGradient.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/IterativeLinearSolvers/IncompleteLUT.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/MetisSupport/MetisSupport.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Householder/HouseholderSequence.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Householder/Householder.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Householder/BlockHouseholder.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseQR/SparseQR.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SuperLUSupport/SuperLUSupport.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_heap_relax_snode.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_panel_dfs.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_copy_to_ucol.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_column_bmod.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_Structs.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_pruneL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_panel_bmod.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_Memory.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_Utils.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_column_dfs.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLUImpl.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_gemm_kernel.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_SupernodalMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_pivotL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_relax_snode.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseLU/SparseLU_kernel_bmod.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseTranspose.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseCwiseUnaryOp.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseSelfAdjointView.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseSparseProductWithPruning.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseDot.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/AmbiVector.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/CompressedStorage.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/TriangularSolver.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseColEtree.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseMatrixBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseBlock.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseView.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseFuzzy.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseCwiseBinaryOp.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseProduct.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparsePermutation.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseRedux.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/ConservativeSparseSparseProduct.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseTriangularView.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseVector.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseUtil.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/MappedSparseMatrix.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseDiagonalProduct.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCore/SparseDenseProduct.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/OrderingMethods/Amd.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/OrderingMethods/Ordering.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/OrderingMethods/Eigen_Colamd.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Jacobi/Jacobi.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCholesky/SimplicialCholesky.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SparseCholesky/SimplicialCholesky_impl.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/MathFunctions.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Lazy.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/CwiseOperators.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Block.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/LU.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Memory.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/TriangularSolver.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Minor.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/SVD.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/QR.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Meta.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Macros.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/LeastSquares.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Cwise.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/All.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/Quaternion.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/Transform.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/Hyperplane.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/ParametrizedLine.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/AngleAxis.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/Scaling.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/RotationBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/Translation.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/AlignedBox.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/Geometry/Rotation2D.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Eigen2Support/VectorBlock.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/plugins/MatrixCwiseUnaryOps.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/plugins/ArrayCwiseUnaryOps.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/plugins/CommonCwiseUnaryOps.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/plugins/MatrixCwiseBinaryOps.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/plugins/CommonCwiseBinaryOps.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/plugins/BlockMethods.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/plugins/ArrayCwiseBinaryOps.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/QR/ColPivHouseholderQR_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/QR/FullPivHouseholderQR.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/QR/ColPivHouseholderQR.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/QR/HouseholderQR.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/QR/HouseholderQR_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SVD/JacobiSVD_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SVD/UpperBidiagonalization.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SVD/JacobiSVD.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/UmfPackSupport/UmfPackSupport.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/SPQRSupport/SuiteSparseQRSupport.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Cholesky/LLT.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Cholesky/LLT_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Cholesky/LDLT.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/StlSupport/StdList.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/StlSupport/details.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/StlSupport/StdVector.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/StlSupport/StdDeque.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/PardisoSupport/PardisoSupport.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/Quaternion.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/Umeyama.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/Transform.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/Hyperplane.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/ParametrizedLine.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/AngleAxis.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/Scaling.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/RotationBase.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/Translation.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/Homogeneous.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/arch/Geometry_SSE.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/EulerAngles.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/AlignedBox.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/Rotation2D.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/Geometry/OrthoMethods.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/misc/blas.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/misc/Kernel.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/misc/SparseSolve.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/misc/Solve.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/misc/Image.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/LU/arch/Inverse_SSE.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/LU/FullPivLU.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/LU/Determinant.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/LU/Inverse.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/LU/PartialPivLU.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/LU/PartialPivLU_MKL.h \
	$(top_srcdir)/external/piccante/include/externals/Eigen/src/PaStiXSupport/PaStiXSupport.h \
	$(top_srcdir)/external/piccante/include/externals/gl_core_4_0.h
