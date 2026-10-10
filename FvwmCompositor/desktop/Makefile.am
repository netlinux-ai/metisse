## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libdesktop.a

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/FvwmCompositor \
	-I$(top_srcdir)/proto \
	-I$(top_srcdir)/libmetisse \
	$(FVWM_CPPFLAGS) \
	$(NUCLEO_CPPFLAGS) \
	$(X_CFLAGS) \
	$(SPI_CFLAGS)

libdesktop_a_SOURCES = \
	MetisseWindow.H \
	MetisseDesktop.H \
	MetisseSource.H \
	MetisseSource.H \
	MetisseDesktop.cxx \
	MetisseWindow.cxx \
	MetisseSource.cxx
