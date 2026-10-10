/*
 * init.c
 */

/*
 *  Copyright (C) 2003 Olivier Chapuis
 */

/*
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

/*

Copyright (c) 1993  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "X11/X.h"
#define NEED_EVENTS
#include "X11/Xproto.h"
#include "X11/Xos.h"
#include "X11/Xdefs.h"
#include "scrnintstr.h"
#include "servermd.h"
#define PSZ 8
#include "fb.h"
#include "mibstore.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "inputstr.h"
#include "mipointer.h"
#include "dixstruct.h"
#include "propertyst.h"
#include <X11/Xatom.h>
#include <errno.h>
#include <sys/param.h>
#include "dix.h"
#include "micmap.h"
#include "site.h"
#include "globals.h"
#include "shmint.h"
#include "rootless.h"
#include "rfb.h"
#include "dispcur.h"

#define XMETISSERELEASE "0"

#define RFB_DEFAULT_WIDTH  1024
#define RFB_DEFAULT_HEIGHT 768
#define RFB_DEFAULT_DEPTH  24
#define RFB_DEFAULT_WHITEPIXEL 0
#define RFB_DEFAULT_BLACKPIXEL 1

rfbScreenInfo rfbScreen;
int rfbGCIndex;
int rfbWindowIndex;

static Bool initOutputCalled = FALSE;
static Bool noCursor = FALSE;
char *desktopName = "x11";

char rfbThisHost[256];

Atom METISSE_LAST_CLIENT_ID;
Atom METISSE_CONNECT;

static HWEventQueueType alwaysCheckForInput[2] = { 0, 1 };
HWEventQueueType *mieqCheckForInput[2];

static char primaryOrder[4] = "";
static int redBits, greenBits, blueBits;


static Bool rfbScreenInit(int index, ScreenPtr pScreen, int argc,
			  char **argv);
static int rfbKeybdProc(DeviceIntPtr pDevice, int onoff);
static int rfbMouseProc(DeviceIntPtr pDevice, int onoff);

static Bool CheckDisplayNumber(int n);

static Bool rfbAlwaysTrue();

static void rfbClientStateChange(
	CallbackListPtr *, pointer myData, pointer client);

int inetdSock = -1;
static char inetdDisplayNumStr[10];

/* Interface address to bind to. */
struct in_addr interface;

CARD8	rfbBpp[] = { 1, 4, 8, 16, 24, 32 };

#define NUM_RFB_BPP (sizeof (rfbBpp) / sizeof (rfbBpp[0]))

/*
 * ddxProcessArgument is our first entry point and will be called at the
 * very start for each argument.  It is not called again on server reset.
 */

