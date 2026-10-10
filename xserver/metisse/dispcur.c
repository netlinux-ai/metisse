/*
   (c) Copyright 2002-2003  Denis Oliver Kropp <dok@directfb.org>
   All rights reserved.

   XDirectFB is mainly based on XDarwin and
   also contains some KDrive, XFree and XWin code.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mi.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "mipointrst.h"
#include "servermd.h"
#include "globals.h"

#include "rfb.h"
#include "dispcur.h"

typedef struct {
	QueryBestSizeProcPtr    QueryBestSize;
	DisplayCursorProcPtr    DisplayCursor;
	ConstrainCursorProcPtr  ConstrainCursor;

	CursorPtr pCursor;
	int x;
	int y;
	Bool checkPixels;

	miPointerSpriteFuncPtr  spriteFuncs;
} rfbCursorScreenRec, *rfbCursorScreenPtr;

static int rfbCursorScreenIndex = -1;
static unsigned long rfbCursorGeneration = 0;

#define CURSOR_PRIV(pScreen) \
    ((rfbCursorScreenPtr)pScreen->devPrivates[rfbCursorScreenIndex].ptr)
#define L_CURSOR_PRIV(pScreen) \
    (pScreen->devPrivates[rfbCursorScreenIndex].ptr)

/*
===========================================================================

 Pointer sprite functions

===========================================================================
*/

/*
 *
 */
static Bool
rfbRealizeCursor (ScreenPtr pScreen, CursorPtr pCursor)
{
	rfbCursorScreenPtr pScreenPriv;

	pScreenPriv = CURSOR_PRIV(pScreen);
	if (pCursor == pScreenPriv->pCursor)
	{
		pScreenPriv->checkPixels = TRUE;
	}
	return (*pScreenPriv->spriteFuncs->RealizeCursor) (pScreen, pCursor);
}


/*
 * Free the storage space associated with a realized cursor.
 */
static Bool
rfbUnrealizeCursor (ScreenPtr pScreen, CursorPtr pCursor)
{
	rfbCursorScreenPtr pScreenPriv;

	pScreenPriv = CURSOR_PRIV(pScreen);

	return (*pScreenPriv->spriteFuncs->UnrealizeCursor) (pScreen, pCursor);
}


/*
 * Set the cursor sprite and position.
 */
static void
rfbSetCursor(ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
	rfbClientPtr cl,nextCl;
	rfbCursorScreenPtr pScreenPriv;

	pScreenPriv = CURSOR_PRIV(pScreen);

	if (pScreenPriv->x == x &&
	    pScreenPriv->y == y &&
	    pScreenPriv->pCursor == pCursor &&
	    !pScreenPriv->checkPixels)
	{
		return;
	}
	pScreenPriv->x = x;
	pScreenPriv->y = y;
	pScreenPriv->pCursor = pCursor;

	for (cl = rfbClientHead; cl; cl = nextCl)
	{
		nextCl = cl->next;
		if (cl->cursorWasMoved && cl->enableCursorPosUpdates)
		{
			if (x == cl->cursorX && y == cl->cursorY) {
				cl->cursorWasMoved = FALSE;
				continue;
			}
		}
		rfbSendCursorInfo(cl, pScreen);
	}
}


/*
 * Move the cursor
 */
static
void rfbMoveCursor(
	ScreenPtr pScreen, int x, int y)
{
	rfbCursorScreenPtr pScreenPriv;

	pScreenPriv = CURSOR_PRIV(pScreen);
	rfbSetCursor (pScreen, pScreenPriv->pCursor, x, y);
}


static miPointerSpriteFuncRec rfbSpriteFuncsRec = {
     rfbRealizeCursor,
     rfbUnrealizeCursor,
     rfbSetCursor,
     rfbMoveCursor
};

/*
===========================================================================

 Pointer screen functions

===========================================================================
*/

static Bool
rfbCursorOffScreen (ppScreen, x, y)
    ScreenPtr   *ppScreen;
    int         *x, *y;
{
	return FALSE;
}

static void
rfbCrossScreen (pScreen, entering)
    ScreenPtr   pScreen;
    Bool        entering;
{
}

static void
rfbWarpCursor (ScreenPtr pScreen, int x, int y)
{
	rfbClientPtr cl,nextCl;

	miPointerWarpCursor (pScreen, x, y);
	for (cl = rfbClientHead; cl; cl = nextCl)
	{
		nextCl = cl->next;
		if (cl->enableCursorPosUpdates)
		{
			if (x == cl->cursorX && y == cl->cursorY) {
				cl->cursorWasMoved = FALSE;
				continue;
			}
			cl->cursorWasMoved = TRUE;
		}
		/* update send at rfbSetCursor */ 
	}
	
}


