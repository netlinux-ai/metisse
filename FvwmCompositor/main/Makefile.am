## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/FvwmCompositor \
	-I$(top_srcdir)/proto \
	-I$(top_srcdir)/libmetisse \
	$(FVWM_CPPFLAGS) \
	$(NUCLEO_CPPFLAGS) \
	$(X_CFLAGS) \
	$(SPI_CFLAGS)

AM_LDFLAGS = \
	$(FVWM_LDFLAGS) \
	$(NUCLEO_LDFLAGS)

LDADD = \
	$(top_builddir)/FvwmCompositor/desktop/libdesktop.a \
	$(top_builddir)/FvwmCompositor/renderer/librenderer.a \
	$(top_builddir)/FvwmCompositor/fvwmmodule/libfvwmModule.a \
	$(top_builddir)/FvwmCompositor/texture/libtexture.a \
	$(top_builddir)/libmetisse/libmetisse.la \
	$(top_builddir)/xserver/mi/libmi.a \
	$(NUCLEO_LIBS) \
	$(SPI_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lX11 $(X_EXTRA_LIBS) $(Xcursor_LIBS)

module_PROGRAMS = FvwmCompositor

FvwmCompositor_SOURCES = \
	compositor.cxx \
	ACursor.cxx \
	ACursor.H \
	AUtils.cxx \
	AUtils.H \
	AWidgets.cxx \
	AWidgets.H \
	AScreen.cxx \
	AScreen.H \
	LayerManager.cxx \
	LayerManager.H \
	ExtendedInputs.cxx \
	ExtendedInputs.H \
	Persistence.cxx \
	Persistence.H 

if HAVE_SPI
FvwmCompositor_SOURCES += WidgetManager.cxx WidgetManager.H
endif
