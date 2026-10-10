## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in


SUBDIRS = doc

if HAVE_FVWM
SUBDIRS += fvwm-insitu
endif

SUBDIRS += libmetisse xlibs xserver libutils bin shaders

if HAVE_FVWMCOMPOSITOR
SUBDIRS += FvwmCompositor
endif

SUBDIRS += @WMTRACE_SUBDIR@ rpm

EXTRA_DIST = config-not-xserver.h

## ---------------------------------------------------------------------------
## Produce an rpm package using dist or from the given tarball
## (requires rpmbuild in your system)
#  Usage:
#    make rpm-dist
#    make release=1 rpm-dist
#    make rpm-this
#    make version=2.3.22 release=2 rpm-this
#    make rpm-dist cparams='--enable-multibyte --quiet' mparams='CFLAGS="-O2 -g"'

# automatical regeneration is missing for other dirs, so do it explicitly
rpm-regenerate:
	(cd rpm && $(MAKE) $(AM_MAKEFLAGS) Makefile *.spec) || exit 1

rpm-dist: dist rpm-this

rpm-this: rpm-regenerate
	(cd rpm && $(MAKE) $(AM_MAKEFLAGS) this) || exit 1