int
ddxProcessArgument (argc, argv, i)
    int argc;
    char *argv[];
    int i;
{
    static Bool firstTime = TRUE;

    if (firstTime)
    {
	rfbScreen.width  = RFB_DEFAULT_WIDTH;
	rfbScreen.height = RFB_DEFAULT_HEIGHT;
	rfbScreen.depth  = RFB_DEFAULT_DEPTH;
	rfbScreen.blackPixel = RFB_DEFAULT_BLACKPIXEL;
	rfbScreen.whitePixel = RFB_DEFAULT_WHITEPIXEL;
	rfbScreen.pfbMemory = NULL; /* FIXME to be removed */
	gethostname(rfbThisHost, 255);
	interface.s_addr = htonl (INADDR_ANY);
	firstTime = FALSE;
    }

    if (strcmp (argv[i], "-geometry") == 0)	/* -geometry WxH */
    {
	if (i + 1 >= argc) UseMsg();
	if (sscanf(argv[i+1],"%dx%d",
		   &rfbScreen.width,&rfbScreen.height) != 2) {
	    ErrorF("Invalid geometry %s\n", argv[i+1]);
	    UseMsg();
	}
	return 2;
    }

    if (strcmp (argv[i], "-depth") == 0)	/* -depth D */
    {
	if (i + 1 >= argc) UseMsg();
	rfbScreen.depth = atoi(argv[i+1]);
	return 2;
    }

    if (strcmp (argv[i], "-pixelformat") == 0) {
	if (i + 1 >= argc) UseMsg();
	if (sscanf(argv[i+1], "%3s%1d%1d%1d", primaryOrder,
		   &redBits, &greenBits, &blueBits) < 4) {
	    ErrorF("Invalid pixel format %s\n", argv[i+1]);
	    UseMsg();
	}

	if (strcasecmp(primaryOrder, "bgr") == 0) {
	    int tmp = redBits;
	    redBits = blueBits;
	    blueBits = tmp;
	} else if (strcasecmp(primaryOrder, "rgb") != 0) {
	    ErrorF("Invalid pixel format %s\n", argv[i+1]);
	    UseMsg();
	}

	return 2;
    }

    if (strcmp (argv[i], "-blackpixel") == 0) {	/* -blackpixel n */
	if (i + 1 >= argc) UseMsg();
	rfbScreen.blackPixel = atoi(argv[i+1]);
	return 2;
    }

    if (strcmp (argv[i], "-whitepixel") == 0) {	/* -whitepixel n */
	if (i + 1 >= argc) UseMsg();
	rfbScreen.whitePixel = atoi(argv[i+1]);
	return 2;
    }

    if (strcmp(argv[i], "-udpinputport") == 0) { /* -udpinputport port */
	if (i + 1 >= argc) UseMsg();
	udpPort = atoi(argv[i+1]);
	return 2;
    }

    if (strcmp(argv[i], "-rfbport") == 0) {	/* -rfbport port */
	if (i + 1 >= argc) UseMsg();
	rfbPort = atoi(argv[i+1]);
	return 2;
    }

    if (strcmp(argv[i], "-rfbwait") == 0) {	/* -rfbwait ms */
	if (i + 1 >= argc) UseMsg();
	rfbMaxClientWait = atoi(argv[i+1]);
	return 2;
    }

    if (strcmp(argv[i], "-nocursor") == 0) {
	noCursor = TRUE;
	return 1;
    }

    if (strcmp(argv[i], "-rfbauth") == 0) {	/* -rfbauth passwd-file */
	if (i + 1 >= argc) UseMsg();
	rfbAuthPasswdFile = argv[i+1];
	return 2;
    }

    if (strcmp(argv[i], "-httpd") == 0) {
	if (i + 1 >= argc) UseMsg();
	httpDir = argv[i+1];
	return 2;
    }

    if (strcmp(argv[i], "-httpport") == 0) {
	if (i + 1 >= argc) UseMsg();
	httpPort = atoi(argv[i+1]);
	return 2;
    }

    /* FIXME */
    if (strcmp(argv[i], "-deferupdate") == 0) {	/* -deferupdate ms */
	if (i + 1 >= argc) UseMsg();
#if 0
	rfbDeferUpdateTime = atoi(argv[i+1]);
#endif
	return 2;
    }

    if (strcmp(argv[i], "-economictranslate") == 0) {
	rfbEconomicTranslate = TRUE;
	return 1;
    }


    if (strcmp(argv[i], "-desktop") == 0) {	/* -desktop desktop-name */
	if (i + 1 >= argc) UseMsg();
	desktopName = argv[i+1];
	return 2;
    }

    if (strcmp(argv[i], "-alwaysshared") == 0) {
	rfbAlwaysShared = TRUE;
	return 1;
    }

    if (strcmp(argv[i], "-nevershared") == 0) {
	rfbNeverShared = TRUE;
	return 1;
    }

    if (strcmp(argv[i], "-dontdisconnect") == 0) {
	rfbDontDisconnect = TRUE;
	return 1;
    }

    /* Run server in view-only mode - Ehud Karni SW */
    if (strcmp(argv[i], "-viewonly") == 0) {
	rfbViewOnly = TRUE;
	return 1;
    }

    if (strcmp(argv[i], "-localhost") == 0) {
	interface.s_addr = htonl (INADDR_LOOPBACK);
	return 1;
    }

    if (strcmp(argv[i], "-interface") == 0) {	/* -interface ipaddr */
	struct in_addr got;
	unsigned long octet;
	char *p, *end;
	int q;
	if (i + 1 >= argc) {
	    UseMsg();
	    return 2;
	}
	if (interface.s_addr != htonl (INADDR_ANY)) {
	    /* Already set (-localhost?). */
	    return 2;
	}
	p = argv[i + 1];
	for (q = 0; q < 4; q++) {
	    octet = strtoul (p, &end, 10);
	    if (p == end || octet > 255) {
		UseMsg ();
		return 2;
	    }
	    if ((q < 3 && *end != '.') || (q == 3 && *end != '\0')) {
		UseMsg ();
		return 2;
	    }
	    got.s_addr = (got.s_addr << 8) | octet;
	    p = end + 1;
	}
	interface.s_addr = htonl (got.s_addr);
	return 2;
    }

    if (strcmp(argv[i], "-inetd") == 0) {	/* -inetd */ 
	int n;
	for (n = 1; n < 100; n++) {
	    if (CheckDisplayNumber(n))
		break;
	}

	if (n >= 100)
	    FatalError("-inetd: couldn't find free display number");

	sprintf(inetdDisplayNumStr, "%d", n);
	display = inetdDisplayNumStr;

	/* fds 0, 1 and 2 (stdin, out and err) are all the same socket to the
           RFB client.  OsInit() closes stdout and stdin, and we don't want
           stderr to go to the RFB client, so make the client socket 3 and
           close stderr.  OsInit() will redirect stderr logging to an
           appropriate log file or /dev/null if that doesn't work. */

	dup2(0,3);
	inetdSock = 3;
	close(2);

	return 1;
    }

    if (strcmp(argv[i], "-version") == 0) {
	ErrorF("Xmetisse version %d.%d.%s\n", rfbProtocolMajorVersion,
	       rfbProtocolMinorVersion, XMETISSERELEASE);
	exit(0);
    }

    if (inetdSock != -1 && argv[i][0] == ':') {
	FatalError("can't specify both -inetd and :displaynumber");
    }

    return 0;
}


/*
 * from kdrive
 */

static Bool
rfbSetPixmapFormats(ScreenInfo *pScreenInfo)
{
    CARD8	    depthToBpp[33];	/* depth -> bpp map */
    int		    i;
    int		    bpp;
    PixmapFormatRec *format;

    for (i = 1; i <= 32; i++)
	depthToBpp[i] = 0;

    /*
     * Generate mappings between bitsPerPixel and depth,
     */

    bpp = rfbBitsPerPixel(rfbScreen.depth);
    if (bpp == 24)
    {
	    bpp = 32;
    }
    depthToBpp[rfbScreen.depth] = bpp;
 
    for (i = 0; i < NUM_RFB_BPP; i++)
    {
	    if (!depthToBpp[rfbBpp[i]])
	    {
		    depthToBpp[rfbBpp[i]] = rfbBpp[i];
	    }
    }
	
    pScreenInfo->imageByteOrder     = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad  = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder     = BITMAP_BIT_ORDER;
    
    pScreenInfo->numPixmapFormats = 0;
    
    for (i = 1; i <= 32; i++)
    {
	if (depthToBpp[i])
	{
	    format = &pScreenInfo->formats[pScreenInfo->numPixmapFormats++];
	    format->depth = i;
	    format->bitsPerPixel = depthToBpp[i];
	    format->scanlinePad = BITMAP_SCANLINE_PAD;
	}
    }
    
    return TRUE;
}

/*
 * InitOutput is called every time the server resets.  It should call
 * AddScreen for each screen (but we only ever have one), and in turn this
 * will call rfbScreenInit.
 */

void
InitOutput(screenInfo, argc, argv)
    ScreenInfo *screenInfo;
    int argc;
    char **argv;
{
	int i,left,top;
	initOutputCalled = TRUE;
    
	if (serverGeneration == 1) {
		rfbLog("Xmetisse version %d.%d.%s\n", rfbProtocolMajorVersion,
		       rfbProtocolMinorVersion, XMETISSERELEASE);
		rfbLog("Copyright (C) 1999 AT&T Laboratories Cambridge.\n");
		rfbLog("Copyright (C) 2000-2002 Constantin Kaplinsky.\n");
		rfbLog("Copyright (C) 2003 Olivier Chapuis.\n");
		rfbLog("Copyright (C) ...etc see the source code\n");
		rfbLog("All Rights Reserved.\n");
		rfbLog("Desktop name '%s' (%s:%s)\n", desktopName,
		       rfbThisHost,display);
		rfbLog("Protocol version supported %d.%d\n",
		       rfbProtocolMajorVersion,
		       rfbProtocolMinorVersion);

		METISSE_LAST_CLIENT_ID = MakeAtom(
			"METISSE_LAST_CLIENT_ID",
			strlen("METISSE_LAST_CLIENT_ID"), TRUE);
		METISSE_CONNECT = MakeAtom(
			"METISSE_CONNECT", strlen("METISSE_CONNECT"), TRUE);
		rfbInitSockets();
		if (inetdSock == -1)
		{
			httpInitSockets();
		}
    }

	/* initialize pixmap formats */

	screenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
	screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
	screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
	screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

	/* FIXME, render support start here ... */
	if (!rfbSetPixmapFormats(screenInfo))
	{
		return;
	}

	if (serverGeneration == 1)
	{
		if (!AddCallback(
			    &ClientStateCallback, rfbClientStateChange, NULL))
		{
			rfbLog("InitOutput: AddCallback failed\n");
			return;
		}
	}

	/* initialize screen */

	if (AddScreen(rfbScreenInit, argc, argv) == -1) {
		FatalError("Couldn't add screen");
	}

	/* I do not think we really what follows?? */

	/* Shift all screens so the X11 (0, 0) coordinate is at the top left
	   of the global screen coordinates.
	   Screens can be arranged so the top left isnt on any screen,
	   so instead use the top left of the leftmost screen as (0,0).
	   This may mean some screen space is in -y, but its better
	   that (0,0) be onscreen, or else default xterms disappear.
	   Its better that -y be used than -x, because when popup
	   menus are forced "onscreen" by dumb window managers like twm,
	   theyll shift the menus down instead of left, which still looks
	   funny but is an easier target to hit. */
	left = dixScreenOrigins[0].x;
	top  = dixScreenOrigins[0].y;

	/* Find leftmost screen. If theres a tie, take the topmost of the two. */
	for (i = 1; i < screenInfo->numScreens; i++) {
		if (dixScreenOrigins[i].x < left  ||
		    (dixScreenOrigins[i].x == left &&
		     dixScreenOrigins[i].y < top)) {
			left = dixScreenOrigins[i].x;
			top = dixScreenOrigins[i].y;
		}
	}

	/* Shift all screens so that there is a screen whose top left
	   is at X11 (0,0) and at global screen coordinate
	   (directfbMainScreenX, directfbMainScreenY). */
	if (left != 0 || top != 0) {
		for (i = 0; i < screenInfo->numScreens; i++) {
			dixScreenOrigins[i].x -= left;
			dixScreenOrigins[i].y -= top;
			ErrorF("Screen %d placed at X11 coordinate (%d,%d).\n",
			       i, dixScreenOrigins[i].x, dixScreenOrigins[i].y);
		}
	}
}

/*
 * Screen functions
 */

static Bool
rfbSaveScreen(ScreenPtr pScreen, int on)
{
     /* FIXME */
     if (on == SCREEN_SAVER_FORCER) {
     }
     else if (on == SCREEN_SAVER_ON) {
     }
     else {
     }

     return TRUE;
}

Bool
rfbCloseScreen (i, pScreen)
    int i;
    ScreenPtr	pScreen;
{
    rfbScreenInfoPtr prfb = &rfbScreen;

    pScreen->CloseScreen = prfb->CloseScreen;

    return (*pScreen->CloseScreen) (i, pScreen);
}

