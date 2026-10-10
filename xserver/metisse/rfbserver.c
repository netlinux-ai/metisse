/*
 * rfbserver.c - deal with server-side of the RFB protocol.
 */

/*
 *  Copyright (C) 2003 Olivier Chapuis
 *  Copyright (C) 2000-2002 Constantin Kaplinsky.  All Rights Reserved.
 *  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "windowstr.h"
#include "rfb.h"
#include "input.h"
#include "mipointer.h"
#include "dispcur.h"

char updateBuf[UPDATE_BUF_SIZE];
int ublen;

rfbClientPtr rfbClientHead = NULL;
rfbClientPtr pointerClient = NULL;  /* Mutex for pointer events */

Bool rfbAlwaysShared = FALSE;
Bool rfbNeverShared = FALSE;
Bool rfbDontDisconnect = FALSE;
Bool rfbViewOnly = FALSE; /* run server in view only mode - Ehud Karni SW */
Bool rfbSendNoOpsBeforeEachMessage = FALSE;

static rfbClientPtr rfbNewClient(int sock);
static void rfbProcessClientProtocolVersion(rfbClientPtr cl);
static void rfbProcessClientNormalMessage(rfbClientPtr cl);
static void rfbProcessClientInitMessage(rfbClientPtr cl);

/*
 * rfbNewClientConnection is called from sockets.c when a new connection
 * comes in.
 */

void
rfbNewClientConnection(sock)
    int sock;
{
    rfbClientPtr cl;

    cl = rfbNewClient(sock);

}


/*
 * rfbReverseConnection is called by the CORBA stuff to make an outward
 * connection to a "listening" RFB client.
 */

rfbClientPtr
rfbReverseConnection(host, port)
    char *host;
    int port;
{
    int sock;
    rfbClientPtr cl;

    if ((sock = rfbConnect(host, port)) < 0)
	return (rfbClientPtr)NULL;

    cl = rfbNewClient(sock);

    if (cl) {
	cl->reverseConnection = TRUE;
    }

    return cl;
}


/*
 * rfbNewClient is called when a new connection has been made by whatever
 * means.
 */

static 
rfbClientPtr rfbNewClient(sock)
{
    rfbProtocolVersionMsg pv;
    rfbClientPtr cl;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if (rfbClientHead == NULL) {
	/* no other clients - make sure we don't think any keys are pressed */
	KbdReleaseAllKeys();
	rfbLog("First client\n");
    } else {
	rfbLog("  (other clients");
	for (cl = rfbClientHead; cl; cl = cl->next) {
		fprintf(stderr," %s(%d)",cl->host, cl->sock);
	}
	fprintf(stderr,")\n");
    }

    cl = (rfbClientPtr)xalloc(sizeof(rfbClientRec));

    cl->sock = sock;
    getpeername(sock, (struct sockaddr *)&addr, &addrlen);
    cl->host = strdup(inet_ntoa(addr.sin_addr));

    cl->state = RFB_PROTOCOL_VERSION;

    cl->viewOnly = FALSE;
    cl->reverseConnection = FALSE;
    cl->readyForSetColourMapEntries = FALSE;
    cl->useCopyRect = FALSE;
    cl->preferredEncoding = rfbEncodingRaw;
    cl->correMaxWidth = 48;
    cl->correMaxHeight = 48;

    cl->format = rfbServerFormat;
    cl->translateFn = rfbTranslateNone;
    cl->translateLookupTable = NULL;

    cl->enableCursorShapeUpdates = FALSE;
    cl->enableCursorPosUpdates = FALSE;
    cl->enableLastRectEncoding = FALSE;
 
    cl->enableShapeUpdate = FALSE;
    cl->enableFacadesEncoding = FALSE;
    cl->next = rfbClientHead;
 
    rfbClientHead = cl;

    rfbResetStats(cl);

    sprintf(pv,rfbProtocolVersionFormat,rfbProtocolMajorVersion,
	    rfbProtocolMinorVersion);

    if (WriteExact(sock, pv, sz_rfbProtocolVersionMsg) < 0) {
	rfbLogPerror("rfbNewClient: write");
	rfbCloseSock(sock);
	return NULL;
    }

    window_add_client(cl);

    return cl;
}


/*
 * rfbClientConnectionGone is called from sockets.c just after a connection
 * has gone away.
 */

