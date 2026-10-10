if HAVE_GLIB
noinst_LIBRARIES = libgfvwm.a
endif

libgfvwm_a_SOURCES = gfvwm.c gfvwm.h

INCLUDES = \
	${GLIB_CFLAGS} \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"gfvwm\"