static Bool
rfbScreenInit(int index, ScreenPtr pScreen, int argc, char ** argv)
{
	rfbScreenInfoPtr prfb = &rfbScreen;
	int dpix = 75, dpiy = 75;
	int ret;
	/* char *pbits; */
	VisualPtr vis;
	extern int monitorResolution;
	int                   i;
	/* int dpi; VisualPtr pvisual; */

	prfb->paddedWidthInBytes = PixmapBytePad(prfb->width, prfb->depth);
	prfb->bitsPerPixel = rfbBitsPerPixel(prfb->depth);

	if (strcasecmp(primaryOrder, "") == 0)
	{
		rfbLog("Use default RGB format\n");

		/* these defaults come from XFree */
		switch(prfb->depth)
		{
		case 24:
  		     strcpy(primaryOrder,"bgr") ;
			redBits = 8;
			blueBits = 8;
			greenBits = 8;
			break;
		case 16:
			redBits = 5;
			greenBits = 6;
			blueBits = 5;
			break;
		case 8:
			redBits = 3;
			greenBits = 3;
			blueBits = 2;
			break;
		default:
			/* not possible */
			break;
		}
	}

	for (i = 0; i < screenInfo.numPixmapFormats; i++)
	{
		unsigned long visuals;
		Pixel rm, gm, bm, ro, go, bo;
		/* int fb; */
	
		visuals = 0;
		rm = gm = bm = 0;
		ro = go = bo = 0;
		if (screenInfo.formats[i].depth == prfb->depth)
		{
			extern int defaultColorVisualClass;
			if (defaultColorVisualClass != -1)
			{
				/* FIXME: ? */
				rfbLog("RGB format %d %d %d\n",0, 0, 0);
				visuals = (1 << defaultColorVisualClass);
				rm = gm = bm = 0;
				rfbServerFormat.redMax =
					rfbServerFormat.greenMax =
					rfbServerFormat.blueMax = 0;
				rfbServerFormat.redShift =
					rfbServerFormat.greenShift =
					rfbServerFormat.blueShift = 0;
				rfbServerFormat.trueColour = 0;
			}
			else
			{
				visuals = (1 << TrueColor);
				if (strcasecmp(primaryOrder, "rgb") == 0)
				{
					rfbLog("RGB format %d %d %d\n", redBits, greenBits, blueBits);
					bo = 0;
					bm = (1 << blueBits) - 1;
					go = blueBits;
					gm = ((1 << greenBits) - 1) << go;
					ro = go + greenBits;
					rm = ((1 << redBits) - 1) << ro;
					
				}
				else if (strcasecmp(primaryOrder, "bgr") == 0)
				{
					rfbLog("BGR format %d %d %d\n", blueBits, greenBits, redBits);
					ro = 0;
					rm = (1 << redBits) - 1;
					go = redBits;
					gm = ((1 << greenBits) - 1) << go;
					bo = go + greenBits;
					bm = ((1 << blueBits) - 1) << bo;
				}
				else
				{
					rfbLog("default: RGB format %d %d %d\n", redBits, greenBits, blueBits);
					bo = 0;
					bm = (1 << blueBits) - 1;
					go = blueBits;
					gm = ((1 << greenBits) - 1) << go;
					ro = go + greenBits;
					rm = ((1 << redBits) - 1) << ro;
				}
			}
		}
		fbSetVisualTypesAndMasks (
			screenInfo.formats[i].depth, visuals,
			(screenInfo.formats[i].depth == 16)? 6:8,
			rm, gm, bm);
	}

	pScreen->blackPixel = prfb->blackPixel;
	pScreen->whitePixel = prfb->whitePixel;

	if (monitorResolution != 0) {
		dpix = monitorResolution;
		dpiy = monitorResolution;
	}
    
	ret = fbScreenInit(
		pScreen, NULL, prfb->width, prfb->height,
		dpix, dpiy, prfb->width, prfb->paddedWidthInBytes * 8);

#ifdef RENDER
	if (! fbPictureInit(pScreen, 0, 0))
		return FALSE;
#endif

#ifdef MITSHM
	ShmRegisterFbFuncs(pScreen);
#endif

     /* TODO? */
#ifdef XTRANSPARENCY
	TransparencyExtAddScreen(pScreen, rfbSetOpacity, rfbGetOpacity);
#endif

#ifdef XFreeXDGA
	/* rfbFBDGAInit(pScreen); */
#endif

	prfb->cursorIsDrawn = FALSE;
	prfb->dontSendFramebufferUpdate = FALSE;

	pScreen->SaveScreen = rfbSaveScreen;

	prfb->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = rfbCloseScreen;

	if (!rfbRootlessSetupScreen(index, pScreen))
	{
		return FALSE;
	}


	rfbInitCursor(pScreen);

	if (noCursor) {
		pScreen->DisplayCursor = rfbAlwaysTrue;
		prfb->cursorIsDrawn = TRUE;
	}

	rfbServerFormat.bitsPerPixel = prfb->bitsPerPixel;
	rfbServerFormat.depth = prfb->depth;
	rfbServerFormat.bigEndian = !(*(char *)&rfbEndianTest);

	for (vis = pScreen->visuals; vis->vid != pScreen->rootVisual; vis++)
		;

	if (!vis) {
		rfbLog("rfbScreenInit: couldn't find root visual\n");
		exit(1);
	}

	if (strcasecmp(primaryOrder, "bgr") == 0) {
		vis->offsetRed = 0;
		vis->redMask = (1 << redBits) - 1;
		vis->offsetGreen = redBits;
		vis->greenMask = ((1 << greenBits) - 1) << vis->offsetGreen;
		vis->offsetBlue = vis->offsetGreen + greenBits;
		vis->blueMask = ((1 << blueBits) - 1) << vis->offsetBlue;
	} else {
		vis->offsetBlue = 0;
		vis->blueMask = (1 << blueBits) - 1;
		vis->offsetGreen = blueBits;
		vis->greenMask = ((1 << greenBits) - 1) << vis->offsetGreen;
		vis->offsetRed = vis->offsetGreen + greenBits;
		vis->redMask = ((1 << redBits) - 1) << vis->offsetRed;
	}

	rfbServerFormat.trueColour = (vis->class == TrueColor);
	if (rfbServerFormat.trueColour) {
		rfbServerFormat.redMax = vis->redMask >> vis->offsetRed;
		rfbServerFormat.greenMax = vis->greenMask >> vis->offsetGreen;
		rfbServerFormat.blueMax = vis->blueMask >> vis->offsetBlue;
		rfbServerFormat.redShift = vis->offsetRed;
		rfbServerFormat.greenShift = vis->offsetGreen;
		rfbServerFormat.blueShift = vis->offsetBlue;
	} else {
		rfbServerFormat.redMax
			= rfbServerFormat.greenMax = rfbServerFormat.blueMax = 0;
		rfbServerFormat.redShift
			= rfbServerFormat.greenShift =
			rfbServerFormat.blueShift = 0;
	}

	dixScreenOrigins[index].x = 0;
	dixScreenOrigins[index].y = 0;

#ifdef GLXEXT
extern void GlxWrapInitVisuals(miInitVisualsProcPtr *);
#endif
#ifdef GLXEXT
  {
	  VisualPtr visuals;
	  VisualID defaultVisual;
	  DepthPtr depths;
	  int numVisuals = 1;
	  int rootDepth,numDepths;
	  miInitVisualsProcPtr proc = NULL;

	  defaultVisual = vis->vid;
	  rootDepth = vis->nplanes;
	  visuals = pScreen->visuals;
	  depths = pScreen->allowedDepths;
	  numDepths = pScreen->numDepths;

	  GlxWrapInitVisuals(&proc);
	  /* GlxInitVisuals ignores the last three arguments. */
	  proc(&visuals, &depths, &numVisuals, &numDepths,
	       &rootDepth, &defaultVisual, 0, 0, 0);


	  pScreen->visuals = visuals;
	  pScreen->allowedDepths = depths;
	  pScreen->numDepths = numDepths;
	  pScreen->rootVisual = defaultVisual;
	  pScreen->numVisuals = numVisuals;
	  rfbLog("GLX initialized\n");
  }
#endif

#if 0
#ifdef XV
  if (rfbInitVideo(pScreen))
  {
	  rfbLog("Dummy Xvideo Initialized\n");
  }
  else
  {
	  rfbLog("Fail to initialize Dummy Xvideo\n");
  }
#endif
#endif

  ret = fbCreateDefColormap(pScreen);

  return ret;

} /* end rfbScreenInit */


