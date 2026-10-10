dnl
dnl  Copyright 2003 Olivier Chapuis
dnl
dnl   strongly modified for metisse

dnl
dnl
dnl Copyright © 2003 Keith Packard
dnl
dnl Permission to use, copy, modify, distribute, and sell this software and its
dnl documentation for any purpose is hereby granted without fee, provided that
dnl the above copyright notice appear in all copies and that both that
dnl copyright notice and this permission notice appear in supporting
dnl documentation, and that the name of Keith Packard not be used in
dnl advertising or publicity pertaining to distribution of the software without
dnl specific, written prior permission.  Keith Packard makes no
dnl representations about the suitability of this software for any purpose.  It
dnl is provided "as is" without express or implied warranty.
dnl
dnl KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
dnl INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
dnl EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
dnl CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
dnl DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
dnl TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
dnl PERFORMANCE OF THIS SOFTWARE.
dnl
dnl Process this file with autoconf to create configure.

AC_PREREQ(2.57)
AC_INIT([metisse],0.4.1,metisse@lists.lri.fr)
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([dist-bzip2])
dnl AM_MAINTAINER_MODE
AC_CONFIG_HEADER(config.h)
AC_CONFIG_HEADERS(xserver/include/config/dix-config.h)

ISRELEASED=no

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
# AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_PROG_MAKE_SET
AM_PROG_AS
PKG_PROG_PKG_CONFIG

with_python=no
problem_python="not found"
AM_PATH_PYTHON(2.0, problem_python=""; HAVE_PYTHON=1; with_python=yes, HAVE_PYTHON=no)
AM_CONDITIONAL([HAVE_PYTHON], [test x"$HAVE_PYTHON" != xno])
AC_SUBST(HAVE_PYTHON)

# expand libexecdir
test "x$prefix" = xNONE && prefix="$ac_default_prefix"
test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
eval MYPYTHONDIR="`eval echo ${pythondir}`"
MYPYTHONDIR="${MYPYTHONDIR}"
AC_SUBST(MYPYTHONDIR)


AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN([ENDIAN="X_BIG_ENDIAN"], [ENDIAN="X_LITTLE_ENDIAN"])

AC_CHECK_SIZEOF([unsigned long])
if test "$ac_cv_sizeof_unsigned_long" = 8; then
	AC_DEFINE(_XSERVER64, 1, [Define to 1 if unsigned long is 64 bits.])
fi

AC_TYPE_PID_T

dnl Checks for library functions. Some test are here just to avoid conflict
dnl with fvwm "config.h"
AC_TYPE_SIGNAL
AC_FUNC_ALLOCA
AC_FUNC_STRFTIME
AC_FUNC_SETPGRP
dnl fvwm have some remplacement for these functions 
AC_CHECK_FUNCS(gethostname strcasecmp strncasecmp strdup strerror usleep atexit)

