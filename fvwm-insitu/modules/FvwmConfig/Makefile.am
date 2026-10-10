moduledir = @FVWM_MODULEDIR@
gladedir = $(METISSE_GLADE_DIR)

if HAVE_CONF_UI
module_PROGRAMS = FvwmConfig
glade_DATA = FvwmConfig.glade
endif

FvwmConfig_CFLAGS  = $(CONF_UI_CFLAGS) \
                     -DGLADEDIR=\"$(METISSE_GLADE_DIR)\" \
		     -I$(top_srcdir)
FvwmConfig_LDFLAGS = $(CONF_UI_LIBS)
FvwmConfig_LDADD   = $(top_builddir)/libs/libgfvwm/libgfvwm.a \
                     $(top_builddir)/libs/libfvwm.a
FvwmConfig_SOURCES = main.c \
                     preferences.c \
                     preferences.h

EXTRA_DIST = $(glade_DATA)
