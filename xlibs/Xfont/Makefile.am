# 
#  $Id: Makefile.am,v 1.2 2003/12/17 10:50:26 chapuis Exp $
# 
#  Copyright © 2003 Keith Packard, Noah Levitt
# 
#  Permission to use, copy, modify, distribute, and sell this software and its
#  documentation for any purpose is hereby granted without fee, provided that
#  the above copyright notice appear in all copies and that both that
#  copyright notice and this permission notice appear in supporting
#  documentation, and that the name of Keith Packard not be used in
#  advertising or publicity pertaining to distribution of the software without
#  specific, written prior permission.  Keith Packard makes no
#  representations about the suitability of this software for any purpose.  It
#  is provided "as is" without express or implied warranty.
# 
#  KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
#  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
#  EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
#  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
#  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#  PERFORMANCE OF THIS SOFTWARE.

noinst_LTLIBRARIES = libXfont.la

if XFONT_FONTFILE
FONTFILE_DIR = fontfile
FONTFILE_LIB = fontfile/libfontfile.la
endif

if XFONT_FREETYPE
FREETYPE_DIR = FreeType
FREETYPE_LIB = FreeType/libft.la
endif

if XFONT_BITMAP
BITMAP_DIR = bitmap
BITMAP_LIB = bitmap/libbitmap.la
endif

if XFONT_BUILTINS
BUILTINS_DIR = builtins
BUILTINS_LIB = builtins/libbuiltins.la
endif

if XFONT_FC
FC_DIR = fc
FC_LIB = fc/libfc.la
endif

UTIL_DIR = util
UTIL_LIB = util/libutil.la

STUBS_LIB = stubs/libstubs.la
STUBS_DIR = stubs

SUBDIRS=$(FONTFILE_DIR) $(FREETYPE_DIR) $(BITMAP_DIR) \
	$(BUILTINS_DIR) $(FC_DIR) $(UTIL_DIR) $(STUBS_DIR)

libXfont_la_LIBADD = $(FREETYPE_LIB) $(BITMAP_LIB) $(BUILTINS_LIB) \
	$(FC_LIB) $(FONTFILE_LIB) $(UTIL_LIB) $(STUBS_LIB) \
	$(FREETYPE_LIBS)

libXfont_la_SOURCES =

libXfont_la_LDFLAGS = -version-info 0:0:0

EXTRA_DIST = \
	include/X11/fonts/FS.h \
	include/X11/fonts/FSproto.h \
	include/X11/fonts/fontenc.h \
	include/X11/fonts/fontconf.h \
	include/X11/fonts/font.h \
	include/X11/fonts/fontproto.h \
	include/X11/fonts/fontstruct.h \
	include/X11/fonts/fsmasks.h\
	include/X11/fonts/fontconf.h.in \
	include/bitmap.h	\
	include/bufio.h		\
	include/fntfil.h	\
	include/fntfilio.h	\
	include/fntfilst.h	\
	include/fontencc.h	\
	include/fontmisc.h	\
	include/fontmod.h	\
	include/fontshow.h	\
	include/fontutil.h	\
	include/fontxlfd.h

.SILENT: