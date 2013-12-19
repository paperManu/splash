#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.68)
AC_INIT(SHADEROMATIC, 0.0.9, emmanueldurand@gmail.com)
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE()

# Check for C and C++ compilers
AC_PROG_CXX
AC_PROG_CC

# Check for header files
AC_HEADER_STDC

# GLFW
PKG_CHECK_MODULES([LIBGLFW], [libglfw >= 2.7.4])
if test "x${have_glfw}" = "xfalse" ; then
    AC_MSG_ERROR([Missing glfw])
fi

# OpenGL
PKG_CHECK_MODULES([GL], [gl >= 2.3])
if test "x${have_gl}" = "xfalse" ; then
    AC_MSG_ERROR([Missing opengl])
fi

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    ])
AC_OUTPUT
