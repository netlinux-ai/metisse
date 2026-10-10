/*
 * cursor.c - support for cursor shape updates.
 */

/*
 *  Copyright (C) 2000, 2001 Const Kaplinsky.  All Rights Reserved.
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
#include "rfb.h"
#include "mipointer.h"
//#include "sprite.h"
#include "cursorstr.h"
#include "servermd.h"

#include "dispcur.h"

/* Copied from Xvnc/lib/font/util/utilbitmap.c */
static unsigned char _reverse_byte[0x100] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};


/*
 * Send cursor shape either in X-style format or in client pixel format.
 */

Bool
rfbSendCursorShape(rfbClientPtr cl, ScreenPtr pScreen)
{
	CursorPtr pCursor;
	int saved_ublen;
	int bitmapRowBytes, paddedRowBytes, maskBytes, dataBytes;
	CARD8 *bitmapData;
	CARD8 bitmapByte;

	rfbFramebufferUpdateRectHeader *rect;

	saved_ublen = ublen;

	if (ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE)
	{
		if (!rfbSendUpdateBuf(cl))
			return FALSE;
	}
	rect = (rfbFramebufferUpdateRectHeader*)&updateBuf[ublen];
	ublen += sz_rfbFramebufferUpdateRectHeader;


	pCursor = rfbSpriteGetCursorPtr(pScreen);

#ifdef ARGB_CURSOR
	if (cl->suppEncodingARGBCursor && pCursor != NULL &&
	    pCursor->bits->argb != NULL)
	{
		rect->encoding = Swap32IfLE(rfbEncodingARGBCursor);
	}
	else
#endif
	{
		rect->encoding = Swap32IfLE(rfbEncodingXCursor);
	}

	/* If there is no cursor, send update with empty cursor data. */

	if (pCursor != NULL && pCursor->bits->width == 1 &&
	    pCursor->bits->height == 1 && pCursor->bits->mask[0] == 0)
	{
		pCursor = NULL;
	}

	if (pCursor == NULL)
	{
		rect->encoding = Swap32IfLE(rfbEncodingXCursor);
		rect->r.x = rect->r.y = 0;
		rect->r.w = rect->r.h = 0;

		cl->rfbCursorShapeBytesSent += sz_rfbFramebufferUpdateRectHeader;
		cl->rfbCursorShapeUpdatesSent++;

		return TRUE;
	}

	/* Calculate data sizes. */

	bitmapRowBytes = (pCursor->bits->width + 7) / 8;
	paddedRowBytes = PixmapBytePad(pCursor->bits->width, 1);
	maskBytes = bitmapRowBytes * pCursor->bits->height;
#ifdef ARGB_CURSOR
	dataBytes = (rect->encoding == Swap32IfLE(rfbEncodingARGBCursor)) ?
		(pCursor->bits->width * pCursor->bits->height * 4) : maskBytes;
#else
	dataBytes = maskBytes;
#endif


	/* Prepare rectangle header. */

	rect->r.x = Swap16IfLE(pCursor->bits->xhot);
	rect->r.y = Swap16IfLE(pCursor->bits->yhot);
	rect->r.w = Swap16IfLE(pCursor->bits->width);
	rect->r.h = Swap16IfLE(pCursor->bits->height);

	/* Prepare actual cursor data (depends on encoding used). */

	if (rect->encoding == Swap32IfLE(rfbEncodingXCursor))
	{
		int x, y;
		rfbXCursorColors *colors;
		if (ublen + sz_rfbXCursorColors > UPDATE_BUF_SIZE)
		{
			if (!rfbSendUpdateBuf(cl))
				return FALSE;
		}
		colors = (rfbXCursorColors*)&updateBuf[ublen];
		ublen += sz_rfbXCursorColors;

		/* XCursor encoding. */
		colors->foreRed   = (char)(pCursor->foreRed   >> 8);
		colors->foreGreen = (char)(pCursor->foreGreen >> 8);
		colors->foreBlue  = (char)(pCursor->foreBlue  >> 8);
		colors->backRed   = (char)(pCursor->backRed   >> 8);
		colors->backGreen = (char)(pCursor->backGreen >> 8);
		colors->backBlue  = (char)(pCursor->backBlue  >> 8);

		bitmapData = (CARD8 *)pCursor->bits->source;

		for (y = 0; y < pCursor->bits->height; y++)
		{
			for (x = 0; x < bitmapRowBytes; x++)
			{
				if (ublen >= UPDATE_BUF_SIZE)
				{
					if (!rfbSendUpdateBuf(cl))
						return FALSE;
				}

				bitmapByte = bitmapData[y * paddedRowBytes + x];
				if (screenInfo.bitmapBitOrder == LSBFirst)
				{
					bitmapByte = _reverse_byte[bitmapByte];
				}
				updateBuf[ublen++] = (char)bitmapByte;
			}
		}
		/* Prepare transparency mask. */

		bitmapData = (CARD8 *)pCursor->bits->mask;

		for (y = 0; y < pCursor->bits->height; y++)
		{
			for (x = 0; x < bitmapRowBytes; x++)
			{
				if (ublen >= UPDATE_BUF_SIZE)
				{
					if (!rfbSendUpdateBuf(cl))
						return FALSE;
				}

				bitmapByte = bitmapData[y * paddedRowBytes + x];
				if (screenInfo.bitmapBitOrder == LSBFirst)
				{
					bitmapByte = _reverse_byte[bitmapByte];
				}
				updateBuf[ublen++] = (char)bitmapByte;
			}
		}
	}
	else
	{
#ifdef ARGB_CURSOR
		int longsLeft = dataBytes/4;
		int longsSent = 0;
		while (longsLeft > 0)
		{
			int nbLongs;

			if (UPDATE_BUF_SIZE - ublen <= 3)
			{
				if (!rfbSendUpdateBuf(cl))
					return FALSE;
			}

			nbLongs = min((UPDATE_BUF_SIZE - ublen)/4, longsLeft);

			if (cl->format.bigEndian != rfbServerFormat.bigEndian)
			{
				int i;
				CARD32 *buf = (CARD32*)&updateBuf[ublen];
				for(i = 0; i < nbLongs; i++)
				{
					buf[i] = Swap32(pCursor->bits->argb[longsSent+i]);
				}
			}
			else
			{
				memcpy(&updateBuf[ublen], &pCursor->bits->argb[longsSent],
				       nbLongs*4);
			}
			ublen += nbLongs*4;
			longsSent += nbLongs;
			longsLeft -= nbLongs;
		}
#endif
	}

	/* Update statistics. */

	cl->rfbCursorShapeBytesSent += (ublen - saved_ublen);
	cl->rfbCursorShapeUpdatesSent++;

	return TRUE;
}


