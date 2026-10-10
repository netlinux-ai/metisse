INCLUDES = 					\
	-D_BSD_SOURCE -D_SVID_SOURCE  		\
	-D_GNU_SOURCE -D_XOPEN_SOURCE		\
	-I${top_srcdir}/xserver/render	\
	$(XSERVER_CFLAGS)

noinst_LIBRARIES = libos.a

#	k5auth.c	\
#	lbxio.c		\
#	xdmcp.c		\
#	rpcauth.c	\
#	genalloca.c	\
#	secauth.c

libos_a_SOURCES = 	\
	osdep.h 	\
	xstrans.h 	\
	WaitFor.c	\
	access.c	\
	auth.c		\
	connection.c	\
	hpsocket.c	\
	io.c		\
	mitauth.c	\
	oscolor.c	\
	oscolor.h	\
	osinit.c	\
	utils.c		\
	xdmauth.c	\
	xstrans.c	\
	xalloc.c
