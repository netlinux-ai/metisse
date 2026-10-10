INCLUDES = 					\
	-I$(top_srcdir)/xserver/include		\
	-I$(top_srcdir)/xserver/mi		\
	-I$(top_srcdir)/xserver/include/X11/extensions \
	$(XSERVER_CFLAGS)

noinst_LIBRARIES = libXext.a

if MITSHM
MITSHM_SRCS = shm.c shmint.h
endif

if XV
XV_SRCS = xvmain.c xvdisp.c xvmc.c xvdix.h xvmcext.h
endif

if RES
RES_SRCS = xres.c
endif

if SCREENSAVER
SCREENSAVER_SRCS = saver.c
endif

if XINERAMA
PANORAMIX_SRCS = panoramiX.c panoramiX.h panoramiXsrv.h panoramiXprocs.c panoramiXSwap.c 
endif

if XCALIBRATE
XCALIBRATE_SRCS = xcalibrate.c
endif

if EVI
EVI_SRCS = EVI.c EVIstruct.h sampleEVI.c
endif

libXext_a_SOURCES =		\
	bigreq.c		\
	mitmisc.c		\
	shape.c			\
	shapeint.h		\
	sleepuntil.c		\
	sleepuntil.h		\
	sync.c			\
	syncint.h		\
	xext.h			\
	xtest.c			\
	xtest1di.c		\
	xtest1dd.c		\
	xtest1dd.h		\
	xcmisc.c		\
	$(MITSHM_SRCS)		\
	$(XV_SRCS)		\
	$(RES_SRCS)		\
	$(SCREENSAVER_SRCS)	\
	$(PANORAMIX_SRCS)	\
	$(XCALIBRATE_SRCS)	\
	$(EVI_SRCS)

