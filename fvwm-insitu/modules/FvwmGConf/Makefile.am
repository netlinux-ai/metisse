moduledir = @FVWM_MODULEDIR@

if HAVE_CONF_UI
module_PROGRAMS = FvwmGConf
endif

FvwmGConf_CFLAGS = -I$(top_srcdir) $(CONF_UI_CFLAGS)
FvwmGConf_SOURCES = FvwmGConf.c
FvwmGConf_LDFLAGS = $(CONF_UI_LIBS)
FvwmGConf_LDADD = $(top_builddir)/libs/libgfvwm/libgfvwm.a \
                  $(top_builddir)/libs/libfvwm.a
