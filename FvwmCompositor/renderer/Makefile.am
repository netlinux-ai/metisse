## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

noinst_LIBRARIES = librenderer.a

librenderer_a_SOURCES = \
	goodies.cxx \
	WindowRenderer.cxx \
	FoldablePolygon.cxx \
	FoldablePolygon.H \
	WinRegions.H \
	WindowRenderer.H \
	goodies.H \
	WindowShaders.cxx \
	WindowShaders.H

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/FvwmCompositor \
	$(FVWM_CPPFLAGS) \
	$(NUCLEO_CPPFLAGS) \
	$(X_CFLAGS) \
	$(SPI_CFLAGS)
