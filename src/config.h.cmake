#ifndef CONFIG_H
#define CONFIG_H

/* Considering a Linux system */
#cmakedefine01 HAVE_LINUX

/* Apple Mac OS X operating system not detected */
#cmakedefine01 HAVE_OSX

/* Defined to 1 if libgphoto2 is detected */
#cmakedefine01 HAVE_GPHOTO

/* Defined to 1 if OpenCV is detected */
#cmakedefine01 HAVE_OPENCV

/* Defined to 1 if portaudio-2.0 is detected */
#cmakedefine01 HAVE_PORTAUDIO

/* Defined to 1 if jack is detected */
#cmakedefine01 HAVE_JACK

/* Defined to 1 if shmdata-1.0 is detected */
#cmakedefine01 HAVE_SHMDATA

/* Defined to 1 if python3.x is detected */
#cmakedefine01 HAVE_PYTHON

/* Defined to 1 if the Datapath SDK is detected */
#cmakedefine01 HAVE_DATAPATH

/* Support mmx instructions */
#cmakedefine01 HAVE_MMX

/* Support SSE (Streaming SIMD Extensions) instructions */
#cmakedefine01 HAVE_SSE

/* Support SSE2 (Streaming SIMD Extensions 2) instructions */
#cmakedefine01 HAVE_SSE2

/* Support SSE3 (Streaming SIMD Extensions 3) instructions */
#cmakedefine01 HAVE_SSE3

/* Support SSSE4.1 (Streaming SIMD Extensions 4.1) instructions */
#cmakedefine01 HAVE_SSE4_1

/* Support SSSE3 (Supplemental Streaming SIMD Extensions 3) instructions */
#cmakedefine01 HAVE_SSSE3

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION "@PACKAGE_VERSION@"

#endif // CONFIG_H
