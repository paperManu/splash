#!/bin/sh
if test ! -e NEWS ; then
touch NEWS
fi

if test ! -e INSTALL ; then
touch INSTALL
fi

if test ! -e AUTHORS ; then
touch AUTHORS
fi

if test ! -e README ; then
touch README
fi

if test ! -e ChangeLog ; then
touch ChangeLog
fi

mkdir m4
autoreconf -fivI m4
#./configure
