## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libtexture.a

AM_CPPFLAGS = \
	-I$(top_srcdir)/FvwmCompositor \
	-I$(top_srcdir)/proto \
	-I$(top_srcdir)/libmetisse \
	$(FVWM_CPPFLAGS) \
	$(NUCLEO_CPPFLAGS) \
	$(X_CFLAGS)

libtexture_a_SOURCES = \
	glTiledTexturedImage.cxx  glTiledTexturedImage.H