/*
 * InitInput is also called every time the server resets.  It is called after
 * InitOutput so we can assume that rfbInitSockets has already been called.
 */

static DeviceIntPtr pPointer, pKeyboard;

void
InitInput(argc, argv)
    int argc;
    char *argv[];
{
    pKeyboard = AddInputDevice(rfbKeybdProc, TRUE);
    pPointer = AddInputDevice(rfbMouseProc, TRUE);
    RegisterKeyboardDevice(pKeyboard);
    RegisterPointerDevice(pPointer);
    mieqInit (&pKeyboard->public, &pPointer->public);
    mieqCheckForInput[0] = checkForInput[0];
    mieqCheckForInput[1] = checkForInput[1];
    SetInputCheck(&alwaysCheckForInput[0], &alwaysCheckForInput[1]);
}

static int
rfbKeybdProc(pDevice, onoff)
    DeviceIntPtr pDevice;
    int onoff;
{
    KeySymsRec		keySyms;
    CARD8 		modMap[MAP_LENGTH];
    DevicePtr pDev = (DevicePtr)pDevice;

    switch (onoff)
    {
    case DEVICE_INIT: 
	KbdDeviceInit(pDevice, &keySyms, modMap);
	InitKeyboardDeviceStruct(pDev, &keySyms, modMap,
				 (BellProcPtr)rfbSendBell,
				 (KbdCtrlProcPtr)NoopDDA);
	    break;
    case DEVICE_ON: 
	pDev->on = TRUE;
	KbdDeviceOn();
	break;
    case DEVICE_OFF: 
	pDev->on = FALSE;
	KbdDeviceOff();
	break;
    case DEVICE_CLOSE:
	if (pDev->on)
	    KbdDeviceOff();
	break;
    }
    return Success;
}

