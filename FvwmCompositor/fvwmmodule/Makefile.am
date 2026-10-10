## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libfvwmModule.a

libfvwmModule_a_SOURCES = \
	FvwmModule.H \
	FvwmModule.cxx \
	AFvwm.H


INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/FvwmCompositor \
	-I$(top_srcdir)/proto \
	-I$(top_srcdir)/libmetisse \
	$(FVWM_CPPFLAGS) \
	$(NUCLEO_CPPFLAGS) \
	$(SPI_CFLAGS)