void
rfbClientConnectionGone(sock)
{
	rfbClientPtr cl, prev, tcl;

    for (prev = NULL, cl = rfbClientHead; cl; prev = cl, cl = cl->next) {
	if (sock == cl->sock)
	    break;
    }

    if (!cl) {
	rfbLog("rfbClientConnectionGone: unknown socket %d\n",sock);
	return;
    }

    rfbLog("Client %s (%d) gone\n", cl->host, cl->sock);
    free(cl->host);

    if (pointerClient == cl)
	pointerClient = NULL;

    if (prev)
	prev->next = cl->next;
    else
	rfbClientHead = cl->next;


    rfbLog("rfbClientConnectionGone: %s\n",
	   (rfbClientHead)? "There are other clients:":"No more clients");

    for (tcl = rfbClientHead; tcl; tcl = tcl->next) {
	    rfbLog("   %s (%d)\n",tcl->host, tcl->sock);
    }

    rfbPrintStats(cl);

    if (cl->translateLookupTable) free(cl->translateLookupTable);

    window_remove_client(cl);

    xfree(cl);
}


/*
 * rfbProcessClientMessage is called when there is data to read from a client.
 */

void
rfbProcessClientMessage(sock)
    int sock;
{
    rfbClientPtr cl;

    for (cl = rfbClientHead; cl; cl = cl->next) {
	if (sock == cl->sock)
	    break;
    }

    if (!cl) {
	rfbLog("rfbProcessClientMessage: unknown socket %d\n",sock);
	rfbCloseSock(sock);
	return;
    }

    switch (cl->state) {
    case RFB_PROTOCOL_VERSION:
	rfbProcessClientProtocolVersion(cl);
	return;
    case RFB_AUTHENTICATION:
	rfbAuthProcessClientMessage(cl);
	return;
    case RFB_INITIALISATION:
	rfbProcessClientInitMessage(cl);
	return;
    default:
	rfbProcessClientNormalMessage(cl);
	return;
    }
}


/*
 * rfbProcessClientProtocolVersion is called when the client sends its
 * protocol version.
 */

static void
rfbProcessClientProtocolVersion(cl)
    rfbClientPtr cl;
{
    rfbProtocolVersionMsg pv;
    int n, major, minor;
    char failureReason[256];

    if ((n = ReadExact(cl->sock, pv, sz_rfbProtocolVersionMsg)) <= 0) {
	if (n == 0)
	    rfbLog("rfbProcessClientProtocolVersion: client gone\n");
	else
	    rfbLogPerror("rfbProcessClientProtocolVersion: read");
	rfbCloseSock(cl->sock);
	return;
    }

    pv[sz_rfbProtocolVersionMsg] = 0;
    if (sscanf(pv,rfbProtocolVersionFormat,&major,&minor) != 2) {
	rfbLog("rfbProcessClientProtocolVersion: not a valid METISSE client\n");
	rfbCloseSock(cl->sock);
	return;
    }
    rfbLog("Protocol version %d.%d\n", major, minor);

    if (major != rfbProtocolMajorVersion) {
	/* Major version mismatch - send a ConnFailed message */

	rfbLog("Major version mismatch\n");
	sprintf(failureReason,
		"METISSE protocol version mismatch - server %d.%d, client %d.%d",
		rfbProtocolMajorVersion,rfbProtocolMinorVersion,major,minor);
	rfbClientConnFailed(cl, failureReason);
	return;
    }

    if (minor != rfbProtocolMinorVersion) {
	/* Minor version mismatch - warn but try to continue */
	rfbLog("Ignoring minor version mismatch\n");
    }

    rfbAuthNewClient(cl);
}


/*
 * rfbClientConnFailed is called when a client connection has failed either
 * because it talks the wrong protocol or it has failed authentication.
 */

void
rfbClientConnFailed(cl, reason)
    rfbClientPtr cl;
    char *reason;
{
    char *buf;
    int len = strlen(reason);

    buf = (char *)xalloc(8 + len);
    ((CARD32 *)buf)[0] = Swap32IfLE(rfbConnFailed);
    ((CARD32 *)buf)[1] = Swap32IfLE(len);
    memcpy(buf + 8, reason, len);

    if (WriteExact(cl->sock, buf, 8 + len) < 0)
	rfbLogPerror("rfbClientConnFailed: write");
    xfree(buf);
    rfbCloseSock(cl->sock);
}


/*
 * rfbProcessClientInitMessage is called when the client sends its
 * initialisation message.
 */

