/*
 * rfb.h - header file for RFB DDX implementation.
 */

/*
 *  Copyright (C) 2000, 2001 Const Kaplinsky.  All Rights Reserved.
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

#include "scrnintstr.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "osdep.h"
#include "libmetisse/proto.h"
#include "libmetisse/auth.h"

#define MAX_ENCODINGS 10

extern char *display;


/*
 * Per-screen (framebuffer) structure.  There is only one of these, since we
 * don't allow the X server to have multiple screens.
 */

typedef struct
{
	int width;
	int paddedWidthInBytes;
	int height;
	int depth;
	int bitsPerPixel;
	int sizeInBytes;
	char *pfbMemory;
	Pixel blackPixel;
	Pixel whitePixel;

	Bool cursorIsDrawn; /* TRUE if the cursor is currently drawn */
	Bool dontSendFramebufferUpdate; /* TRUE while removing or drawing the
					 * cursor */

	/* wrapped screen functions */

	CloseScreenProcPtr CloseScreen;

} rfbScreenInfo, *rfbScreenInfoPtr;


/*
 * rfbTranslateFnType is the type of translation functions.
 */

struct rfbClientRec;
typedef void (*rfbTranslateFnType)(
	char *table, rfbPixelFormat *in, rfbPixelFormat *out, char *iptr,
	char *optr, int bytesBetweenInputLines, int width, int height, uint8_t alphaShift);


/*
 * Per-client structure.
 */

typedef struct rfbClientRec {

	int sock;
	char *host;
	/* Possible client states: */
	enum {
		RFB_PROTOCOL_VERSION,	/* establishing protocol version */
		RFB_AUTHENTICATION,	/* authenticating */
		RFB_INITIALISATION,	/* sending initialisation messages */
		RFB_NORMAL		/* normal protocol messages */
	} state;

	Bool viewOnly;		/* Do not accept input from this client. */

	Bool reverseConnection;

	Bool readyForSetColourMapEntries;

	Bool useCopyRect;
	int preferredEncoding;
	int correMaxWidth, correMaxHeight;

	/* The following member is only used during METISSE authentication */

	CARD8 authChallenge[CHALLENGESIZE];

	/* translateFn points to the translation function which is used to copy
	   and translate a rectangle from the framebuffer to an output buffer. */

	rfbTranslateFnType translateFn;

	char *translateLookupTable;

	rfbPixelFormat format;
     uint8_t alphaShift;

	/* statistics */

	int rfbBytesSent[MAX_ENCODINGS];
	int rfbRectanglesSent[MAX_ENCODINGS];
	int rfbLastRectMarkersSent;
	int rfbLastRectBytesSent;
	int rfbCursorShapeBytesSent;
	int rfbCursorShapeUpdatesSent;
	int rfbCursorPosBytesSent;
	int rfbCursorPosUpdatesSent;
	int rfbFramebufferUpdateMessagesSent;
	int rfbRawBytesEquivalent;
	int rfbKeyEventsRcvd;
	int rfbPointerEventsRcvd;
	int rfbWindowShapeUpdatesSent;
	int rfbWindowShapeBytesSent;
	int rfbWindowFacadesUpdatesSent;
	int rfbWindowFacadesBytesSent;

	Bool enableLastRectEncoding;   /* client supports LastRect encoding */
	Bool enableCursorShapeUpdates; /* client supports cursor shape updates */
	Bool enableCursorPosUpdates;   /* client supports PointerPos updates */
	Bool enableShapeUpdate;        /* client supports WindowShape updates */
	Bool enableFacadesEncoding;    /* client support Facades descriptions */
	Bool suppEncodingARGBCursor;  /* rfbEncodingARGBCursor is supported */
	Bool cursorWasChanged;         /* cursor shape update should be sent */
	Bool cursorWasMoved;           /* cursor position update should be sent
					*/

	int cursorX, cursorY;          /* client's cursor position */

	struct rfbClientRec *next;

} rfbClientRec, *rfbClientPtr;


/*
 * This macro is used to test whether there is a framebuffer update needing to
 * be sent to the client.
 */

#define FB_UPDATE_PENDING(c)                                               \
    ((!(c->cl)->enableCursorShapeUpdates && !rfbScreen.cursorIsDrawn) ||   \
     ((c->cl)->enableCursorShapeUpdates && (c->cl)->cursorWasChanged) ||   \
     ((c->cl)->enableCursorPosUpdates && (c->cl)->cursorWasMoved)     ||   \
     ((c->cl)->enableShapeUpdate && c->shapeChanged)                  ||   \
     (REGION_NOTEMPTY((pScreen),&(c)->modifiedRegion) && c->updateRequested))

