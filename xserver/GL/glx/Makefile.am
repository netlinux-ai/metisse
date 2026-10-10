INCLUDES = 					\
	-I$(top_srcdir)/xserver/GL/glx			\
	-I$(top_srcdir)/xserver/GL/glx			\
	-I$(top_srcdir)/xserver/GL/include		\
	-I$(top_srcdir)/xserver/GL/mesa/include		\
	-I$(top_srcdir)/xserver/mi			\
	-I$(top_srcdir)/xserver/fb			\
	-I$(top_srcdir)/xserver/Xext			\
	-I$(top_srcdir)/xserver/render			\
	$(XSERVER_CFLAGS)

noinst_LIBRARIES = libglx.a

libglx_a_SOURCES = \
	g_disptab.c \
	g_disptab.h \
	g_disptab_EXT.c \
	g_disptab_EXT.h \
	g_render.c \
	g_renderswap.c \
	g_single.c \
	g_singleswap.c \
	global.c \
	glxbuf.c \
	glxbuf.h \
	glxcmds.c \
	glxcmdsswap.c \
	glxcontext.h \
	glxdrawable.h \
	glxerror.h \
	glxext.c \
	glxext.h \
	glxfb.c \
	glxfb.h \
	glximports.c \
	glximports.h \
	glxmem.c \
	glxmem.h \
	glxpix.c \
	glxpix.h \
	glxscreens.c \
	glxscreens.h \
	glxserver.h \
	glxutil.c \
	glxutil.h \
	impsize.h \
	render2.c \
	render2swap.c \
	renderpix.c \
	renderpixswap.c \
	rensize.c \
	rensizetab.c \
	single2.c \
	single2swap.c \
	singlepix.c \
	singlepixswap.c \
	singlesize.c \
	singlesize.h \
	xfont.c \
	unpack.h

#libglx_a_CFLAGS =  -DGLXEXT -DXF86DRI -DGLX_DIRECT_RENDERING \
#	-DGLX_USE_DLOPEN -DGLX_USE_MESA
#libglx_a_CFLAGS =  -DGLXEXT -DGLX_USE_DLOPEN -DGLX_USE_MESA