static void
rfbProcessClientInitMessage(cl)
    rfbClientPtr cl;
{
    rfbClientInitMsg ci;
    char buf[256];
    rfbServerInitMsg *si = (rfbServerInitMsg *)buf;
    struct passwd *user;
    int len, n;
    rfbClientPtr otherCl, nextCl;

    if ((n = ReadExact(cl->sock, (char *)&ci,sz_rfbClientInitMsg)) <= 0) {
	if (n == 0)
	    rfbLog("rfbProcessClientInitMessage: client gone\n");
	else
	    rfbLogPerror("rfbProcessClientInitMessage: read");
	rfbCloseSock(cl->sock);
	return;
    }

    si->framebufferWidth = Swap16IfLE(rfbScreen.width);
    si->framebufferHeight = Swap16IfLE(rfbScreen.height);
    si->format = rfbServerFormat;
    si->format.redMax = Swap16IfLE(si->format.redMax);
    si->format.greenMax = Swap16IfLE(si->format.greenMax);
    si->format.blueMax = Swap16IfLE(si->format.blueMax);

    user = getpwuid(getuid());

    if (strlen(desktopName) > 128)	/* sanity check on desktop name len */
	desktopName[128] = 0;

    if (user) {
	sprintf(buf + sz_rfbServerInitMsg, "%s's %s desktop (%s:%s)",
		user->pw_name, desktopName, rfbThisHost, display);
    } else {
	sprintf(buf + sz_rfbServerInitMsg, "%s desktop (%s:%s)",
		desktopName, rfbThisHost, display);
    }
    len = strlen(buf + sz_rfbServerInitMsg);
    si->nameLength = Swap32IfLE(len);

    if (WriteExact(cl->sock, buf, sz_rfbServerInitMsg + len) < 0) {
	rfbLogPerror("rfbProcessClientInitMessage: write");
	rfbCloseSock(cl->sock);
	return;
    }

    cl->state = RFB_NORMAL;

    if (!cl->reverseConnection &&
			(rfbNeverShared || (!rfbAlwaysShared && !ci.shared))) {

	if (rfbDontDisconnect) {
	    for (otherCl = rfbClientHead; otherCl; otherCl = otherCl->next) {
		if ((otherCl != cl) && (otherCl->state == RFB_NORMAL)) {
		    rfbLog("-dontdisconnect: Not shared & existing client\n");
		    rfbLog("  refusing new client %s\n", cl->host);
		    rfbCloseSock(cl->sock);
		    return;
		}
	    }
	} else {
	    for (otherCl = rfbClientHead; otherCl; otherCl = nextCl) {
		nextCl = otherCl->next;
		if ((otherCl != cl) && (otherCl->state == RFB_NORMAL)) {
		    rfbLog("Not shared - closing connection to client %s\n",
			   otherCl->host);
		    rfbCloseSock(otherCl->sock);
		}
	    }
	}
    }
}


/*
 * rfbProcessClientNormalMessage is called when the client has sent a normal
 * protocol message.
 */