/*
 * This macro creates an empty region (ie. a region with no areas) if it is
 * given a rectangle with a width or height of zero. It appears that 
 * REGION_INTERSECT does not quite do the right thing with zero-width
 * rectangles, but it should with completely empty regions.
 */

#define SAFE_REGION_INIT(pscreen, preg, rect, size)          \
{                                                            \
      if ( ( (rect) ) &&                                     \
           ( ( (rect)->x2 == (rect)->x1 ) ||                 \
	     ( (rect)->y2 == (rect)->y1 ) ) ) {              \
	  REGION_INIT( (pscreen), (preg), NullBox, 0 );      \
      } else {                                               \
	  REGION_INIT( (pscreen), (preg), (rect), (size) );  \
      }                                                      \
}

/*
 * An rfbGCRec is where we store the pointers to the original GC funcs and ops
 * which we wrap (NULL means not wrapped).
 */

typedef struct {
	GCFuncs *wrapFuncs;
	GCOps *wrapOps;
} rfbGCRec, *rfbGCPtr;

/*
 * Macros for endian swapping.
 */

#define Swap16(s) ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff))

#define Swap32(l) (((l) >> 24) | \
		   (((l) & 0x00ff0000) >> 8)  | \
		   (((l) & 0x0000ff00) << 8)  | \
		   ((l) << 24))


/* init.c */

static const int rfbEndianTest = 1;

#define Swap16IfLE(s) (*(const char *)&rfbEndianTest ? Swap16(s) : (s))

#define Swap32IfLE(l) (*(const char *)&rfbEndianTest ? Swap32(l) : (l))

extern char *desktopName;
extern char rfbThisHost[];

extern rfbScreenInfo rfbScreen;
extern int rfbGCIndex;

extern int inetdSock;
extern struct in_addr interface;

extern int rfbBitsPerPixel(int depth);
extern void rfbLog(char *format, ...);
extern void rfbLogPerror(char *str);


/* sockets.c */

extern int rfbMaxClientWait;

extern int udpPort;
extern int udpSock;
extern Bool udpSockConnected;

extern int rfbPort;
extern int rfbListenSock;

extern void rfbInitSockets();
extern void rfbDisconnectUDPSock();
extern void rfbCloseSock();
extern void rfbCheckFds();
extern void rfbWaitForClient(int sock);
extern int rfbConnect(char *host, int port);

extern int ReadExact(int sock, char *buf, int len);
extern int WriteExact(int sock, char *buf, int len);
extern int ListenOnTCPPort(int port);
extern int ListenOnUDPPort(int port);
extern int ConnectToTcpAddr(char *host, int port);


/* cmap.c */

extern ColormapPtr rfbInstalledColormap;

extern int rfbListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps);
extern void rfbInstallColormap(ColormapPtr pmap);
extern void rfbUninstallColormap(ColormapPtr pmap);
extern void rfbStoreColors(ColormapPtr pmap, int ndef, xColorItem *pdefs);


/* cutpaste.c */

extern void rfbSetXCutText(char *str, int len);
extern void rfbGotXCutText(char *str, int len);


/* kbdptr.c */

extern unsigned char ptrAcceleration;

extern void PtrDeviceInit();
extern void PtrDeviceOn();
extern void PtrDeviceOff();
extern void PtrDeviceControl();
extern void PtrAddEvent(
	unsigned long buttonMask, Window win, int x, int y, rfbClientPtr cl);

extern void KbdDeviceInit();
extern void KbdDeviceOn();
extern void KbdDeviceOff();
extern void KbdAddEvent(Bool down, KeySym keySym, rfbClientPtr cl);
extern void KbdReleaseAllKeys();


/* rfbserver.c */

/*
 * UPDATE_BUF_SIZE must be big enough to send at least one whole line of the
 * framebuffer.  So for a max screen width of say 2K with 32-bit pixels this
 * means 8K minimum.
 */

#define UPDATE_BUF_SIZE 30000
extern char updateBuf[UPDATE_BUF_SIZE];
extern int ublen;

extern rfbClientPtr rfbClientHead;
extern rfbClientPtr pointerClient;

extern Bool rfbAlwaysShared;
extern Bool rfbNeverShared;
extern Bool rfbDontDisconnect;
extern Bool rfbViewOnly; /* run server in view-only mode - Ehud Karni SW */
extern Bool rfbSendNoOpsBeforeEachMessage;

