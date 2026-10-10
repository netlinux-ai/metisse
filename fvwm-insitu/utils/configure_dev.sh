#!/bin/sh

# reconfigure CVS source code
# must be called from main fvwm directory
set -x
aclocal -I m4 || exit 1
autoheader || exit 2
automake --add-missing || exit 3
autoconf || exit 4
#./configure ${1+"$@"} || exit 5