static void
rfbProcessClientNormalMessage(cl)
    rfbClientPtr cl;
{
    int n;
    rfbClientToServerMsg msg;
    char *str;

    if ((n = ReadExact(cl->sock, (char *)&msg, 1)) <= 0) {
	if (n != 0)
	    rfbLogPerror("rfbProcessClientNormalMessage: read");
	rfbCloseSock(cl->sock);
	return;
    }

    switch (msg.type) {

    case rfbSetPixelFormat:

	if ((n = ReadExact(cl->sock, ((char *)&msg) + 1,
			   sz_rfbSetPixelFormatMsg - 1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseSock(cl->sock);
	    return;
	}

	cl->format.bitsPerPixel = msg.spf.format.bitsPerPixel;
	cl->format.depth = msg.spf.format.depth;
	cl->format.bigEndian = (msg.spf.format.bigEndian ? 1 : 0);
	cl->format.trueColour = (msg.spf.format.trueColour ? 1 : 0);
	cl->format.redMax = Swap16IfLE(msg.spf.format.redMax);
	cl->format.greenMax = Swap16IfLE(msg.spf.format.greenMax);
	cl->format.blueMax = Swap16IfLE(msg.spf.format.blueMax);
	cl->format.redShift = msg.spf.format.redShift;
	cl->format.greenShift = msg.spf.format.greenShift;
	cl->format.blueShift = msg.spf.format.blueShift;

	cl->readyForSetColourMapEntries = TRUE;

	rfbSetTranslateFunction(cl);
	return;


    case rfbFixColourMapEntries:
	if ((n = ReadExact(cl->sock, ((char *)&msg) + 1,
			   sz_rfbFixColourMapEntriesMsg - 1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseSock(cl->sock);
	    return;
	}
	rfbLog("rfbProcessClientNormalMessage: %s",
		"FixColourMapEntries unsupported\n");
	rfbCloseSock(cl->sock);
	return;


    case rfbSetEncodings:
    {
	int i;
	CARD32 enc;

	if ((n = ReadExact(cl->sock, ((char *)&msg) + 1,
			   sz_rfbSetEncodingsMsg - 1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseSock(cl->sock);
	    return;
	}

	msg.se.nEncodings = Swap16IfLE(msg.se.nEncodings);

	cl->preferredEncoding = -1;
	cl->useCopyRect = FALSE;
	cl->enableCursorShapeUpdates = FALSE;
	cl->enableCursorPosUpdates = FALSE;
	cl->enableLastRectEncoding = FALSE;
	cl->enableShapeUpdate = FALSE;
	cl->enableFacadesEncoding = FALSE;

	for (i = 0; i < msg.se.nEncodings; i++) {
	    if ((n = ReadExact(cl->sock, (char *)&enc, 4)) <= 0) {
		if (n != 0)
		    rfbLogPerror("rfbProcessClientNormalMessage: read");
		rfbCloseSock(cl->sock);
		return;
	    }
	    enc = Swap32IfLE(enc);

	    switch (enc) {

	    case rfbEncodingRaw:
		if (cl->preferredEncoding == -1) {
		    cl->preferredEncoding = enc;
		    rfbLog("Using raw encoding for client %s\n",
			   cl->host);
		}
		break;

	    case rfbEncodingRawShm:
		    if (cl->preferredEncoding == -1) {
			    cl->preferredEncoding = enc;
			    rfbLog("Using raw shm encoding for client %s\n",
				   cl->host);
		    }
		    break;
	    case rfbEncodingCoRRE:
		if (cl->preferredEncoding == -1) {
		    cl->preferredEncoding = enc;
		    rfbLog("Using CoRRE encoding for client %s\n",
			   cl->host);
		}
		break;


	    case rfbEncodingXCursor:
		rfbLog("Enabling X-style cursor updates for client %s\n",
		       cl->host);
		cl->enableCursorShapeUpdates = TRUE;
		cl->cursorWasChanged = TRUE;
		break;

	    case rfbEncodingARGBCursor:
		    rfbLog("client %s support ARGB Cursor\n", cl->host);
		    cl->suppEncodingARGBCursor = TRUE;
		    cl->cursorWasChanged = TRUE;
		break;

	    case rfbEncodingPointerPos:
		if (!cl->enableCursorPosUpdates) {
		    rfbLog("Enabling cursor position updates for client %s\n",
			   cl->host);
		    cl->enableCursorPosUpdates = TRUE;
		    cl->cursorWasMoved = TRUE;
		    cl->cursorX = -1;
		    cl->cursorY = -1;
		}
	        break;

	    case rfbEncodingWindowShape:
		if (!cl->enableShapeUpdate) {
		    rfbLog("Enabling window shape for client %s\n",
			   cl->host);
		    cl->enableShapeUpdate = TRUE;
		}
	        break;

#if 0
	    case rfbEncodingLastRect:
		if (!cl->enableLastRectEncoding) {
		    rfbLog("Enabling LastRect protocol extension for client "
			   "%s\n", cl->host);
		    cl->enableLastRectEncoding = TRUE;
		}
		break;
#endif
	    case rfbEncodingFacades:
		if (!cl->enableFacadesEncoding) {
			rfbLog("Enabling Facades extension for client "
			       "%s\n", cl->host);
			cl->enableFacadesEncoding = TRUE;
		}
	        break;
	    default:
		rfbLog("rfbProcessClientNormalMessage: ignoring unknown "
			   "encoding %d\n", (int)enc);
		break;
	    }
	}

	if (cl->preferredEncoding == -1) {
	    cl->preferredEncoding = rfbEncodingRaw;
	}

	if (cl->enableCursorPosUpdates && !cl->enableCursorShapeUpdates) {
	    rfbLog("Disabling cursor position updates for client %s\n",
		   cl->host);
	    cl->enableCursorPosUpdates = FALSE;
	}

	return;
    }


    case rfbFramebufferUpdateRequest:
    {
	RegionRec tmpRegion;
	BoxRec box;

	if ((n = ReadExact(cl->sock, ((char *)&msg) + 1,
			   sz_rfbFramebufferUpdateRequestMsg-1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseSock(cl->sock);
	    return;
	}

	box.x1 = Swap16IfLE(msg.fur.x);
	box.y1 = Swap16IfLE(msg.fur.y);
	box.x2 = box.x1 + Swap16IfLE(msg.fur.w);
	box.y2 = box.y1 + Swap16IfLE(msg.fur.h);
	SAFE_REGION_INIT(pScreen,&tmpRegion,&box,0);

	window_set_updateRequested(cl);

	if (!cl->readyForSetColourMapEntries) {
	    /* client hasn't sent a SetPixelFormat so is using server's */
	    cl->readyForSetColourMapEntries = TRUE;
	    if (!cl->format.trueColour) {
		if (!rfbSetClientColourMap(cl, 0, 0)) {
		    REGION_UNINIT(pScreen,&tmpRegion);
		    return;
		}
	    }
	}

	if (msg.fur.incremental)
		window_schedule_fb_update_all(cl);
	
	REGION_UNINIT(pScreen,&tmpRegion);
	return;
    }

    case rfbWindowUpdateRequest:
    {
	    BoxRec box;
	    Window w;

	    /*fprintf(stderr,"rfbWindowUpdateRequest %i\n",
	      sizeof(rfbWindowUpdateRequestMsg) ); */
	    if ((n = ReadExact(cl->sock, ((char *)&msg) + 1,
			       sz_rfbWindowUpdateRequestMsg-1)) <= 0)
	    {
		    if (n != 0)
		    {
			    rfbLogPerror("rfbProcessClientNormalMessage: read");
		    }
		    rfbCloseSock(cl->sock);
		    return;
	    }

	    box.x1 = Swap16IfLE(msg.wur.x);
	    box.y1 = Swap16IfLE(msg.wur.y);
	    box.x2 = box.x1 + Swap16IfLE(msg.wur.w);
	    box.y2 = box.y1 + Swap16IfLE(msg.wur.h);

	    w = Swap32IfLE(msg.wur.window);

	    window_sendWindowUpdate(cl, w, box, msg.fur.incremental);

	    return;
    }

    case rfbKeyEvent:

	cl->rfbKeyEventsRcvd++;

	if ((n = ReadExact(cl->sock, ((char *)&msg) + 1,
			   sz_rfbKeyEventMsg - 1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseSock(cl->sock);
	    return;
	}

	if (!rfbViewOnly && !cl->viewOnly) {
	    KbdAddEvent(msg.ke.down, (KeySym)Swap32IfLE(msg.ke.key), cl);
	}
	return;


    case rfbPointerEvent:

	cl->rfbPointerEventsRcvd++;

	/* fprintf(stderr,"rfbPointerEvent %i\n",
	   sizeof(rfbPointerEventMsg) ); */
	
	if ((n = ReadExact(cl->sock, ((char *)&msg) + 1,
			   sz_rfbPointerEventMsg - 1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseSock(cl->sock);
	    return;
	}

	if (pointerClient && (pointerClient != cl))
	    return;

	if (msg.pe.buttonMask == 0)
	    pointerClient = NULL;
	else
	    pointerClient = cl;

	if (!rfbViewOnly && !cl->viewOnly) {
		Window win = (Window) Swap32IfLE(msg.pe.window);
		cl->cursorX = (int)Swap16IfLE(msg.pe.x);
		cl->cursorY = (int)Swap16IfLE(msg.pe.y);
		if (msg.pe.xsgn == 0)
			cl->cursorX = - cl->cursorX;
		if (msg.pe.ysgn == 0)
			cl->cursorY = - cl->cursorY;
		PtrAddEvent(
			Swap32IfLE(msg.pe.buttonMask), win, cl->cursorX,
			cl->cursorY, cl);
	}
	return;


    case rfbClientCutText:

	if ((n = ReadExact(cl->sock, ((char *)&msg) + 1,
			   sz_rfbClientCutTextMsg - 1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseSock(cl->sock);
	    return;
	}

	msg.cct.length = Swap32IfLE(msg.cct.length);

	str = (char *)xalloc(msg.cct.length);

	if ((n = ReadExact(cl->sock, str, msg.cct.length)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    xfree(str);
	    rfbCloseSock(cl->sock);
	    return;
	}

	/* NOTE: We do not accept cut text from a view-only client */
	if (!cl->viewOnly)
	    rfbSetXCutText(str, msg.cct.length);

	xfree(str);
	return;


    default:

	rfbLog("rfbProcessClientNormalMessage: unknown message type %d\n",
		msg.type);
	rfbLog(" ... closing connection\n");
	rfbCloseSock(cl->sock);
	return;
    }
}

/*
 * Send a given rectangle in raw encoding (rfbEncodingRaw).
 */
Bool
metisseSendRectEncodingRaw(
	rfbClientPtr cl, char *mem, int bitsPerPixel, int bytesPerRow,
	int x, int y, int w, int h)
{
    rfbFramebufferUpdateRectHeader rect;
    int nlines;
    int bytesPerLine = w * (cl->format.bitsPerPixel / 8);
    char *fbptr = (mem + (bytesPerRow * y) + (x * (bitsPerPixel / 8)));

    /* Flush the buffer to guarantee correct alignment for translateFn(). */
    if (ublen > 0) {
	if (!rfbSendUpdateBuf(cl))
	    return FALSE;
    }

    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingRaw);

    memcpy(&updateBuf[ublen], (char *)&rect,sz_rfbFramebufferUpdateRectHeader);
    ublen += sz_rfbFramebufferUpdateRectHeader;

    cl->rfbRectanglesSent[rfbEncodingRaw]++;
    cl->rfbBytesSent[rfbEncodingRaw]
	    += sz_rfbFramebufferUpdateRectHeader + bytesPerLine * h;

    nlines = (UPDATE_BUF_SIZE - ublen) / bytesPerLine;

    while (TRUE) {
	if (nlines > h)
	    nlines = h;

	(*cl->translateFn)(cl->translateLookupTable, &rfbServerFormat,
			   &cl->format, fbptr, &updateBuf[ublen],
			   bytesPerRow, w, nlines, cl->alphaShift);

	ublen += nlines * bytesPerLine;
	h -= nlines;

	if (h == 0)	/* rect fitted in buffer, do next one */
	    return TRUE;

	/* buffer full - flush partial rect and do another nlines */

	if (!rfbSendUpdateBuf(cl))
	    return FALSE;

	fbptr += (bytesPerRow * nlines);

	nlines = (UPDATE_BUF_SIZE - ublen) / bytesPerLine;
	if (nlines == 0) {
	    rfbLog("rfbSendRectEncodingRaw: send buffer too small for %d "
		   "bytes per line\n", bytesPerLine);
	    rfbCloseSock(cl->sock);
	    return FALSE;
	}
    }
}


/*
 * Send an empty rectangle with encoding field set to value of
 * rfbEncodingLastRect to notify client that this is the last
 * rectangle in framebuffer update ("LastRect" extension of RFB
 * protocol).
 */

Bool rfbSendLastRectMarker(cl)
    rfbClientPtr cl;
{
    rfbFramebufferUpdateRectHeader rect;

    if (ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
	if (!rfbSendUpdateBuf(cl))
	    return FALSE;
    }

    rect.encoding = Swap32IfLE(rfbEncodingLastRect);
    rect.r.x = 0;
    rect.r.y = 0;
    rect.r.w = 0;
    rect.r.h = 0;

    memcpy(&updateBuf[ublen], (char *)&rect,sz_rfbFramebufferUpdateRectHeader);
    ublen += sz_rfbFramebufferUpdateRectHeader;

    cl->rfbLastRectMarkersSent++;
    cl->rfbLastRectBytesSent += sz_rfbFramebufferUpdateRectHeader;

    fprintf(stderr, "LAST RECTANGLE");
    return TRUE;
}


/*
 * Send the contents of updateBuf.  Returns 1 if successful, -1 if
 * not (errno should be set).
 */

Bool
rfbSendUpdateBuf(rfbClientPtr cl)
{
    /*
    int i;
    for (i = 0; i < ublen; i++) {
	fprintf(stderr,"%02x ",((unsigned char *)updateBuf)[i]);
    }
    fprintf(stderr,"\n");
    */

    if (ublen > 0 && WriteExact(cl->sock, updateBuf, ublen) < 0) {
	rfbLogPerror("rfbSendUpdateBuf: write");
	rfbCloseSock(cl->sock);
	return FALSE;
    }

    ublen = 0;
    return TRUE;
}



/*
 * rfbSendNoOp sends a NoOp message with an incremented integer.
 * It can be used to debug the protocol.
 */
Bool
rfbSendNoOp(rfbClientPtr cl)
{
    if (rfbSendNoOpsBeforeEachMessage)
    {
	static CARD32 i = 0;
	rfbNoOpMsg no;
	no.type = rfbNoOp;
	no.ab = 0xAB;
	no.cd = 0xCD;
	no.ef = 0xEF;
	no.counter = Swap32IfLE(i);
	i++;

	rfbLog("rfbSendNoOp: 0x%lx\n", *(uint64_t *)&no);
	usleep(100);
	if (WriteExact(cl->sock, (char *)&no, sz_rfbNoOpMsg) < 0) {
	    rfbLogPerror("rfbSendNoOp: write");
	    return FALSE;
	}
	usleep(100);
    }
    return TRUE;
}



/*
 * rfbSendSetColourMapEntries sends a SetColourMapEntries message to the
 * client, using values from the currently installed colormap.
 */

Bool
rfbSendSetColourMapEntries(cl, firstColour, nColours)
    rfbClientPtr cl;
    int firstColour;
    int nColours;
{
    char buf[sz_rfbSetColourMapEntriesMsg + 256 * 3 * 2];
    rfbSetColourMapEntriesMsg *scme = (rfbSetColourMapEntriesMsg *)buf;
    CARD16 *rgb = (CARD16 *)(&buf[sz_rfbSetColourMapEntriesMsg]);
    EntryPtr pent;
    int i, len;

    scme->type = rfbSetColourMapEntries;
#if !defined(NO_RFB_PADDING_IN_METISSE)
    scme->pad = 0; /* for valgrind */
#endif
    scme->firstColour = Swap16IfLE(firstColour);
    scme->nColours = Swap16IfLE(nColours);

    len = sz_rfbSetColourMapEntriesMsg;

    pent = (EntryPtr)&rfbInstalledColormap->red[firstColour];
    for (i = 0; i < nColours; i++) {
	if (pent->fShared) {
	    rgb[i*3] = Swap16IfLE(pent->co.shco.red->color);
	    rgb[i*3+1] = Swap16IfLE(pent->co.shco.green->color);
	    rgb[i*3+2] = Swap16IfLE(pent->co.shco.blue->color);
	} else {
	    rgb[i*3] = Swap16IfLE(pent->co.local.red);
	    rgb[i*3+1] = Swap16IfLE(pent->co.local.green);
	    rgb[i*3+2] = Swap16IfLE(pent->co.local.blue);
	}
	pent++;
    }

    len += nColours * 3 * 2;

    if (!rfbSendNoOp(cl))
	return FALSE;

    if (WriteExact(cl->sock, buf, len) < 0) {
	rfbLogPerror("rfbSendSetColourMapEntries: write");
	rfbCloseSock(cl->sock);
	return FALSE;
    }
    return TRUE;
}


/*
 * rfbSendBell sends a Bell message to all the clients.
 */

void
rfbSendBell()
{
    rfbClientPtr cl, nextCl;
    rfbBellMsg b;

    for (cl = rfbClientHead; cl; cl = nextCl) {
	nextCl = cl->next;
	b.type = rfbBell;
	if (!rfbSendNoOp(cl))
	    rfbCloseSock(cl->sock);

	if (WriteExact(cl->sock, (char *)&b, sz_rfbBellMsg) < 0) {
	    rfbLogPerror("rfbSendBell: write");
	    rfbCloseSock(cl->sock);
	}
    }
}


/*
 * rfbSendServerCutText sends a ServerCutText message to all the clients.
 */

void
rfbSendServerCutText(char *str, int len)
{
    rfbClientPtr cl, nextCl;
    rfbServerCutTextMsg sct;

    for (cl = rfbClientHead; cl; cl = nextCl) {
	nextCl = cl->next;
	sct.type = rfbServerCutText;
	sct.length = Swap32IfLE(len);
#if !defined(NO_RFB_PADDING_IN_METISSE)
	/* for valgrind */
	sct.pad1 = 0;
	sct.pad2 = 0;
#endif
	if (!rfbSendNoOp(cl))
	    rfbCloseSock(cl->sock);
	if (WriteExact(cl->sock, (char *)&sct,
		       sz_rfbServerCutTextMsg) < 0) {
	    rfbLogPerror("rfbSendServerCutText: write");
	    rfbCloseSock(cl->sock);
	    continue;
	}
	if (WriteExact(cl->sock, str, len) < 0) {
	    rfbLogPerror("rfbSendServerCutText: write");
	    rfbCloseSock(cl->sock);
	}
    }
}



/*
 *
 */

void
rfbSendConfigureWindow(
	unsigned long window, int x, int y, unsigned int width,
	unsigned int height, int isroot)
{
    rfbClientPtr cl, nextCl;
    rfbConfigureWindowMsg cw;

    for (cl = rfbClientHead; cl; cl = nextCl) {
	nextCl = cl->next;
	cw.type = rfbConfigureWindow;
	cw.window = Swap32IfLE(window);
	if (x >= 0)
	{
		cw.xsgn = Swap16IfLE(1);
		cw.x =  Swap16IfLE(x);
	}
	else
	{
		cw.xsgn = Swap16IfLE(0);
		cw.x =  Swap16IfLE((-x));
	}
	if (y >= 0)
	{
		cw.ysgn = Swap16IfLE(1);
		cw.y =  Swap16IfLE(y);
	}
	else
	{
		cw.ysgn = Swap16IfLE(0);
		cw.y =  Swap16IfLE((-y));
	}
	cw.width = Swap32IfLE(width);
	cw.height = Swap32IfLE(height);
#if !defined(NO_RFB_PADDING_IN_METISSE)
	/* for valgrind */
	cw.pad1 = 0;
	cw.pad2 = 0;
#endif
	cw.isroot = Swap16IfLE(isroot);
	if (!rfbSendNoOp(cl))
	    rfbCloseSock(cl->sock);
	if (WriteExact(cl->sock, (char *)&cw, sz_rfbConfigureWindowMsg) < 0)
	{
		rfbLogPerror("rfbConfigureWindow: write");
		rfbCloseSock(cl->sock);
		continue;
	}
    }
}

void rfbSendDestroyWindow(unsigned long window)
{
    rfbClientPtr cl, nextCl;
    rfbDestroyWindowMsg dw;

    for (cl = rfbClientHead; cl; cl = nextCl) {
	nextCl = cl->next;
	dw.type = rfbDestroyWindow;
	dw.window = Swap32IfLE(window);
#if !defined(NO_RFB_PADDING_IN_METISSE)
	/* for valgrind */
	dw.pad1 = 0;
	dw.pad2 = 0;
#endif
	if (!rfbSendNoOp(cl))
	    rfbCloseSock(cl->sock);
	if (WriteExact(cl->sock, (char *)&dw, sz_rfbDestroyWindowMsg) < 0)
	{
		rfbLogPerror("rfbWindow: write");
		rfbCloseSock(cl->sock);
		continue;
	}
    }
}

void rfbSendUnmapWindow(unsigned long window)
{
    rfbClientPtr cl, nextCl;
    rfbUnmapWindowMsg uw;

    for (cl = rfbClientHead; cl; cl = nextCl) {
	nextCl = cl->next;
	uw.type = rfbUnmapWindow;
	uw.window = Swap32IfLE(window);
#if !defined(NO_RFB_PADDING_IN_METISSE)
	/* for valgrind */
	uw.pad1 = 0;
	uw.pad2 = 0;
#endif
	if (!rfbSendNoOp(cl))
	    rfbCloseSock(cl->sock);
	if (WriteExact(cl->sock, (char *)&uw, sz_rfbUnmapWindowMsg) < 0)
	{
		rfbLogPerror("rfbUnmapWindow: write");
		rfbCloseSock(cl->sock);
		continue;
	}
    }
}

void rfbSendRestackWindow(
	unsigned long window, unsigned long nextWindow,
	unsigned long transientFor, unsigned long unmanagedFor,
	unsigned long grabWin, unsigned long duplicateFor, 
	unsigned long facadeReal, unsigned long flags)
{
    rfbClientPtr cl, nextCl;
    rfbRestackWindowMsg rw;

    for (cl = rfbClientHead; cl; cl = nextCl) {
	nextCl = cl->next;
	rw.type = rfbRestackWindow;
	rw.window = Swap32IfLE(window);
	rw.nextWindow = Swap32IfLE(nextWindow);
	rw.transientFor =  Swap32IfLE(transientFor);
	rw.unmanagedFor =  Swap32IfLE(unmanagedFor);
	rw.grabWindow = Swap32IfLE(grabWin);
	rw.duplicateFor = Swap32IfLE(duplicateFor);
	rw.facadeReal = Swap32IfLE(facadeReal);
	rw.flags = Swap32IfLE(flags);
#if !defined(NO_RFB_PADDING_IN_METISSE)
	/* for valgrind */
	rw.pad1 = 0;
	rw.pad2 = 0;
#endif
	if (!rfbSendNoOp(cl))
	    rfbCloseSock(cl->sock);
	if (WriteExact(cl->sock, (char *)&rw, sz_rfbRestackWindowMsg) < 0)
	{
		rfbLogPerror("rfbRestackWindow: write");
		rfbCloseSock(cl->sock);
		continue;
	}
    }
}

/*****************************************************************************
 *
 * UDP can be used for keyboard and pointer events when the underlying
 * network is highly reliable.  This is really here to support ORL's
 * videotile, whose TCP implementation doesn't like sending lots of small
 * packets (such as 100s of pen readings per second!).
 */

void
rfbNewUDPConnection(sock)
    int sock;
{
    if (write(sock, &ptrAcceleration, 1) < 0) {
	rfbLogPerror("rfbNewUDPConnection: write");
    }
}

/*
 * Because UDP is a message based service, we can't read the first byte and
 * then the rest of the packet separately like we do with TCP.  We will always
 * get a whole packet delivered in one go, so we ask read() for the maximum
 * number of bytes we can possibly get.
 */

void
rfbProcessUDPInput(sock)
    int sock;
{
    int n;
    rfbClientToServerMsg msg;

    if ((n = read(sock, (char *)&msg, sizeof(msg))) <= 0) {
	if (n < 0) {
	    rfbLogPerror("rfbProcessUDPInput: read");
	}
	rfbDisconnectUDPSock();
	return;
    }

    switch (msg.type) {

    case rfbKeyEvent:
	if (n != sz_rfbKeyEventMsg) {
	    rfbLog("rfbProcessUDPInput: key event incorrect length\n");
	    rfbDisconnectUDPSock();
	    return;
	}
	if (!rfbViewOnly) {
	    KbdAddEvent(msg.ke.down, (KeySym)Swap32IfLE(msg.ke.key), 0);
	}
	break;

    case rfbPointerEvent:
	if (n != sz_rfbPointerEventMsg) {
	    rfbLog("rfbProcessUDPInput: ptr event incorrect length\n");
	    rfbDisconnectUDPSock();
	    return;
	}
	if (!rfbViewOnly) {
		int x = (int)Swap16IfLE(msg.pe.x);
		int y = (int)Swap16IfLE(msg.pe.y);
		if (msg.pe.xsgn == 0)
			x = - x;
		if (msg.pe.ysgn == 0)
			y = - y;
		PtrAddEvent(
			Swap32IfLE(msg.pe.buttonMask), Swap32IfLE(msg.pe.window),
			x, y, 0);
	}
	break;

    default:
	rfbLog("rfbProcessUDPInput: unknown message type %d\n",
	       msg.type);
	rfbDisconnectUDPSock();
    }
}