extern void rfbNewClientConnection(int sock);
extern rfbClientPtr rfbReverseConnection(char *host, int port);
extern void rfbClientConnectionGone(int sock);
extern void rfbProcessClientMessage(int sock);
extern void rfbClientConnFailed(rfbClientPtr cl, char *reason);
extern void rfbNewUDPConnection(int sock);
extern void rfbProcessUDPInput(int sock);
extern Bool rfbSendRectEncodingRaw(rfbClientPtr cl, int x,int y,int w,int h);
extern Bool metisseSendRectEncodingRaw(
	rfbClientPtr cl, char *mem, int bitsPerPixel, int bytesPerRow,
	int x, int y, int w, int h);

extern Bool rfbSendUpdateBuf(rfbClientPtr cl);
extern Bool rfbSendNoOp(rfbClientPtr cl);
extern Bool rfbSendSetColourMapEntries(
	rfbClientPtr cl, int firstColour, int nColours);
extern void rfbSendBell();
extern void rfbSendServerCutText(char *str, int len);

void
rfbSendConfigureWindow(
	unsigned long window, int x, int y, unsigned int width,
	unsigned int height, int isroot);
void rfbSendDestroyWindow(unsigned long window);
void rfbSendUnmapWindow(unsigned long window);
void rfbSendRestackWindow(
	unsigned long window, unsigned long nextWindow,
	unsigned long transientFor, unsigned long unmanagedFor,
	unsigned long grabWin, unsigned long duplicateFor,
	unsigned long facadeReal, unsigned long flags);

extern Bool rfbSendCopyRegion(rfbClientPtr cl, RegionPtr reg, int dx, int dy);
extern Bool rfbSendLastRectMarker(rfbClientPtr cl);

/* translate.c */

extern Bool rfbEconomicTranslate;
extern rfbPixelFormat rfbServerFormat;

extern void rfbTranslateNone(
	char *table, rfbPixelFormat *in, rfbPixelFormat *out, char *iptr,
	char *optr, int bytesBetweenInputLines, int width, int height,
	uint8_t alphaShift);
extern Bool rfbSetTranslateFunction(rfbClientPtr cl);
extern void rfbSetClientColourMaps(int firstColour, int nColours);
extern Bool rfbSetClientColourMap(
	rfbClientPtr cl, int firstColour, int nColours);


/* httpd.c */

extern int httpPort;
extern char *httpDir;

extern void httpInitSockets();
extern void httpCheckFds();



/* auth.c */

extern char *rfbAuthPasswdFile;
extern Bool rfbAuthenticating;

void rfbAuthNewClient(rfbClientPtr cl);
extern void rfbAuthNewClientConnection(rfbClientPtr cl);
extern void rfbAuthProcessClientMessage(rfbClientPtr cl);

/* corre.c */

extern Bool rfbSendRectEncodingCoRRE(
	rfbClientPtr cl, char *mem, int bitsPerPixel, int bytesPerRow,
	int x, int y, int w, int h);

/* cursor.c */

extern Bool rfbSendCursorShape(rfbClientPtr cl, ScreenPtr pScreen);
extern Bool rfbSendCursorPos(rfbClientPtr cl, ScreenPtr pScreen);
Bool rfbSendCursorInfo(rfbClientPtr cl, ScreenPtr pScreen);

/* stats.c */

extern void rfbResetStats(rfbClientPtr cl);
extern void rfbPrintStats(rfbClientPtr cl);

/* rfbRootless.c */
typedef struct
{
	CARD32 size;
	CARD32 *data;
	
} rfb_facades_prop_t;

typedef struct _rfbWindowRec rfbWindowRec;

void window_add_client(rfbClientPtr cl);
void window_remove_client(rfbClientPtr cl);
void window_set_updateRequested(rfbClientPtr cl);
void window_union_modifiedRegion(
	rfbClientPtr cl, rfbWindowRec *w, RegionRec *reg);

void window_substract_copyregion(rfbClientPtr cl, RegionRec *reg);

void window_region_uninit(rfbClientPtr cl);
void window_region_init(rfbClientPtr cl, BoxRec *box);

Bool window_rfbSendFramebufferUpdate(rfbClientPtr cl);
void window_schedule_fb_update_all(rfbClientPtr cl);
void window_sendWindowUpdate(rfbClientPtr cl, Window win, BoxRec box, int inc);

Bool rfbRootlessSetupScreen (int index, ScreenPtr pScreen);


/* */
#if 0
#ifdef XV
Bool rfbInitVideo(ScreenPtr pScreen);
#endif
#endif
