FONTDIR=$(COMPILEDINFONTDIR)
FONTENCDEFS = -DFONT_ENCODINGS_DIRECTORY=\"$(FONT_ENCODINGS)\"

INCLUDES =			\
	-I${top_srcdir}/xlibs/Xfont/include \
	-I${top_srcdir}/xlibs/Xfont/include/X11/fonts \
	-I${top_srcdir}/xserver/include \
	$(FONTENCDEFS)		

#noinst_LIBRARIES = libfontfile.a
noinst_LTLIBRARIES = libfontfile.la

libfontfile_la_LIBADD = 	\
	$(Z_LIBS)

libfontfile_la_SOURCES = 	\
	bitsource.c		\
	bufio.c			\
	decompress.c		\
	defaults.c		\
	dirfile.c		\
	encparse.c		\
	ffcheck.c		\
	fileio.c		\
	filewr.c		\
	fontdir.c		\
	fontenc.c		\
	fontencI.h		\
	fontencc.c		\
	fontfile.c		\
	fontscale.c		\
	gunzip.c		\
	printerfont.c		\
	register.c		\
	renderers.c
