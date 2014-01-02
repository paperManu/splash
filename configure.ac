#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.68)
AC_INIT(SPLASH, 0.0.1, emmanueldurand@gmail.com)
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE()
AC_CONFIG_HEADERS([include/config.h])

# Check for C and C++ compilers
AC_PROG_CXX
AC_PROG_CC

CXXFLAGS="$CFLAGS -std=gnu++0x"

export PKG_CONFIG_PATH=$srcdir/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig

# Check for header files
AC_HEADER_STDC

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
if test "$HAVE_GLM" = "false" ; then
    AC_MSG_ERROR([Missing glm])
fi

# OpenMesh
PKG_CHECK_MODULES([OPENMESH], [openmesh >= 3.0.0])
if test "x${have_openmesh}" = "xfalse" ; then
    AC_MSG_ERROR([Missing openmesh])
fi

# OpenImageIO
PKG_CHECK_MODULES([OPENIMAGEIO], [openimageio >= 1.3.0])
if test "x${have_openimageio}" = "xfalse" ; then
    AC_MSG_ERROR([Missing openimageio])
fi

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    tests/Makefile
    ])
AC_OUTPUT
