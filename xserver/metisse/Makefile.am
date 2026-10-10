INCLUDES = 					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/xserver/fb		\
	-I$(top_srcdir)/xserver/mi		\
	-I$(top_srcdir)/xserver/render		\
	-I$(top_srcdir)/xserver/xfixes		\
	-I$(top_srcdir)/xserver/Xext		\
	-I$(top_srcdir)/xserver/os		\
	$(XSERVER_CFLAGS)

bin_PROGRAMS = Xmetisse

Xmetisse_SOURCES =	\
	auth.c 		\
	cmap.c 		\
	corre.c 	\
	cursor.c 	\
	cutpaste.c 	\
	dispcur.c 	\
	httpd.c 	\
	kbdptr.c 	\
	rfbserver.c 	\
	rfbRootless.c 	\
	sockets.c 	\
	stats.c 	\
	translate.c 	\
	rfb.h 		\
	dispcur.h	\
	rootlessWindow.h	\
	rootlessConfig.h 	\
	rootlessCommon.h 	\
	rootless.h 		\
	rootlessCommon.c 	\
	rootlessGC.c 		\
	rootlessScreen.c 	\
	rootlessValTree.c 	\
	rootlessWindow.c	\
	init.c


#	safeAlpha.h
#	safeAlphaWindow.c
#	safeAlphaPicture.c

# if XV
# Xmetisse_a_SOURCES += rfbxv.c rfbxv.h kxv.c kxv.h fourcc.h
# endif

EXTRA_DIST = \
	tableinitcmtemplate.c	\
	tableinittctemplate.c	\
	tabletranstemplate.c

Xmetisse_LDADD = \
	$(top_builddir)/xserver/dix/libdix.a			\
	$(top_builddir)/xserver/os/libos.a			\
	$(top_builddir)/xserver/fb/libfb.a			\
	$(top_builddir)/xserver/fb/libfbmmx.a			\
	$(top_builddir)/xserver/mi/libmi.a			\
	$(top_builddir)/xserver/Xext/libXext.a			\
	$(top_builddir)/xserver/dix/libxpstubs.a		\
	$(top_builddir)/xserver/render/librender.a		\
	$(top_builddir)/xserver/xfixes/libxfixes.a		\
	$(top_builddir)/xserver/record/librecord.a		\
	$(top_builddir)/xserver/dbe/libdbe.a			\
	$(XSERVER_LIBS)					\
	$(top_builddir)/libmetisse/libmetisse.la             \
	-lm -lz

# Xmetisse_LDFLAGS = -static

if GLX
Xmetisse_LDADD += \
	$(top_builddir)/xserver/GL/glx/libglx.a \
	$(top_builddir)/xserver/GL/mesa/libglcore.a
endif

#     
#	$(top_builddir)/xserver/miext/layer/liblayer.a
#	$(top_builddir)/xserver/miext/shadow/libshadow.a
#	$(top_builddir)/xserver/randr/librandr.a


## Create compatibility symlinks if available

install-exec-local:
	@rm -f $(DESTDIR)$(bindir)/Xwnc
	@$(LN_S) Xmetisse $(DESTDIR)$(bindir)/Xwnc || \
		echo "Minor warning: $(bindir)/Xwnc symlink was not created"