static int
rfbMouseProc(pDevice, onoff)
    DeviceIntPtr pDevice;
    int onoff;
{
    BYTE map[32];
    DevicePtr pDev = (DevicePtr)pDevice;
    int i;

    switch (onoff)
    {
    case DEVICE_INIT:
	PtrDeviceInit();
	for(i = 1; i < 32; i++)
	{
		map[i] = i;
	}
	InitPointerDeviceStruct(pDev, map, 31, miPointerGetMotionEvents,
				PtrDeviceControl, 
				miPointerGetMotionBufferSize());
	break;

    case DEVICE_ON:
	pDev->on = TRUE;
	PtrDeviceOn(pDevice);
        break;

    case DEVICE_OFF:
	pDev->on = FALSE;
	PtrDeviceOff();
	break;

    case DEVICE_CLOSE:
	if (pDev->on)
	    PtrDeviceOff();
	break;
    }
    return Success;
}

Bool
LegalModifier(key, pDev)
    unsigned int key;
    DevicePtr	pDev;
{
    return TRUE;
}


void
ProcessInputEvents()
{
    rfbCheckFds();
    httpCheckFds();
    if (*mieqCheckForInput[0] != *mieqCheckForInput[1]) {
	    mieqProcessInputEvents();
	    miPointerUpdate();
    }
}


static Bool CheckDisplayNumber(int n)
{
    char fname[32];
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(6000+n);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	close(sock);
	return FALSE;
    }
    close(sock);

    sprintf(fname, "/tmp/.X%d-lock", n);
    if (access(fname, F_OK) == 0)
	return FALSE;

    sprintf(fname, "/tmp/.X11-unix/X%d", n);
    if (access(fname, F_OK) == 0)
	return FALSE;

    return TRUE;
}


