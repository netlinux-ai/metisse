## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

bin_PROGRAMS =
bin_SCRIPTS =
python_PYTHON =

if HAVE_FVWMCOMPOSITOR
bin_SCRIPTS += metisse-start-fvwm
#bin_SCRIPTS += metisse-demo-address-app metisse-combo2radio-edit
bin_PROGRAMS += facade-holder metisse-xlib
if HAVE_PYTHON
python_PYTHON += facade_setup.py 
bin_SCRIPTS += metisse-demo-address-app metisse-combo2radio-edit \
	metisse-combo2radio
endif
endif

facade_holder_CPPFLAGS = $(X_CFLAGS) -I$(top_srcdir)
facade_holder_LDADD = $(top_builddir)/libutils/libutils.a \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

metisse_xlib_CPPFLAGS = $(X_CFLAGS) -I$(top_srcdir)
metisse_xlib_LDFLAGS = \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

CLEANFILES = $(bin_SCRIPTS)