static miPointerScreenFuncRec rfbScreenFuncsRec = {
     rfbCursorOffScreen,
     rfbCrossScreen,
     rfbWarpCursor,
};

/*
===========================================================================

 Other screen functions

===========================================================================
*/

static Bool
rfbDisplayCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
    rfbClientPtr cl;
    rfbCursorScreenPtr pScreenPriv;
    int ret;

    pScreenPriv = CURSOR_PRIV(pScreen);

    ret = (*pScreenPriv->DisplayCursor)(pScreen, pCursor);

    for (cl = rfbClientHead; cl ; cl = cl->next) {
	if (cl->enableCursorShapeUpdates)
	{
	    cl->cursorWasChanged = TRUE;
	    rfbSendCursorInfo(cl, pScreen);
	}
    }
    return ret;
}



static void
rfbConstrainCursor(ScreenPtr pScreen, BoxPtr pBox)
{
	rfbCursorScreenPtr pScreenPriv = CURSOR_PRIV(pScreen);

	pBox->x1 = 0;
	pBox->x2 = pScreen->width;
	pBox->y1 = 0;
	pBox->y2 = pScreen->height;

	(*pScreenPriv->ConstrainCursor)(pScreen, pBox);
}

/*
 * rfbCursorQueryBestSize
 * Handle queries for best cursor size
 */
static void
rfbCursorQueryBestSize(
	int class, unsigned short *width, unsigned short *height,
	ScreenPtr pScreen)
{
     rfbCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

     if (class == CursorShape) {
          /* what is a reasonable limit? */
          *width  = 1024;
          *height = 1024;
     }
     else
          (*ScreenPriv->QueryBestSize)(class, width, height, pScreen);
}



/*
===========================================================================

===========================================================================
*/

/*
 * obtain current cursor pointer
 */

CursorPtr
rfbSpriteGetCursorPtr (ScreenPtr pScreen)
{
	rfbCursorScreenPtr pScreenPriv;
	
	pScreenPriv = CURSOR_PRIV(pScreen);

	return pScreenPriv->pCursor;
}

/*
 * obtain current cursor position
 */

void
rfbSpriteGetCursorPos (ScreenPtr pScreen, int *px, int *py)
{
	
	rfbCursorScreenPtr pScreenPriv;
	
	pScreenPriv = CURSOR_PRIV(pScreen);

	*px = pScreenPriv->x;
	*py = pScreenPriv->y;
}

/*
 * rfbInitCursor
 * Initialize cursor support
 */
Bool
rfbInitCursor (ScreenPtr pScreen)
{
     rfbCursorScreenPtr ScreenPriv;
     miPointerScreenPtr PointPriv;

     /* initialize software cursor handling (always needed as backup) */
     if (!miDCInitialize(pScreen, &rfbScreenFuncsRec)) {
          return FALSE;
     }

     /* allocate private storage for this screen's cursor info */
     if (rfbCursorGeneration != serverGeneration) {
          if ((rfbCursorScreenIndex = AllocateScreenPrivateIndex()) < 0)
               return FALSE;

          rfbCursorGeneration = serverGeneration;
     }

     ScreenPriv = xcalloc( 1, sizeof(rfbCursorScreenRec) );
     if (!ScreenPriv)
          return FALSE;

     L_CURSOR_PRIV(pScreen) = ScreenPriv;

     /* override some screen procedures */
     ScreenPriv->QueryBestSize = pScreen->QueryBestSize;
     pScreen->QueryBestSize = rfbCursorQueryBestSize;
     
     ScreenPriv->DisplayCursor = pScreen->DisplayCursor;
     pScreen->DisplayCursor = rfbDisplayCursor;

     ScreenPriv->ConstrainCursor = pScreen->ConstrainCursor;
     pScreen->ConstrainCursor = rfbConstrainCursor;
     
     ScreenPriv->pCursor = NULL;
     ScreenPriv->x = 0;
     ScreenPriv->y = 0;
     ScreenPriv->checkPixels = TRUE;

     PointPriv = (miPointerScreenPtr)
                 pScreen->devPrivates[miPointerScreenIndex].ptr;
     ScreenPriv->spriteFuncs = PointPriv->spriteFuncs;
     PointPriv->spriteFuncs = &rfbSpriteFuncsRec;

     return TRUE;
}