void
rfbRootPropertyChange(PropertyPtr pProp)
{
    if ((pProp->propertyName == XA_CUT_BUFFER0) && (pProp->type == XA_STRING)
	&& (pProp->format == 8))
    {
	rfbGotXCutText(pProp->data, pProp->size);
	return;
    }
    if ((pProp->propertyName == METISSE_CONNECT) && (pProp->type == XA_STRING)
	&& (pProp->format == 8))
    {
	int i;
	rfbClientPtr cl;
	int port = 5500;
	char *host = (char *)Xalloc(pProp->size+1);
	memcpy(host, pProp->data, pProp->size);
	host[pProp->size] = 0;
	for (i = 0; i < pProp->size; i++) {
	    if (host[i] == ':') {
		port = atoi(&host[i+1]);
		host[i] = 0;
	    }
	}

	cl = rfbReverseConnection(host, port);

	free(host);
    }
}


int
rfbBitsPerPixel(depth)
    int depth;
{
    if (depth == 1) return 1;
    else if (depth <= 8) return 8;
    else if (depth <= 16) return 16;
    else return 32;
}


static Bool
rfbAlwaysTrue()
{
    return TRUE;
}


static void
rfbClientStateChange(cbl, myData, clt)
    CallbackListPtr *cbl;
    pointer myData;
    pointer clt;
{
    dispatchException &= ~DE_RESET;	/* hack - force server not to reset */
}

void
ddxGiveUp()
{
    Xfree(rfbScreen.pfbMemory);
    if (initOutputCalled) {
	char unixSocketName[32];
	sprintf(unixSocketName,"/tmp/.X11-unix/X%s",display);
	unlink(unixSocketName);
    }
}

void
AbortDDX()
{
    ddxGiveUp();
}

void
OsVendorInit()
{
}

void
OsVendorFatalError()
{
}

#ifdef DDXTIME /* from ServerOSDefines */
CARD32
GetTimeInMillis()
{
    struct timeval  tp;

    X_GETTIMEOFDAY(&tp);
    return(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}
#endif

void
ddxUseMsg()
{
    ErrorF("-geometry WxH          set framebuffer width & height\n");
    ErrorF("-depth D               set framebuffer depth\n");
    ErrorF("-pixelformat format    set pixel format (BGRnnn or RGBnnn)\n");
    ErrorF("-udpinputport port     UDP port for keyboard/pointer data\n");
    ErrorF("-rfbport port          TCP port for RFB protocol\n");
    ErrorF("-rfbwait time          max time in ms to wait for RFB client\n");
    ErrorF("-nocursor              don't put up a cursor\n");
    ErrorF("-rfbauth passwd-file   use authentication on RFB protocol\n");
    ErrorF("-httpd dir             serve files via HTTP from here\n");
    ErrorF("-httpport port         port for HTTP\n");
    ErrorF("-deferupdate time      time in ms to defer updates "
							     "(default 40)\n");
    ErrorF("-economictranslate     less memory-hungry translation\n");
    ErrorF("-desktop name          METISSE desktop name (default x11)\n");
    ErrorF("-alwaysshared          always treat new clients as shared\n");
    ErrorF("-nevershared           never treat new clients as shared\n");
    ErrorF("-dontdisconnect        don't disconnect existing clients when a "
                                                             "new non-shared\n"
	   "                       connection comes in (refuse new connection "
								 "instead)\n");
    ErrorF("-viewonly              let clients only to view the desktop\n");
    ErrorF("-localhost             only allow connections from localhost\n");
    ErrorF("-interface ipaddr      only bind to specified interface "
								"address\n");
    ErrorF("-inetd                 Xmetisse is launched by inetd\n");
    ErrorF("-version               report Xmetisse version on stderr\n");
    exit(1);
}

/*
 * rfbLog prints a time-stamped message to the log file (stderr).
 */

void rfbLog(char *format, ...)
{
    va_list args;
    char buf[256];
    time_t clock;

    va_start(args, format);

    time(&clock);
    strftime(buf, 255, "%F %T ", localtime(&clock));
    fprintf(stderr, buf);

    vfprintf(stderr, format, args);
    fflush(stderr);

    va_end(args);
}

void rfbLogPerror(char *str)
{
    rfbLog("");
    perror(str);
}