/*
 * Send cursor position (PointerPos pseudo-encoding).
 */

Bool
rfbSendCursorPos(rfbClientPtr cl, ScreenPtr pScreen)
{
	int x, y;

	rfbFramebufferUpdateRectHeader *rect;
	if (ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE)
	{
		if (!rfbSendUpdateBuf(cl))
			return FALSE;
	}
	rect = (rfbFramebufferUpdateRectHeader*)&updateBuf[ublen];
	ublen += sz_rfbFramebufferUpdateRectHeader;

	rfbSpriteGetCursorPos(pScreen, &x, &y);

	rect->encoding = Swap32IfLE(rfbEncodingPointerPos);
	rect->r.x = Swap16IfLE((CARD16)x);
	rect->r.y = Swap16IfLE((CARD16)y);
	rect->r.w = 0;
	rect->r.h = 0;

	cl->rfbCursorPosBytesSent += sz_rfbFramebufferUpdateRectHeader;
	cl->rfbCursorPosUpdatesSent++;

	if (!rfbSendUpdateBuf(cl))
		return FALSE;

	cl->cursorX = x;
	cl->cursorY = y;

	return TRUE;
}

Bool
rfbSendCursorInfo(rfbClientPtr cl, ScreenPtr pScreen)
{
	rfbFramebufferUpdateMsg *fu = (rfbFramebufferUpdateMsg *)updateBuf;
	Bool sendCursorShape = FALSE;
	Bool sendCursorPos = FALSE;

	/*
	 * If this client understands cursor shape updates, cursor should be
	 * removed from the framebuffer. Otherwise, make sure it's put up.
	 */

	if (cl->enableCursorShapeUpdates)
	{
		if (!rfbScreen.cursorIsDrawn && cl->cursorWasChanged)
		{
			sendCursorShape = TRUE;
		}
	}

	/*
	 * Do we plan to send cursor position update?
	 */

	if (cl->enableCursorPosUpdates && cl->cursorWasMoved)
	{
		sendCursorPos = TRUE;
	}

	if (!sendCursorShape && !sendCursorPos)
	{
		return TRUE;
	}

	cl->rfbFramebufferUpdateMessagesSent++;

	fu->type = rfbFramebufferUpdate;
	fu->window = 0;
	fu->topWindow = 0;
	fu->shmid = 0;

	fu->nRects = Swap16IfLE(!!sendCursorShape + !!sendCursorPos);

	ublen = sz_rfbFramebufferUpdateMsg;

	/* send the NoOp before the first Rect is potentially flushed out */
	if (!rfbSendNoOp(cl))
	    rfbCloseSock(cl->sock);

	if (sendCursorShape)
	{
		cl->cursorWasChanged = FALSE;
		if (!rfbSendCursorShape(cl, pScreen))
			return FALSE;
	}

	if (sendCursorPos) {
		cl->cursorWasMoved = FALSE;
		if (!rfbSendCursorPos(cl, pScreen))
			return FALSE;
	}

	if (!rfbSendUpdateBuf(cl))
		return FALSE;

	return TRUE;
}

