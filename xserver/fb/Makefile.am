INCLUDES = 					\
	-I$(top_srcdir)/xserver/mi		\
	-I$(top_srcdir)/xserver/render		\
	-I$(top_srcdir)/xserver/Xext		\
	$(XSERVER_CFLAGS)			\
	$(MMX_CFLAGS)

noinst_LIBRARIES = libfb.a libfbmmx.a

if MMX_CAPABLE
AM_CFLAGS = -DUSE_MMX

libfbmmx_a_CFLAGS =				\
	$(DIX_CFLAGS)				\
	-DUSE_MMX				\
	-mmmx					\
	-msse					\
	-Winline				\
	--param inline-unit-growth=10000	\
        --param large-function-growth=10000
endif

libfbmmx_a_SOURCES =	\
	fbmmx.c		\
	fbmmx.h

libfb_a_SOURCES = \
	fbedge.c \
	fb.h \
	fb24_32.c \
	fb24_32.h \
	fballpriv.c \
	fbarc.c \
	fbbits.c \
	fbbits.h \
	fbblt.c \
	fbbltone.c \
	fbbstore.c \
	fbcmap.c \
	fbcompose.c \
	fbcopy.c \
	fbfill.c \
	fbfillrect.c \
	fbfillsp.c \
	fbgc.c \
	fbgetsp.c \
	fbglyph.c \
	fbimage.c \
	fbline.c \
	fbedgeimp.h \
	fboverlay.c \
	fboverlay.h \
	fbpict.c \
	fbpict.h \
	fbpixmap.c \
	fbpoint.c \
	fbpush.c \
	fbrop.h \
	fbscreen.c \
	fbseg.c \
	fbsetsp.c \
	fbsolid.c \
	fbstipple.c \
	fbtile.c \
	fbtrap.c \
	fbutil.c \
	fbwindow.c \
	fbpseudocolor.c \
	fbpseudocolor.h \
	wfbrename.h
