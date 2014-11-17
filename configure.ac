#											   -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.68)
AC_INIT(SPLASH, 0.1.9, emmanueldurand@gmail.com)
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([include/config.h])

AC_SUBST([LIBSPLASH_SO_VERSION], [0:1:9])
AC_SUBST([LIBSPLASH_API_VERSION], [0.1])
AC_DEFINE(LIBSPLASH_API_VERSION, ["0.1"], [API version])

# Check for C and C++ compilers
AC_PROG_CXX
AC_PROG_CC_C_O

CXXFLAGS="$CXXFLAGS -std=c++11"

export PKG_CONFIG_PATH=$srcdir/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig

# Check for header files
AC_HEADER_STDC

# Check for SSE
AX_EXT
if test "$ax_cv_have_sse41_ext" = yes; then
    CXXFLAGS="$CXXFLAGS -DSIMDPP_ARCH_X86_SSE4_1"
else
    if test "$ax_cv_have_sse3_ext" = yes; then
        CXXFLAGS="$CXXFLAGS -DSIMDPP_ARCH_X86_SSE3"
    else
        CXXFLAGS="$CXXFLAGS -DSIMDPP_NO_SSE"
    fi
fi

# GLFW
PKG_CHECK_MODULES([GLFW], [glfw3 >= 3.0.3])
if test "x${have_glfw}" = "xfalse" ; then
	AC_MSG_ERROR([Missing glfw])
fi

# OpenGL
PKG_CHECK_MODULES([GL], [gl >= 2.3])
if test "x${have_gl}" = "xfalse" ; then
	AC_MSG_ERROR([Missing opengl])
fi

# GLM
AC_LANG_PUSH([C++])
AC_CHECK_HEADER([glm/glm.hpp],
	[HAVE_GLM=true],
	[HAVE_GLM=false])
AC_LANG_POP
if test "$HAVE_GLM" = "false" ; then
	AC_MSG_ERROR([Missing glm])
fi

# Glew, needed by the embedded GLV library
PKG_CHECK_MODULES([GLEW], [glew])
if test "x${have_glew}" = "xfalse" ; then
	AC_MSG_ERROR([Missing GLEW])
fi

# OpenImageIO
AC_LANG_PUSH([C++])
AC_CHECK_HEADER([OpenImageIO/imageio.h],
    [HAVE_OIIO=true],
    [HAVE_OIIO=false])
AC_LANG_POP
if test "$HAVE_OIIO" = "false" ; then
    AC_MSG_ERROR([Missing OpenImageIO])
fi

# Snappy
AC_LANG_PUSH([C])
AC_CHECK_HEADER([snappy-c.h],
    [HAVE_SNAPPY=true],
    [HAVE_SNAPPY=false])
AC_LANG_POP
if test "$HAVE_SNAPPY" = "false" ; then
    AC_MSG_ERROR([Missing Snappy])
fi

# OpenMesh
PKG_CHECK_MODULES([OPENMESH], [openmesh >= 3.0.0])
if test "x${have_openmesh}" = "xfalse" ; then
	AC_MSG_ERROR([Missing openmesh])
fi

# JSoncpp
PKG_CHECK_MODULES([JSONCPP], [jsoncpp])
if test "x${have_jsoncpp}" = "xfalse" ; then
	AC_MSG_ERROR([Missing jsoncpp])
fi

# Shmdata
PKG_CHECK_MODULES([SHMDATA], [shmdata-any-0.8])
if test "x${have_shmdata}" = "xfalse" ; then
	AC_MSG_ERROR([Missing shmdata])
fi

# GNU Scientific Library, also used for calibration
PKG_CHECK_MODULES([GSL], [gsl])
if test "x${have_gsl}" = "xfalse" ; then
    AC_MSG_ERROR([Missing gsl])
fi

# ZMQ, for communication between World and Scenes
PKG_CHECK_MODULES([ZMQ], [libzmq > 3.0])
if test "x${have_zmq}" = "xfalse" ; then
    AC_MSG_ERROR([Missing libzmq])
fi

# Check for Markdown to generate documentation
AC_CHECK_PROGS([MARKDOWN], [markdown])
if test "x$MARKDOWN" = "xtrue" ; then
    AC_MSG_WARN([Markdown was not found - README.md will not be converted to HTML])
fi
AM_CONDITIONAL([HAVE_MARKDOWN], [test -n "$MARKDOWN"])

# Some options
AC_ARG_WITH([tests],
    AS_HELP_STRING([--with-tests], [If enabled, all tests are enabled]),
    [],
    [with_tests=no])
AM_CONDITIONAL(HAVE_TESTS, test "x$with_tests" != "xno")

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	data/Makefile
	tests/Makefile
	])
AC_OUTPUT
