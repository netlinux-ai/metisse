INCLUDES = -I$(top_srcdir)/xserver/include

noinst_LIBRARIES = libXau.a

libXau_a_SOURCES = \
	Xauth.h \
	AuDispose.c \
	AuFileName.c \
	AuGetAddr.c \
	AuGetBest.c \
	AuLock.c \
	AuRead.c \
	AuUnlock.c \
	AuWrite.c \
	Autest.c


if KERBEROS
libXau_a_SOURCES +=k5encode.c
endif KERBEROS