AC_FUNC_VPRINTF
AC_CHECK_FUNCS(geteuid getuid link memset mkstemp strchr strrchr strtol getopt getopt_long vsnprintf walkcontext backtrace getisax)
AC_FUNC_VPRINTF
AC_FUNC_ALLOCA
dnl Old HAS_* names used in os/*.c.
AC_CHECK_FUNC([getdtablesize],
	AC_DEFINE(HAS_GETDTABLESIZE, 1, [Have the `getdtablesize' function.]))
AC_CHECK_FUNC([getifaddrs],
	AC_DEFINE(HAS_GETIFADDRS, 1, [Have the `getifaddrs' function.]))
AC_CHECK_FUNC([getpeereid],
	AC_DEFINE(HAS_GETPEEREID, 1, [Have the `getpeereid' function.]))
AC_CHECK_FUNC([getpeerucred],
	AC_DEFINE(HAS_GETPEERUCRED, 1, [Have the `getpeerucred' function.]))
AC_CHECK_FUNC([strlcat], HAVE_STRLCAT=yes, HAVE_STRLCAT=no)
AM_CONDITIONAL(NEED_STRLCAT, [test x$HAVE_STRLCAT = xno])	

AM_CONDITIONAL(NEED_VSNPRINTF, [test x$HAVE_VSNPRINTF = xno])

dnl Check for mmap support for Xvfb
AC_CHECK_FUNC([mmap], AC_DEFINE(HAS_MMAP, 1, [Have the `mmap' function.]))

# Find the math libary
AC_CHECK_LIB(m,sqrt)

AC_CHECK_HEADERS([ndbm.h dbm.h rpcsvc/dbm.h])

dnl ---------------------------------------------------------------------------
dnl Bus options and CPU capabilities. 
dnl ---------------------------------------------------------------------------
DEFAULT_INT10="x86emu"
use_x86_asm="no"

dnl Override defaults as needed for specific platforms:

case $host_cpu in
  alpha*)
	ALPHA_VIDEO=yes
	case $host_os in
		*netbsd*)	AC_DEFINE(USE_ALPHA_PIO, 1, [NetBSD PIO alpha IO]) ;;
	esac
	;;
  arm*)
	ARM_VIDEO=yes
	;;
  i*86)
	use_x86_asm="yes"
	I386_VIDEO=yes
	case $host_os in
		*linux*)	DEFAULT_INT10=vm86 ;;
		*freebsd*)	AC_DEFINE(USE_DEV_IO) ;;
		*netbsd*)	AC_DEFINE(USE_I386_IOPL)
				SYS_LIBS=-li386
				;;
		*openbsd*)	AC_DEFINE(USE_I386_IOPL) 
				SYS_LIBS=-li386
				;;
	esac
        ;;
  cygwin*)
        CFLAGS="$CFLAGS -DFD_SETSIZE=256" 
	;;
  powerpc*)
	PPC_VIDEO=yes
	AC_DEFINE(__powerpc__,1,[powerpc architecture])
	case $host_os in
		*freebsd*)	DEFAULT_INT10=stub ;;
	esac
	;;
  sparc*)
	xorg_loader_sparcmuldiv="yes"
	SPARC64_VIDEO=yes
	BSD_ARCH_SOURCES="sparc64_video.c ioperm_noop.c"
	;;
  x86_64*|amd64*)
  	use_x86_asm="yes"
	I386_VIDEO=yes
	case $host_os in
		*freebsd*)	AC_DEFINE(USE_DEV_IO, 1, [BSD /dev/io]) ;;
		*netbsd*)	AC_DEFINE(USE_I386_IOPL, 1, [BSD i386 iopl])
				SYS_LIBS=-lx86_64
				;;
		*openbsd*)	AC_DEFINE(USE_AMD64_IOPL, 1, [BSD AMD64 iopl])
				SYS_LIBS=-lamd64
				;;
	esac
	;;
esac

dnl BSD *_video.c selection
AM_CONDITIONAL(ALPHA_VIDEO, [test "x$ALPHA_VIDEO" = xyes])
AM_CONDITIONAL(ARM_VIDEO, [test "x$ARM_VIDEO" = xyes])
AM_CONDITIONAL(I386_VIDEO, [test "x$I386_VIDEO" = xyes])
AM_CONDITIONAL(PPC_VIDEO, [test "x$PPC_VIDEO" = xyes])
AM_CONDITIONAL(SPARC64_VIDEO, [test "x$SPARC64_VIDEO" = xyes])
AM_CONDITIONAL(__powerpc__,[test "x$__powerpc__" = "x1"])

# FIXME: remove this?
AC_MSG_CHECKING(if unaligned word accesses behave as expected)
AC_TRY_RUN([
#include <stdio.h>

int main()
{
        char j[8]={1,2,3,4,5,6,7,8};
        int k= *(int *)(j+1);
        if(k==0x02030405 || k==0x05040302)
                return 0;
        return 1;
}
],
 [AC_DEFINE(WORKING_UNALIGNED_INT, 1, [unaligned word accesses behave as expected])
  AC_MSG_RESULT(yes)],
 AC_MSG_RESULT(no),
 AC_MSG_RESULT(assuming not on target machine))


OSNAME=`uname -srm`
AC_DEFINE_UNQUOTED(OSNAME, "$OSNAME", 
	[Define to OS Name string to display for build OS in Xorg log])

DEFAULT_VENDOR_NAME="InSitu Project"
DEFAULT_VENDOR_NAME_SHORT="InSitu"
DEFAULT_VERSION_MAJOR=0
DEFAULT_VERSION_MINOR=4
DEFAULT_VERSION_PATCH=0
DEFAULT_VERSION_SNAP=4
#DEFAULT_RELEASE_DATE="1 December 2006"
DEFAULT_RELEASE_DATE="from cvs "`date`
DEFAULT_VENDOR_WEB="http://www.insitu.fr/metisse"

m4_ifdef([AS_HELP_STRING], , [m4_define([AS_HELP_STRING], m4_defn([AC_HELP_STRING]))])

dnl Build options.

AC_ARG_ENABLE(mmx, AS_HELP_STRING([--enable-mmx],[enable MMX fb code]),
  [MMX=$enableval],[MMX=no])

mmx_capable=no
with_mmx=no
problem_mmx="disabled by default try --enable-mmx"

if test "x$MMX" = "xyes"; then
  AC_MSG_CHECKING(for MMX capable platform)
  if test "x$use_x86_asm" = xyes && test "x$GCC" = xyes ; then
	  AC_PREPROC_IFELSE([
#if (!defined (__GNUC__) || __GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 4))
    #error Not supported
#endif
], mmx_capable=yes;with_mmx=yes;problem_mmx="", mmx_capable=no;problem_mmx="platform does not support MMX")
  fi
  AC_MSG_RESULT([$mmx_capable])
fi
AM_CONDITIONAL(MMX_CAPABLE, [test "x$mmx_capable" = xyes])

AC_ARG_ENABLE(werror,        AS_HELP_STRING([--enable-werror],
				  [Treat warnings as errors (default: disabled)]),
			        [WERROR=$enableval], [WERROR=no])
AC_ARG_ENABLE(debug,         AS_HELP_STRING([--enable-debug],
				  [Enable debugging (default: disabled)]),
			        [DEBUGGING=$enableval], [DEBUGGING=no])

AC_ARG_WITH(vendor-name,     AS_HELP_STRING([--with-vendor-string=VENDOR],
				  [Vendor string reported by the server]),
			     	[ VENDOR_STRING="$withval" ],
			     	[ VENDOR_STRING="$DEFAULT_VENDOR_NAME" ])
AC_ARG_WITH(vendor-name-short, AS_HELP_STRING([--with-vendor-string-short=VENDOR],
				  [Short version of vendor string reported by the server]),
			     	[ VENDOR_STRING_SHORT="$withval" ],
			     	[ VENDOR_STRING_SHORT="$DEFAULT_VENDOR_NAME_SHORT" ])
AC_ARG_WITH(release-major,   AS_HELP_STRING([--with-release-major=MAJOR],
				  [Release major number reported by the server]),
				[ VERSION_MAJOR="$withval" ],
				[ VERSION_MAJOR="$DEFAULT_VERSION_MAJOR" ])
AC_ARG_WITH(release-minor,   AS_HELP_STRING([--with-release-minor=MINOR],
				  [Release minor number reported by the server]),
				[ VERSION_MINOR="$withval" ],
				[ VERSION_MINOR="$DEFAULT_VERSION_MINOR" ])
AC_ARG_WITH(release-patch,   AS_HELP_STRING([--with-release-patch=PATCH],
				  [Release patch number reported by the server]),
				[ VERSION_PATCH="$withval" ],
				[ VERSION_PATCH="$DEFAULT_VERSION_PATCH" ])
AC_ARG_WITH(release-snap,    AS_HELP_STRING([--with-release-snap=SNAP],
				  [Release snap number reported by the server]),
				[ VERSION_SNAP="$withval" ],
				[ VERSION_SNAP="$DEFAULT_VERSION_SNAP" ])
AC_ARG_WITH(release-date,    AS_HELP_STRING([--with-release-date=DATE],
				  [Release date reported by the server]),
				[ RELEASE_DATE="$withval" ],
				[ RELEASE_DATE="$DEFAULT_RELEASE_DATE" ])
AC_ARG_WITH(vendor-web,      AS_HELP_STRING([--with-vendor-web=URL],
				  [Vendor web address reported by the server]),
				[ VENDOR_WEB="$withval" ],
				[ VENDOR_WEB="$DEFAULT_VENDOR_WEB" ])
AC_ARG_WITH(os-name,         AS_HELP_STRING([--with-os-name=OSNAME], [Name of OS (default: UNKNOWN)]),
				[ OSNAME="$withval" ],
				[ OSNAME="UNKNOWN" ])
AC_ARG_WITH(os-vendor,       AS_HELP_STRING([--with-os-vendor=OSVENDOR], [Name of OS vendor]),
				[ OSVENDOR="$withval" ],
				[ OSVENDOR="" ])
#AC_ARG_ENABLE(null-root-cursor, AS_HELP_STRING([--enable-null-root-cursor], [Use an empty root cursor (default: use core cursor)]),
#                                 [NULL_ROOT_CURSOR=$enableval],
#                                 [NULL_ROOT_CURSOR=no])
NULL_ROOT_CURSOR=no


font_dir_found=no

DEFAULT_FONT_DIR="/usr/share/fonts/X11"
AC_MSG_CHECKING([For fonts in /usr/share/fonts/X11])
AC_CHECK_FILE($DEFAULT_FONT_DIR/misc, font_dir_found=yes,)

if test x$font_dir_found = "xno"; then
  AC_MSG_CHECKING([For fonts in /usr/share/fonts])
  DEFAULT_FONT_DIR="/usr/share/fonts"
  AC_CHECK_FILE($DEFAULT_FONT_DIR/misc, font_dir_found=yes,)
fi

if test x$font_dir_found = "xno"; then
  AC_MSG_CHECKING([For fonts in /usr/lib/X11/fonts])
  DEFAULT_FONT_DIR="/usr/lib/X11/fonts"
  AC_CHECK_FILE($DEFAULT_FONT_DIR/misc, font_dir_found=yes,)
fi

DEFAULT_FONT_ENCODINGS="$DEFAULT_FONT_DIR/encodings/encodings.dir"
DEFAULT_FONT_PATH="$DEFAULT_FONT_DIR/misc,$DEFAULT_FONT_DIR/75dpi,$DEFAULT_FONT_DIR/100dpi"
font_dir_set=no

AC_ARG_WITH(fontdir,         AS_HELP_STRING([--with-fontdir=DIR], [Path to top level dir where fonts are installed (default: /usr/lib/X11/fonts/ or /usr/share/fonts or /usr/share/fonts/X11)
set fontpath (misc,75dpi,100dpi) and fontenc-file]),
                                [ FONTDIR="$withval"; font_dir_set="yes"],
                                [ FONTDIR="$DEFAULT_FONT_DIR"])

AC_ARG_WITH(fontpath,        AS_HELP_STRING([--with-fontpath=PATH],[Path to fonts are installed (default: fontdir/misc,fontdir/75dpi,fontdir/100dpi)]),
				[FONTPATH="$withval"],
				[FONTPATH="$DEFAULT_FONT_PATH"])

AC_ARG_WITH(fontenc-file,     AS_HELP_STRING([--with-fontenc-file=FILE],[Path to fonts encoding file (default: fontdir/encodings/encodings.dir)]),
				[FONT_ENCODINGS="$withval"],
				[FONT_ENCODINGS="$DEFAULT_FONT_DIR/encodings/encodings.dir"])

if test x$font_dir_set = "xyes"; then
	FONTPATH="$FONTDIR/misc,$FONTDIR/75dpi,$FONTDIR/100dpi"
fi

BUILTIN_FONTS=no

#AC_ARG_ENABLE(builtinrgb,
#   [  --disable-builtinrgb  disable builtin rgb table],
#   [USE_RGB_BUILTIN=$enableval],[USE_RGB_BUILTIN=yes])
USE_RGB_BUILTIN=yes

dnl Extensions.
#AC_ARG_ENABLE(composite,      AS_HELP_STRING([--disable-composite], [Build Composite extension (default: enabled)]), [COMPOSITE=$enableval], [COMPOSITE=yes])
COMPOSITE=no
#AC_ARG_ENABLE(mitshm,         AS_HELP_STRING([--disable-shm], [Build SHM extension (default: enabled)]), [MITSHM=$enableval], [MITSHM=yes])
MITSHM=yes
#AC_ARG_ENABLE(xres,           AS_HELP_STRING([--disable-xres], [Build XRes extension (default: enabled)]), [RES=$enableval], [RES=yes])
RES=no
# FIXME!
#AC_ARG_ENABLE(xtrap,          AS_HELP_STRING([--disable-xtrap], [Build XTrap extension (default: enabled)]), [XTRAP=$enableval], [XTRAP=yes])
XTRAP=no
AC_ARG_ENABLE(record,         AS_HELP_STRING([--disable-record], [Build Record extension (default: enabled)]), [RECORD=$enableval], [RECORD=yes])
#AC_ARG_ENABLE(xv,             AS_HELP_STRING([--disable-xv], [Build Xv extension (default: enabled)]), [XV=$enableval], [XV=yes])
XV=no
#AC_ARG_ENABLE(screensaver,    AS_HELP_STRING([--disable-screensaver], [Build ScreenSaver extension (default: enabled)]), [SCREENSAVER=$enableval], [SCREENSAVER=yes])
SCREENSAVER=no
AC_ARG_ENABLE(glx,   AS_HELP_STRING([--enable-glx], [enable GLX extension (default: disabled)]), [GLXX=$enableval],[GLXX=no])
AC_ARG_ENABLE(glx-x86,  AS_HELP_STRING([--enable-glx-x86], [enable GLX extension with x86 optimization (default: disabled)]),[GLX_X86=$enableval ; GLXX=$enableval],[GLX_X86=no])
#AC_ARG_ENABLE(xinerama,	      AS_HELP_STRING([--disable-xinerama], [Build Xinerama extension (default: enabled)]), [XINERAMA=$enableval], [XINERAMA=yes])
XINERAMA=no
#AC_ARG_ENABLE(xace,     AS_HELP_STRING([--disable-xace], [Build X-ACE extension (default: enabled)]), [XACE=$enableval], [XACE=yes])
XACE=yno
#AC_ARG_ENABLE(xcsecurity,     AS_HELP_STRING([--disable-xcsecurity], [Build Security extension (default: enabled)]), [XCSECURITY=$enableval], [XCSECURITY=$XACE])
XCSECURITY=no
#AC_ARG_ENABLE(appgroup,       AS_HELP_STRING([--disable-appgroup], [Build XC-APPGROUP extension (default: enabled)]), [APPGROUP=$enableval], [APPGROUP=$XCSECURITY])
APPGROUP=no
#AC_ARG_ENABLE(xcalibrate,     AS_HELP_STRING([--enable-xcalibrate], [Build XCalibrate extension (default: disabled)]), [XCALIBRATE=$enableval], [XCALIBRATE=no])
XCALIBRATE=yes
#AC_ARG_ENABLE(xevie,          AS_HELP_STRING([--disable-xevie], [Build XEvIE extension (default: enabled)]), [XEVIE=$enableval], [XEVIE=yes])
XEVIE=no
#AC_ARG_ENABLE(evi,            AS_HELP_STRING([--disable-evi], [Build Extended-Visual-Information extension (default: enabled)]), [EVI=$enableval], [EVI=yes])
EVI=no
#AC_ARG_ENABLE(multibuffer,    AS_HELP_STRING([--enable-multibuffer], [Build Multibuffer extension (default: disabled)]), [MULTIBUFFER=$enableval], [MULTIBUFFER=no])
MULTIBUFFER=no
#AC_ARG_ENABLE(fontcache,      AS_HELP_STRING([--enable-fontcache], [Build FontCache extension (default: disabled)]), [FONTCACHE=$enableval], [FONTCACHE=no])
FONTCACHE=no
#AC_ARG_ENABLE(dbe,            AS_HELP_STRING([--disable-dbe], [Build DBE extension (default: enabled)]), [DBE=$enableval], [DBE=yes])
DBE=yes

AM_CONDITIONAL(XV, [test "x$XV" = xyes])
if test "x$XV" = xyes; then
	AC_DEFINE(XV, 1, [Support Xv extension])
	AC_DEFINE(XvExtension, 1, [Build Xv extension])
	REQUIRED_MODULES="$REQUIRED_MODULES videoproto"
else
	XVMC=no
fi

AM_CONDITIONAL(XVMC, [test "x$XVMC" = xyes])
if test "x$XVMC" = xyes; then
	AC_DEFINE(XvMCExtension, 1, [Build XvMC extension])
fi


AM_CONDITIONAL(COMPOSITE, [test "x$COMPOSITE" = xyes])
if test "x$COMPOSITE" = xyes; then
	AC_DEFINE(COMPOSITE, 1, [Support Composite Extension])
	REQUIRED_MODULES="$REQUIRED_MODULES [compositeproto >= 0.3]"
	COMPOSITE_LIB='$(top_builddir)/xserver/composite/libcomposite.a'
	COMPOSITE_INC='-I$(top_srcdir)/xserver/composite'
fi

AM_CONDITIONAL(MITSHM, [test "x$MITSHM" = xyes])
if test "x$MITSHM" = xyes; then
	AC_DEFINE(MITSHM, 1, [Support MIT-SHM extension])
	AC_DEFINE(HAS_SHM, 1, [Support SHM])
fi

AM_CONDITIONAL(XTRAP, [test "x$XTRAP" = xyes])
if test "x$XTRAP" = xyes; then
	AC_DEFINE(XTRAP, 1, [Support XTrap extension])
	REQUIRED_MODULES="$REQUIRED_MODULES trapproto"
	XTRAP_LIB='$(top_builddir)/xserver/XTrap/libxtrap.a'
fi

AM_CONDITIONAL(RECORD, [test "x$RECORD" = xyes])
if test "x$RECORD" = xyes; then
	AC_DEFINE(XRECORD, 1, [Support Record extension])
	REQUIRED_MODULES="$REQUIRED_MODULES recordproto"
	RECORD_LIB='$(top_builddir)/xserver/record/librecord.a'
fi

AM_CONDITIONAL(SCREENSAVER, [test "x$SCREENSAVER" = xyes])
if test "x$SCREENSAVER" = xyes; then
	AC_DEFINE(SCREENSAVER, 1, [Support MIT-SCREEN-SAVER extension])
fi

AM_CONDITIONAL(RES, [test "x$RES" = xyes])
if test "x$RES" = xyes; then
	AC_DEFINE(RES, 1, [Support X resource extension])
	REQUIRED_MODULES="$REQUIRED_MODULES resourceproto"
fi


with_glx=no
problem_glx="disabled by default try --enable-glx or --enable-glx-x86"
if test "$GLXX" = yes; then
  GLX=yes
  with_glx=yes
  problem_glx="(without x86 optimisation)"
fi

AM_CONDITIONAL(GLX, test x$GLX = xyes)
if test "$GLX" = yes; then
  AC_DEFINE(GLXEXT,1,[Build GLX extension])
fi

AM_CONDITIONAL(GLX_X86, test x$GLX_X86 = xyes)
if test "$GLX_X86" = yes; then
  AC_DEFINE(GLX_X86,1,[Build GLX extension with x86 optimisation])
  problem_glx="(x86 optimisation)"
fi

AM_CONDITIONAL(XINERAMA, [test "x$XINERAMA" = xyes])
if test "x$XINERAMA" = xyes; then
	AC_DEFINE(XINERAMA, 1, [Support Xinerama extension])
	AC_DEFINE(PANORAMIX, 1, [Internal define for Xinerama])
	REQUIRED_MODULES="$REQUIRED_MODULES xineramaproto"
fi

AM_CONDITIONAL(XACE, [test "x$XACE" = xyes])
if test "x$XACE" = xyes; then
	AC_DEFINE(XACE, 1, [Build X-ACE extension])
fi

AM_CONDITIONAL(XCSECURITY, [test "x$XCSECURITY" = xyes])
if test "x$XCSECURITY" = xyes; then
	if test "x$XACE" != xyes; then
		AC_MSG_ERROR([cannot build Security extension without X-ACE])
	fi
	AC_DEFINE(XCSECURITY, 1, [Build Security extension])
fi

AM_CONDITIONAL(XEVIE, [test "x$XEVIE" = xyes])
if test "x$XEVIE" = xyes; then
	AC_DEFINE(XEVIE, 1, [Build XEvIE extension])
	REQUIRED_MODULES="$REQUIRED_MODULES evieproto"
fi

AM_CONDITIONAL(APPGROUP, [test "x$APPGROUP" = xyes])
if test "x$APPGROUP" = xyes; then
	if test "x$XACE" != xyes || test "x$XCSECURITY" != xyes; then
		AC_MSG_ERROR([cannot build APPGROUP extension without X-ACE and XC-SECURITY])
	fi
	AC_DEFINE(XAPPGROUP, 1, [Build APPGROUP extension])
fi

AM_CONDITIONAL(EVI, [test "x$EVI" = xyes])
if test "x$EVI" = xyes; then
	AC_DEFINE(EVI, 1, [Build Extended-Visual-Information extension])
	# Requires xextproto which is always required
fi

AM_CONDITIONAL(MULTIBUFFER, [test "x$MULTIBUFFER" = xyes])
if test "x$MULTIBUFFER" = xyes; then
	AC_DEFINE(MULTIBUFFER, 1, [Build Multibuffer extension])
	# Requires xextproto which is always required
fi

AM_CONDITIONAL(FONTCACHE, [test "x$FONTCACHE" = xyes])
if test "x$FONTCACHE" = xyes; then
	AC_DEFINE(FONTCACHE, 1, [Build FontCache extension])
	REQUIRED_MODULES="$REQUIRED_MODULES fontcacheproto"
fi

AM_CONDITIONAL(DBE, [test "x$DBE" = xyes])
if test "x$DBE" = xyes; then
	AC_DEFINE(DBE, 1, [Support DBE extension])
	DBE_LIB='$(top_builddir)/xserver/dbe/libdbe.a'
fi

if test "x$BUILTIN_FONTS" = xyes; then
       AC_DEFINE(BUILTIN_FONTS, 1, [Use only built-in fonts])
       AC_DEFINE(NOFONTSERVERACCESS, 1, [Avoid using a font server])
fi

if test "x$XCALIBRATE" = xyes && test "$KDRIVE" = yes; then
   AC_DEFINE(XCALIBRATE, 1, [Build XCalibrate extension])
   REQUIRED_MODULES="$REQUIRED_MODULES xcalibrateproto"
else
   XCALIBRATE=no
fi
AM_CONDITIONAL(XCALIBRATE, [test "x$XCALIBRATE" = xyes])

AC_DEFINE(RENDER,1,[Support RENDER extension])
EXTENSION_LIBS="$EXTENSION_LIBS "'$(top_builddir)/xserver/render/librender.a'
EXTENSION_INCS="$EXTENSION_INCS "'-I$(top_srcdir)/xserver/render'

#AC_DEFINE(RANDR,1,[Support RANDR extension])
#EXTENSION_LIBS="$EXTENSION_LIBS "'$(top_builddir)/xserver/randr/librandr.a'
#EXTENSION_INCS="$EXTENSION_INCS "-I'$(top_srcdir)/xserver/randr'

AC_DEFINE(XFIXES,1,[Support XFixes extension])
FIXES_LIB='$(top_builddir)/xserver/xfixes/libxfixes.a'
FIXES_INC='-I$(top_srcdir)/xserver/xfixes'

#AC_DEFINE(DAMAGE,1,[Support Damage extension])
#DAMAGE_LIB='$(top_builddir)/damageext/libdamageext.la'
#DAMAGE_INC='-I$(top_srcdir)/damageext'
#MIEXT_DAMAGE_LIB='$(top_builddir)/miext/damage/libdamage.la'
#dnl damage (may) need the composite wrapper when composite is enabled
#if test "$COMPOSITE" = yes; then
#	MIEXT_DAMAGE_LIB="$MIEXT_DAMAGE_LIB "'$(top_builddir)/miext/cw/libcw.la'
#fi
#MIEXT_DAMAGE_INC='-I$(top_srcdir)/miext/damage'

#AC_DEFINE(XINPUT, 1, [Support X Input extension])
#XI_LIB='$(top_builddir)/Xi/libXi.la'
#XI_INC='-I$(top_srcdir)/Xi'

AC_DEFINE(SHAPE, 1, [Support SHAPE extension])

AC_CHECK_FUNC(strcasecmp, [], AC_DEFINE([NEED_STRCASECMP], 1,
                                        [Do not have `strcasecmp'.]))

if test "x$NULL_ROOT_CURSOR" = xyes; then
        AC_DEFINE(NULL_ROOT_CURSOR, 1, [Use an empty root cursor])
fi

AM_CONDITIONAL(USE_RGB_BUILTIN, [test "x$USE_RGB_BUILTIN" = xyes])
if test "x$USE_RGB_BUILTIN" = xyes; then
	AC_DEFINE(USE_RGB_BUILTIN, 1, [Use built-in RGB color database])
fi

VENDOR_RELEASE="((($VERSION_MAJOR) * 10000000) + (($VERSION_MINOR) * 100000) + (($VERSION_PATCH) * 1000) + $VERSION_SNAP)"

if test $VERSION_SNAP = "0"; then
	if test $VERSION_PATCH = "0"; then
		VENDOR_VERSION_STRING="${VERSION_MAJOR}.${VERSION_MINOR}"
	else
		VENDOR_VERSION_STRING="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
	fi
else
	VENDOR_VERSION_STRING="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_SNAP}"
fi

VENDOR_RELEASE_STRING="Release ${VENDOR_VERSION_STRING}"
VENDOR_MAN_VERSION="Version ${VENDOR_VERSION_STRING}"

AC_DEFUN([AC_DEFINE_DIR], [
  prefix_NONE=
  exec_prefix_NONE=
  test "x$prefix" = xNONE && prefix_NONE=yes && prefix=$ac_default_prefix
  test "x$exec_prefix" = xNONE && exec_prefix_NONE=yes && exec_prefix=$prefix
dnl In Autoconf 2.60, ${datadir} refers to ${datarootdir}, which in turn
dnl refers to ${prefix}.  Thus we have to use `eval' twice.
  eval ac_define_dir="\"[$]$2\""
  eval ac_define_dir="\"$ac_define_dir\""
  AC_SUBST($1, "$ac_define_dir")
  AC_DEFINE_UNQUOTED($1, "$ac_define_dir", [$3])
  test "$prefix_NONE" && prefix=NONE
  test "$exec_prefix_NONE" && exec_prefix=NONE
])

AC_DEFINE_DIR(COMPILEDDEFAULTFONTPATH, FONTPATH, [Default font path])
AC_DEFINE_DIR(RGB_DB, RGBPATH, [Default RGB path])
AC_DEFINE_DIR(BASE_FONT_PATH, FONTDIR, [Default base font path])
AC_DEFINE_DIR(DRI_DRIVER_PATH, DRI_DRIVER_PATH, [Default DRI driver path])
AC_DEFINE_UNQUOTED(XVENDORNAME, ["$VENDOR_STRING"], [Vendor name])
AC_DEFINE_UNQUOTED(XVENDORNAMESHORT, ["$VENDOR_STRING_SHORT"], [Short vendor name])
AC_DEFINE_UNQUOTED(METISSE_RELEASE, ["$VENDOR_RELEASE_STRING"], [Vendor release])
AC_DEFINE_UNQUOTED(METISSE_DATE, ["$RELEASE_DATE"], [Vendor release])
#AC_DEFINE_UNQUOTED(XORG_MAN_VERSION, ["$VENDOR_MAN_VERSION"], [Vendor man version])
AC_DEFINE_UNQUOTED(BUILDERADDR, ["$BUILDERADDR"], [Builder address])
AC_DEFINE_UNQUOTED(OSNAME, ["$OSNAME"], [Operating System Name])
AC_DEFINE_UNQUOTED(OSVENDOR, ["$OSVENDOR"], [Operating System Vendor])
AC_SUBST([VENDOR_STRING])
AC_SUBST([VENDOR_STRING_SHORT])
AC_SUBST([VENDOR_RELEASE])
#AC_SUBST([VENDOR_MAN_VERSION])

AC_DEFINE_UNQUOTED(FONT_ENCODINGS, ["$FONT_ENCODINGS"], [Default font encodings dir file])

AC_SUBST(FONT_ENCODINGS)


AC_DEFINE(DDXOSINIT,1,[Use OsVendorInit])
AC_DEFINE(SERVER_LOCK,1,[Use a lock to prevent multiple servers on a display])
AC_DEFINE(SMART_SCHEDULE,1,[Include time-based scheduler])
AC_DEFINE(NO_LIBCWRAPPER, 1, [Define to 1 if modules should avoid the libcwrapper])

if test "x$DEBUGGING" = xyes; then
       AC_DEFINE(DEBUG, 1, [Enable debugging code])
else
       AC_DEFINE(NDEBUG, 1, [Disable some debugging code])
fi

AC_DEFINE(USE_RGB_TXT,1,[Use rgb.txt directly])
AC_DEFINE(MITMISC,1,[Support MIT Misc extension])
AC_DEFINE(XTEST,1,[Support XTest extension])
AC_DEFINE(XSYNC,1,[Support XSync extension])
AC_DEFINE(XCMISC,1,[Support XCMisc extension])
AC_DEFINE(BIGREQS,1,[Support BigRequests extension])
AC_DEFINE(PIXPRIV,1,[Support pixmap privates])

# The Xorg binary needs to export symbols so that they can be used from modules
# Some platforms require extra flags to do this.   gcc should set these flags
# when -rdynamic is passed to it, other compilers/linkers may need to be added
# here.
if test "x$GCC" = "xyes"; then
	CFLAGS="-Wall $CFLAGS"
       	if test "x$WERROR" = "xyes"; then
        	  CFLAGS="-Werror $CFLAGS"
       	fi	
       	LD_EXPORT_SYMBOLS_FLAG="-rdynamic"
fi

AC_CHECK_FUNCS([clock_gettime], [have_clock_gettime=yes],
               [AC_CHECK_LIB([rt], [clock_gettime], [have_clock_gettime=-lrt],
                             [have_clock_gettime=no])])

AC_MSG_CHECKING([for a useful monotonic clock ...])

if ! test "x$have_clock_gettime" = xno; then
    if ! test "x$have_clock_gettime" = xyes; then
        CLOCK_LIBS="$have_clock_gettime"
    else
        CLOCK_LIBS=""
    fi

    AC_RUN_IFELSE([
#define _POSIX_C_SOURCE 199309L
#include <time.h>

int main(int argc, char *argv[]) {
    struct timespec tp;

    if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
        return 0;
    else
        return 1;
}
    ], [MONOTONIC_CLOCK=yes], [MONOTONIC_CLOCK=no],
       [MONOTONIC_CLOCK="cross compiling"])
else
    MONOTONIC_CLOCK=no
fi

AC_MSG_RESULT([$MONOTONIC_CLOCK])

if test "x$MONOTONIC_CLOCK" = xyes; then
    AC_DEFINE(MONOTONIC_CLOCK, 1, [Have monotonic clock from clock_gettime()])
    XSERVER_LIBS="$XSERVER_LIBS $CLOCK_LIBS"
    LIBS="$LIBS $CLOCK_LIBS"
fi

dnl Imake defines SVR4 on SVR4 systems, and many files check for it, so
dnl we need to replicate that here until those can all be fixed
AC_MSG_CHECKING([if SVR4 needs to be defined])
AC_EGREP_CPP([I_AM_SVR4],[
#if defined(SVR4) || defined(__svr4__) || defined(__SVR4)
 I_AM_SVR4
#endif
],[
AC_DEFINE([SVR4],1,[Define to 1 on systems derived from System V Release 4])
AC_MSG_RESULT([yes])], AC_MSG_RESULT([no]))

#
# YES :o)
#

AC_DEFINE(METISSE_SERVER,1,[We build the Xmetisse server])

# bad, only for debugging
#AC_DEFINE(USE_POINTER_LIMITS,1, [Confine pointer events on screen])


#
# dnl Xfont stuf
#

AC_CHECK_HEADERS([endian.h])

AC_ARG_ENABLE(freetype,
  AS_HELP_STRING([--disable-freetype], [disable support for FreeType fonts in libXfont]),
  [XFONT_FREETYPE=$enableval],[XFONT_FREETYPE=yes])

AM_CONDITIONAL(XFONT_FREETYPE, [test x$XFONT_FREETYPE = xyes])
if test x$XFONT_FREETYPE = xyes; then
  AC_DEFINE(XFONT_FREETYPE,1,[Xfont Support FreeType fonts])
  AC_ARG_WITH(freetype-config,
    AS_HELP_STRING([--with-freetype-config=PROG],[Use FreeType configuration program PROG]),
    freetype_config=$withval, freetype_config=yes)
	
    if test "$freetype_config" = "yes"; then 
      AC_PATH_PROG(ft_config,freetype-config,no)
      if test "$ft_config" = "no"; then
        AC_MSG_ERROR(
          [You must have freetype installed; see http://www.freetype.org/])
      fi
    else
      ft_config="$freetype_config"
    fi
      FREETYPE_CFLAGS="`$ft_config --cflags`"
      FREETYPE_LIBS="`$ft_config --libs`"
else
  FREETYPE_CFLAGS=""
  FREETYPE_LIBS=""
fi
AC_SUBST(FREETYPE_LIBS)
AC_SUBST(FREETYPE_CFLAGS)
#AC_SUBST(XFONT_FREETYPE)

# FIXME!
#AC_ARG_ENABLE(fc,
#  [  --enable-fc  enable support for the X Font Services Protocol],
#  [XFONT_FC=$enableval],[XFONT_FC=no])
XFONT_FC=no
AM_CONDITIONAL(XFONT_FC, [test x$XFONT_FC = xyes])
if test x$XFONT_FC = xyes; then
	AC_DEFINE(XFONT_FC,1,[Xfont Support the X Font Services Protocol])
fi
#AC_SUBST(XFONT_FC)

#AC_ARG_ENABLE(builtins, [  --disable-builtins ], [XFONT_BUILTINS=$enableval], [XFONT_BUILTINS=yes])
XFONT_BUILTINS=yes
AM_CONDITIONAL(XFONT_BUILTINS, [test "x$XFONT_BUILTINS" = xyes ])
if test "x$XFONT_BUILTINS" = xyes; then
  AC_DEFINE(XFONT_BUILTINS,1,[Xfont Support built-in fonts])
fi
#AC_SUBST(XFONT_BUILTINS)

#AC_ARG_ENABLE(bitmap, [  --disable-bitmap ], [XFONT_BITMAP=$enableval], [XFONT_BITMAP=yes])
XFONT_BITMAP=yes
AM_CONDITIONAL(XFONT_BITMAP, [test "x$XFONT_BITMAP" = xyes ])
if test "x$XFONT_BITMAP" = xyes; then
  AC_DEFINE(XFONT_BITMAP,1,[Xfont Support bitmap font files])
fi
#AC_SUBST(XFONT_BITMAP)

if test "x$XFONT_BULITINS" = xyes -o "x$XFONT_BITMAP" = xyes -o "x$XFONT_FREETYPE" = xyes; then
  XFONT_FONTFILE=yes
else
  XFONT_FONTFILE=no
fi
AM_CONDITIONAL(XFONT_FONTFILE, [test "x$XFONT_FONTFILE" = xyes])
if test "x$XFONT_FONTFILE" = xyes; then
  AC_DEFINE(XFONT_FONTFILE,,[Xfont Support fonts in files])
fi
#AC_SUBST(XFONT_FONTFILE)

dnl FIXME!
AC_CHECK_LIB(z, gzopen, [Z_LIBS=-lz
AC_SUBST(Z_LIBS)], AC_MSG_ERROR([*** zlib is required]))

AC_DEFINE(PCFFORMAT,,[Xfont Support PCF format fonts])
AC_DEFINE(X_GZIP_FONT_COMPRESSION,,[Xfont Support gzip fonts])

# PKG_CHECK_MODULES(XFONT, xproto)

dnl
dnl xdmcp stuff
dnl

HASXDMAUTH=no
AM_CONDITIONAL(HASXDMAUTH,test x$HASXDMAUTH = xyes)
# AC_SUBST(HASXDMAUTH)

dnl
dnl Xau stuff
dnl
kerberos=false
AM_CONDITIONAL(KERBEROS, test x$kerberos = xtrue)
# AC_SUBST(KERBEROS)

dnl
dnl 
dnl
AUXLIBS='-I${top_srcdir}/xserver/include -I${top_srcdir}/xlibs/Xfont/include/X11/fonts/ -I${top_srcdir}/xlibs/Xau -I${top_srcdir}/xserver/include/X11/Xtrans '

XSERVER_CFLAGS="${AUXLIBS} ${XSERVER_CFLAGS}"

dnl xtrans is needed for
dnl ${top_srcdir}/xlibs/xtrans/libxtrans.a ?

AUXLIBS='${top_builddir}/xlibs/Xfont/libXfont.la ${top_builddir}/xlibs/Xau/libXau.a'
XSERVER_LIBS="${XSERVER_LIBS} ${AUXLIBS} ${DBE_LIB} ${RENDER_LIB} ${FIXES_LIB} ${RECORD_LIB}"

AC_SUBST(XSERVER_LIBS)
AC_SUBST(XSERVER_CFLAGS)

dnl
dnl end of xserver and xlibs
dnl

dnl
dnl libmetisse stuff
dnl

LIBMETISSE_INCLUDEDIR='${includedir}'/libmetisse
LIBMETISSE_VERSION=1.0.0
LT_LIBMETISSE_VERSION=1:0:0

AC_SUBST(LIBMETISSE_INCLUDEDIR)
AC_SUBST(LIBMETISSE_VERSION)
AC_SUBST(LT_LIBMETISSE_VERSION)

dnl
dnl FvwmCompositor stuff
dnl

with_FvwmCompositor=yes
FvwmCompositor_failure=
NUCLEO_CPPFLAGS=
NUCLEO_LDFLAGS=
NUCLEO_LIBS=
FVWMCOMPOSITOR_DIR=
HAVE_FVWMCOMPOSITOR=0

dnl Need to know where X is, for finding some libraries
no_x=""
AC_PATH_XTRA

if test x"$no_x" = x"yes"; then
  with_FvwmCompositor=no
  FvwmCompositor_failure="X11 libraries or header files could not be found"
fi

if test x"$with_FvwmCompositor" = x"yes"; then
  AC_CHECK_PROG(NUCLEO_CONFIG, nucleo-config, yes, no)
  if test $NUCLEO_CONFIG = no; then
    with_FvwmCompositor=no
    FvwmCompositor_failure="nucleo-config could not be found"
  fi
fi

dnl
dnl Xcursor
dnl
problem_xcursor=
with_xcursor=
Xcursor_LIBS=

if test x"$with_FvwmCompositor" = x"yes"; then
  AC_CHECK_LIB(
    Xcursor, XcursorSetTheme,
    with_xcursor=yes; Xcursor_LIBS="-lXcursor";
    AC_DEFINE(HAVE_XCURSOR,1,[Support Xcursor library]),
    with_xcursor=no; problem_xcursor=": Can't detect Xcursor library",
    [$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS])
fi
AC_SUBST(Xcursor_LIBS)

# -----------------------------------------------------------------
# wmtrace
wmtrace_failure=
with_wmtrace=check
HAVE_WMTRACE=0
WMTRACE_SUBDIR=

AC_ARG_ENABLE(wmtrace,
  [  --disable-wmtrace       disable wmtrace support],
  [ if test x"$enableval" = xyes; then
    with_wmtrace="check"
  else
    with_wmtrace="no"
    wmtrace_failure="explicitly disabled"
  fi ],
  [ with_wmtrace="check" ]
)

if test x$with_wmtrace = xcheck ; then
  AC_MSG_CHECKING(for wmtrace in $srcdir)
  if test -x $srcdir/wmtrace/configure; then
      AC_MSG_RESULT(found)
      with_wmtrace=yes
      # should check for wmtrace needed lib ...
      PKG_CHECK_MODULES(M_GLIB, glib-2.0, have_glib=yes, have_glib=no)
      if test x$with_wmtrace = xyes && test x$have_glib = xno ; then
	  AC_MSG_WARN([wmtrace: GLib-2 development libraries not found for wmtrace])
	  with_wmtrace=no
          wmtrace_failure="GLib/GTK-2 development libraries not found"
      fi
      PKG_CHECK_MODULES(GTK, gtk+-2.0, have_gtk=yes, have_gtk=no)
      if test x$with_wmtrace = xyes && test x$have_gtk = xno ; then
	  AC_MSG_WARN([wmtrace: GTK+-2 development libraries not found for wmtrace])
	  with_wmtrace=no
          wmtrace_failure="GTK+-2 development libraries not found"
      fi
      if test x$with_wmtrace = xyes ; then
        with_wmtrace=yes
        AC_DEFINE(HAVE_WMTRACE,1,[wmtrace Support])
        HAVE_WMTRACE=1
        WMTRACE_SUBDIR=wmtrace
        dnl Should use a "literal" for ./configure --help=recursive
        AC_CONFIG_SUBDIRS(wmtrace)
      fi
  else
      dnl cannot happen with the dist (only with the cvs!)
      AC_MSG_RESULT(not found)
      with_wmtrace=no
      wmtrace_failure="wmtrace source code not found"
  fi
fi

AC_SUBST(HAVE_WMTRACE)
AC_SUBST(WMTRACE_SUBDIR)
AM_CONDITIONAL(HAVE_WMTRACE,test x$HAVE_WMTRACE = x1)

dnl --------------------------------------------------------------------

FVWM_CPPFLAGS=
FVWM_LDFLAGS=
FVWM_MODULEDIR=
HAVE_FVWM=0
FVWM_VERSION=2.5.20
with_fvwm=no

# expand libexecdir
test "x$prefix" = xNONE && prefix="$ac_default_prefix"
test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
eval LIBEXECDIR="`eval echo ${libexecdir}`"
LIBEXECDIR="${LIBEXECDIR}"

if test x"$with_FvwmCompositor" = x"yes"; then
  AC_MSG_CHECKING(for fvwm-insitu in $srcdir)
  if test -d $srcdir/fvwm-insitu; then
    AC_MSG_RESULT(found)

    dnl Should use a "literal" for ./configure --help=recursive
    AC_CONFIG_SUBDIRS(fvwm-insitu)
  else
    dnl cannot happen with the dist (only with the cvs!)
    AC_MSG_RESULT(not found)
    echo
    echo
    echo "-- You should get fvwm-insitu cvs code! and install a symbolic link"
    echo "-- fvwm-insitu in this directory to this source code"
    echo
    echo
    with_FvwmCompositor=no
    FvwmCompositor_failure="fvwm-insitu source code not found"
  fi
fi

if test x"$with_FvwmCompositor" = x"yes"; then
  # compositor
  NUCLEO_CPPFLAGS="`nucleo-config --cppflags`"
  NUCLEO_LDFLAGS="`nucleo-config --ldflags`"
  NUCLEO_LIBS="`nucleo-config --libs`"
  FVWMCOMPOSITOR_DIR=FvwmCompositor
  HAVE_FVWMCOMPOSITOR=1
  # fvwm
  AC_DEFINE(HAVE_FVWM,1,[FVWM Support for FvwmCompositor])
  HAVE_FVWM=1
  with_fvwm=yes
  FVWM_CPPFLAGS='-I${top_srcdir}'"/fvwm-insitu"
  FVWM_LDFLAGS='${top_builddir}/'"fvwm-insitu/libs/libfvwm.a"
  FVWM_MODULEDIR='${libexecdir}/'"fvwm-insitu/$FVWM_VERSION"
  FVWM_MODULEDIR_EXPANDED="${LIBEXECDIR}/fvwm-insitu/$FVWM_VERSION" 
fi


AC_SUBST(NUCLEO_CPPFLAGS)
AC_SUBST(NUCLEO_LDFLAGS)
AC_SUBST(NUCLEO_LIBS)
AC_SUBST(FVWMCOMPOSITOR_DIR)
AC_SUBST(HAVE_FVWMCOMPOSITOR)

AM_CONDITIONAL(HAVE_FVWMCOMPOSITOR,test x$HAVE_FVWMCOMPOSITOR = x1)

AC_SUBST(FVWM_CPPFLAGS)
AC_SUBST(FVWM_LDFLAGS)
AC_SUBST(FVWM_MODULEDIR)
AC_SUBST(FVWM_MODULEDIR_EXPANDED)
AC_SUBST(FVWM_VERSION)
AC_SUBST(HAVE_FVWM)

AM_CONDITIONAL(HAVE_FVWM,test x$HAVE_FVWM = x1)

# ------------------------------------
# fvwm metisse images and config

METISSE_FVWM_CONFIG_DIR='${pkgdatadir}'/fvwm
METISSE_FVWM_IMAGES_DIR='${pkgdatadir}'/fvwm/images
FVWM2RC_REVISION="7"
# 3 -> 4 (bindings)
# 4 -> 5 (styles, complex fp policy; bindings-functions, bindings,
# ametsitarc.in; some fvwm icons config stuff)
# 5 -> 6 main change
# 6 -> 8 a lot of things ...
# 8 -> 9 a lot of things ...
# 9 -> 10 a lot of things ... removing extra (read it in metisserc silently)
METISSERC_REVISION="10"

AC_SUBST(METISSE_FVWM_IMAGES_DIR)
AC_SUBST(METISSE_FVWM_CONFIG_DIR)
AC_SUBST(FVWM2RC_REVISION)
AC_SUBST(METISSERC_REVISION)

# ------------------------------------
# shading

METISSE_SHADERS_DIR='${pkgdatadir}'/shaders
AC_SUBST(METISSE_SHADERS_DIR)

# ------------------------------------
#
problem_widget_support=""
with_widget_support="check"

AC_ARG_ENABLE(spi,
  [  --disable-spi           disable widget tracking support],
  [ if test x"$enableval" = xyes; then
    with_widget_support="check"
  else
    with_widget_support="no"
    problem_widget_support=": Explicitly disabled"
  fi ],
  [ problem_widget_support=="check" ]
)

HAVE_SPI=0

if test x"$with_widget_support" = "xcheck" ; then
  problem_widget_support="csip not found"
  #glib-2.0 ORBit-2.0 bonobo-activation-2.0 libbonobo-2.0.pc
  #atk gtk+-2.0 libspi-1.0.pc
  # at-spi-1.5.4
  with_widget_support=no
  PKG_CHECK_MODULES(SPI, cspi-1.0, have_spi=yes, have_spi=no)
  if test x$have_spi = xno ; then
    AC_MSG_WARN([libspi not found!])
  else
    HAVE_SPI=1
    AC_DEFINE(HAVE_SPI,1,[cspi support for widget tracking])
    with_widget_support=yes
    problem_widget_support=
  fi
fi

AC_SUBST(HAVE_SPI)

AM_CONDITIONAL(HAVE_SPI,test x$HAVE_SPI = x1)

AC_SUBST(SPI_CLAGS)
AC_SUBST(SPI_LIBS)

# ------------------------------------
METISSE_DOC_DIR='${datadir}'/metisse/doc
AC_SUBST(METISSE_DOC_DIR)


# ------------------------------------

AC_OUTPUT([
Makefile
libmetisse/Makefile
libmetisse/libmetisse.pc
xlibs/Makefile
xlibs/Xfont/Makefile
xlibs/Xfont/fontfile/Makefile
xlibs/Xfont/FreeType/Makefile
xlibs/Xfont/builtins/Makefile
xlibs/Xfont/bitmap/Makefile
xlibs/Xfont/fc/Makefile
xlibs/Xfont/util/Makefile
xlibs/Xfont/stubs/Makefile
xlibs/Xau/Makefile
xserver/Makefile
xserver/include/Makefile
xserver/dix/Makefile
xserver/fb/Makefile
xserver/mi/Makefile
xserver/os/Makefile
xserver/Xext/Makefile
xserver/render/Makefile
xserver/xfixes/Makefile
xserver/record/Makefile
xserver/dbe/Makefile
xserver/GL/Makefile
xserver/GL/glx/Makefile
xserver/GL/mesa/Makefile
xserver/metisse/Makefile
FvwmCompositor/Makefile
FvwmCompositor/desktop/Makefile
FvwmCompositor/eigen/Makefile
FvwmCompositor/fvwmmodule/Makefile
FvwmCompositor/renderer/Makefile
FvwmCompositor/texture/Makefile
FvwmCompositor/main/Makefile
FvwmCompositor/config/Makefile
FvwmCompositor/config/config
FvwmCompositor/config/metisserc
FvwmCompositor/config/colours/Makefile
FvwmCompositor/config/window/Makefile
FvwmCompositor/config/themes/Makefile
FvwmCompositor/config/FvwmScript/Makefile
FvwmCompositor/images/Makefile
FvwmCompositor/images/cursors/Makefile
FvwmCompositor/images/buttons/Makefile
FvwmCompositor/images/buttons/redmond/Makefile
FvwmCompositor/images/buttons/Ia_Ora/Makefile
FvwmCompositor/images/backgrounds/Makefile
libutils/Makefile
bin/Makefile
bin/metisse-start-fvwm
bin/metisse-demo-address-app
bin/metisse-combo2radio
bin/metisse-combo2radio-edit
doc/Makefile
doc/docs/Makefile
rpm/Makefile
rpm/metisse.spec
shaders/Makefile
])

dnl xserver/miext/Makefile
dnl xserver/miext/layer/Makefile
dnl xserver/miext/shadow/Makefile
dnl xserver/randr/Makefile

eval my_bindir="`eval echo ${bindir}`"
eval my_libexecdir="`eval echo ${libexecdir}`"
eval my_moddir="`eval echo ${FVWM_MODULEDIR}`"; eval my_moddir="$my_moddir"

echo "
Metisse Configuration:

Xmetisse will be build and installed in:
	$my_bindir"

if test x"$with_FvwmCompositor" = x"no"; then
  echo "
  *** WARNING: FvwmCompositor will not be build because:
	$FvwmCompositor_failure"
else
  echo "FvwmCompositor will be build and installed in:
	$my_moddir"
fi
if test x"$with_wmtrace" = x"no"; then
  echo "WARNING: wmtrace will not be build because: $wmtrace_failure"
else
  echo "wmtrace and FvwmTrace will be build and installed"
fi
echo "Xmetisse font path:
	$FONTPATH"
echo ""
echo "MMX in fb:               $with_mmx $problem_mmx"
echo "Xcursor Library:         $with_xcursor $problem_xcursor"
echo "GLX extension:           $with_glx $problem_glx"
echo "Widget tracking support: $with_widget_support $problem_widget_support"
echo "Python 2.x:              $with_python $problem_python"
echo ""
