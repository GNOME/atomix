#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(test -f $srcdir/configure.ac \
  && test -f $srcdir/ChangeLog \
  && test -f $srcdir/src/level.h) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level Atomix directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "You need to install gnome-common package"
    exit 1
}

. gnome-autogen.sh
