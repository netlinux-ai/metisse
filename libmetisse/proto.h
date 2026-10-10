/*
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

#ifndef METISSEPROTO_H
#define METISSEPROTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define NO_RFB_PADDING_IN_METISSE

#if defined(NO_RFB_PADDING_IN_METISSE)
//#warning Using the padding-less version of the Metisse protocol
  /* See http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Type-Attributes.html#Type-Attributes */
#define PACKED __attribute__ ((__packed__))
#else
#define PACKED
#endif

/*
 * rfbproto.h - header file for the RFB protocol version 3.3
 *
 * Uses types CARD<n> for an n-bit unsigned integer, INT<n> for an n-bit signed
 * integer (for n = 8, 16 and 32).
 *
 * All multiple byte integers are in big endian (network) order (most
 * significant byte first).  Unless noted otherwise there is no special
 * alignment of protocol structures.
 *
 *
 * Once the initial handshaking is done, all messages start with a type byte,
 * (usually) followed by message-specific data.  The order of definitions in
 * this file is as follows:
 *
 *  (1) Structures used in several types of message.
 *  (2) Structures used in the initial handshaking.
 *  (3) Message types.
 *  (4) Encoding types.
 *  (5) For each message type, the form of the data following the type byte.
 *      Sometimes this is defined by a single structure but the more complex
 *      messages have to be explained by comments.
 */


/*****************************************************************************
 *
 * Structures used in several messages
 *
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Structure used to specify a rectangle.  This structure is a multiple of 4
 * bytes so that it can be interspersed with 32-bit pixel data without
 * affecting alignment.
 */

typedef struct PACKED {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} rfbRectangle;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbRectangle 8
#else
#define sz_rfbRectangle sizeof(rfbRectangle)
#endif

/*-----------------------------------------------------------------------------
 * Structure used to specify pixel format.
 */

typedef struct PACKED {

    uint8_t bitsPerPixel;		/* 8,16,32 only */

    uint8_t depth;		/* 8 to 32 */

    uint8_t bigEndian;		/* True if multi-byte pixels are interpreted
				   as big endian, or if single-bit-per-pixel
				   has most significant bit of the byte
				   corresponding to first (leftmost) pixel. Of
				   course this is meaningless for 8 bits/pix */

    uint8_t trueColour;		/* If false then we need a "colour map" to
				   convert pixels to RGB.  If true, xxxMax and
				   xxxShift specify bits used for red, green
				   and blue */

    /* the following fields are only meaningful if trueColour is true */

    uint16_t redMax;		/* maximum red value (= 2^n - 1 where n is the
				   number of bits used for red). Note this
				   value is always in big endian order. */

    uint16_t greenMax;		/* similar for green */

    uint16_t blueMax;		/* and blue */

    uint8_t redShift;		/* number of shifts needed to get the red
				   value in a pixel to the least significant
				   bit. To find the red value from a given
				   pixel, do the following:
				   1) Swap pixel value according to bigEndian
				      (e.g. if bigEndian is false and host byte
				      order is big endian, then swap).
				   2) Shift right by redShift.
				   3) AND with redMax (in host byte order).
				   4) You now have the red value between 0 and
				      redMax. */

    uint8_t greenShift;		/* similar for green, */
    uint8_t blueShift;		/* and blue */

#if !defined(NO_RFB_PADDING_IN_METISSE)
    uint8_t pad1;
    uint16_t pad2;
#endif

} rfbPixelFormat;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbPixelFormat 17
#else
#define sz_rfbPixelFormat sizeof(rfbPixelFormat)
#endif


/*****************************************************************************
 *
 * Initial handshaking messages
 *
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Protocol Version
 *
 * The server always sends 12 bytes to start which identifies the latest RFB
 * protocol version number which it supports.  These bytes are interpreted
 * as a string of 12 ASCII characters in the format "RFB xxx.yyy\n" where
 * xxx and yyy are the major and minor version numbers (for version 3.3
 * this is "RFB 003.003\n").
 *
 * The client then replies with a similar 12-byte message giving the version
 * number of the protocol which should actually be used (which may be different
 * to that quoted by the server).
 *
 * It is intended that both clients and servers may provide some level of
 * backwards compatibility by this mechanism.  Servers in particular should
 * attempt to provide backwards compatibility, and even forwards compatibility
 * to some extent.  For example if a client demands version 3.1 of the
 * protocol, a 3.0 server can probably assume that by ignoring requests for
 * encoding types it doesn't understand, everything will still work OK.  This
 * will probably not be the case for changes in the major version number.
 *
 * The format string below can be used in sprintf or sscanf to generate or
 * decode the version string respectively.
 */

#define rfbProtocolVersionFormat "METISSE %03d.%03d\n"
#define rfbProtocolMajorVersion 1
#define rfbProtocolMinorVersion 0

typedef char rfbProtocolVersionMsg[17];	/* allow extra byte for null */

#define sz_rfbProtocolVersionMsg 16

/*-----------------------------------------------------------------------------
 * Authentication
 *
 * Once the protocol version has been decided, the server then sends a 32-bit
 * word indicating whether any authentication is needed on the connection.
 * The value of this word determines the authentication scheme in use.  For
 * version 3.0 of the protocol this may have one of the following values:
 */

#define rfbConnFailed 0
#define rfbNoAuth 1
#define rfbMetisseAuth 2

/*
 * rfbConnFailed:	For some reason the connection failed (e.g. the server
 *			cannot support the desired protocol version).  This is
 *			followed by a string describing the reason (where a
 *			string is specified as a 32-bit length followed by that
 *			many ASCII characters).
 *
 * rfbNoAuth:		No authentication is needed.
 *
 * rfbMetisseAuth:	The METISSE authentication scheme is to be used. 
 *                      A 16-byte challenge follows, which the client encrypts as
 *			appropriate using the password and sends the resulting
 *			16-byte response.  If the response is correct, the
 *			server sends the 32-bit word rfbMetisseAuthOK.  If a
 *                      simple failure happens, the server sends
 *                      rfbMetisseAuthFailed and closes the connection. If the
 *                      server decides that too many failures have occurred, it
 *                      sends rfbMetisseAuthTooMany
 *			and closes the connection.  In the latter case, the
 *			server should not allow an immediate reconnection by
 *			the client.
 */

#define rfbMetisseAuthOK 0
#define rfbMetisseAuthFailed 1
#define rfbMetisseAuthTooMany 2


/*-----------------------------------------------------------------------------
 * Client Initialisation Message
 *
 * Once the client and server are sure that they're happy to talk to one
 * another, the client sends an initialisation message.  At present this
 * message only consists of a boolean indicating whether the server should try
 * to share the desktop by leaving other clients connected, or give exclusive
 * access to this client by disconnecting all other clients.
 */

typedef struct PACKED {
    uint8_t shared;
} rfbClientInitMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbClientInitMsg 1
#else
#define sz_rfbClientInitMsg sizeof(rfbClientInitMsg)
#endif


/*-----------------------------------------------------------------------------
 * Server Initialisation Message
 *
 * After the client initialisation message, the server sends one of its own.
 * This tells the client the width and height of the server's framebuffer,
 * its pixel format and the name associated with the desktop.
 */

typedef struct PACKED {
    uint16_t framebufferWidth;
    uint16_t framebufferHeight;
    rfbPixelFormat format;	/* the server's preferred pixel format */
    uint32_t nameLength;
    /* followed by char name[nameLength] */
} rfbServerInitMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbServerInitMsg (8 + sz_rfbPixelFormat)
#else
#define sz_rfbServerInitMsg sizeof(rfbServerInitMsg)
#endif

/*
 * Following the server initialisation message it's up to the client to send
 * whichever protocol messages it wants.  Typically it will send a
 * SetPixelFormat message and a SetEncodings message, followed by a
 * FramebufferUpdateRequest.  From then on the server will send
 * FramebufferUpdate messages in response to the client's
 * FramebufferUpdateRequest messages.  The client should send
 * FramebufferUpdateRequest messages with incremental set to true when it has
 * finished processing one FramebufferUpdate and is ready to process another.
 * With a fast client, the rate at which FramebufferUpdateRequests are sent
 * should be regulated to avoid hogging the network.
 */

/*****************************************************************************
 *
 * Message types
 *
 *****************************************************************************/

/* server -> client */

#define rfbFramebufferUpdate 0
#define rfbSetColourMapEntries 1
#define rfbBell 2
#define rfbServerCutText 3
#define rfbConfigureWindow 4
#define rfbUnmapWindow 5
#define rfbDestroyWindow 6
#define rfbRestackWindow 7

/* client -> server */

#define rfbSetPixelFormat 0
#define rfbFixColourMapEntries 1	/* not currently supported */
#define rfbSetEncodings 2
#define rfbFramebufferUpdateRequest 3
#define rfbKeyEvent 4
#define rfbPointerEvent 5
#define rfbClientCutText 6
#define rfbWindowUpdateRequest 7
#define rfbFacadesDescription 8

/*  server <-> client */

#define rfbNoOp 0xFF

/*****************************************************************************
 *
 * Encoding types
 *
 *****************************************************************************/

#define rfbEncodingRaw 0
#define rfbEncodingCopyRect 1
#define rfbEncodingRRE 2
#define rfbEncodingCoRRE 4
#define rfbEncodingHextile 5
#define rfbEncodingZlib 6
#define rfbEncodingTight 7
#define rfbEncodingZlibHex 8

#define rfbEncodingRawShm 9
/*
 * Special encoding numbers:
 *   0xFFFFFF00 .. 0xFFFFFF0F -- encoding-specific compression levels;
 *   0xFFFFFF10 .. 0xFFFFFF1F -- mouse cursor shape data;
 *   0xFFFFFF20 .. 0xFFFFFF2F -- various protocol extensions;
 *   0xFFFFFF30 .. 0xFFFFFFDF -- not allocated yet;
 *   0xFFFFFFE0 .. 0xFFFFFFEF -- quality level for JPEG compressor;
 *   0xFFFFFFF0 .. 0xFFFFFFFF -- cross-encoding compression levels.
 */

#define rfbEncodingCompressLevel0  0xFFFFFF00
#define rfbEncodingCompressLevel1  0xFFFFFF01
#define rfbEncodingCompressLevel2  0xFFFFFF02
#define rfbEncodingCompressLevel3  0xFFFFFF03
#define rfbEncodingCompressLevel4  0xFFFFFF04
#define rfbEncodingCompressLevel5  0xFFFFFF05
#define rfbEncodingCompressLevel6  0xFFFFFF06
#define rfbEncodingCompressLevel7  0xFFFFFF07
#define rfbEncodingCompressLevel8  0xFFFFFF08
#define rfbEncodingCompressLevel9  0xFFFFFF09

#define rfbEncodingXCursor         0xFFFFFF10
#define rfbEncodingARGBCursor      0xFFFFFF12
#define rfbEncodingPointerPos      0xFFFFFF18

#define rfbEncodingLastRect        0xFFFFFF20
#define rfbEncodingWindowShape     0xFFFFFF21

#define rfbEncodingFacades         0xFFFFFF22

#define rfbEncodingQualityLevel0   0xFFFFFFE0
#define rfbEncodingQualityLevel1   0xFFFFFFE1
#define rfbEncodingQualityLevel2   0xFFFFFFE2
#define rfbEncodingQualityLevel3   0xFFFFFFE3
#define rfbEncodingQualityLevel4   0xFFFFFFE4
#define rfbEncodingQualityLevel5   0xFFFFFFE5
#define rfbEncodingQualityLevel6   0xFFFFFFE6
#define rfbEncodingQualityLevel7   0xFFFFFFE7
#define rfbEncodingQualityLevel8   0xFFFFFFE8
#define rfbEncodingQualityLevel9   0xFFFFFFE9


/*****************************************************************************
 *
 * Server -> client message definitions
 *
 *****************************************************************************/


/*-----------------------------------------------------------------------------
 * FramebufferUpdate - a block of rectangles to be copied to the framebuffer.
 *
 * This message consists of a header giving the number of rectangles of pixel
 * data followed by the rectangles themselves.  The header is padded so that
 * together with the type byte it is an exact multiple of 4 bytes (to help
 * with alignment of 32-bit pixels):
 */

typedef struct PACKED {
	uint8_t type;			/* always rfbFramebufferUpdate */
#if !defined(NO_RFB_PADDING_IN_METISSE)
	uint8_t pad;
#endif
	uint16_t nRects;
	uint32_t window;
	uint32_t topWindow;
	uint32_t shmid;
    /* followed by nRects rectangles */
} rfbFramebufferUpdateMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbFramebufferUpdateMsg 16
#else
#define sz_rfbFramebufferUpdateMsg sizeof(rfbFramebufferUpdateMsg)
#endif

/*
 * Each rectangle of pixel data consists of a header describing the position
 * and size of the rectangle and a type word describing the encoding of the
 * pixel data, followed finally by the pixel data.  Note that if the client has
 * not sent a SetEncodings message then it will only receive raw pixel data.
 * Also note again that this structure is a multiple of 4 bytes.
 */

typedef struct PACKED {
    rfbRectangle r;
    uint32_t encoding;	/* one of the encoding types rfbEncoding... */
} rfbFramebufferUpdateRectHeader;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbFramebufferUpdateRectHeader (sz_rfbRectangle + 4)
#else
#define sz_rfbFramebufferUpdateRectHeader sizeof(rfbFramebufferUpdateRectHeader)
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Raw Encoding.  Pixels are sent in top-to-bottom scanline order,
 * left-to-right within a scanline with no padding in between.
 */


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * CopyRect Encoding.  The pixels are specified simply by the x and y position
 * of the source rectangle.
 */

typedef struct PACKED {
    uint16_t srcX;
    uint16_t srcY;
} rfbCopyRect;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbCopyRect 4
#else
#define sz_rfbCopyRect sizeof(rfbCopyRect)
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * RRE - Rise-and-Run-length Encoding.  We have an rfbRREHeader structure
 * giving the number of subrectangles following.  Finally the data follows in
 * the form [<bgpixel><subrect><subrect>...] where each <subrect> is
 * [<pixel><rfbRectangle>].
 */

typedef struct PACKED {
    uint32_t nSubrects;
} rfbRREHeader;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbRREHeader 4
#else
#define sz_rfbRREHeader sizeof(rfbRREHeader)
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * CoRRE - Compact RRE Encoding.  We have an rfbRREHeader structure giving
 * the number of subrectangles following.  Finally the data follows in the form
 * [<bgpixel><subrect><subrect>...] where each <subrect> is
 * [<pixel><rfbCoRRERectangle>].  This means that
 * the whole rectangle must be at most 255x255 pixels.
 */

typedef struct PACKED {
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
} rfbCoRRERectangle;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbCoRRERectangle 4
#else
#define sz_rfbCoRRERectangle sizeof(rfbCoRRERectangle)
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Hextile Encoding.  The rectangle is divided up into "tiles" of 16x16 pixels,
 * starting at the top left going in left-to-right, top-to-bottom order.  If
 * the width of the rectangle is not an exact multiple of 16 then the width of
 * the last tile in each row will be correspondingly smaller.  Similarly if the
 * height is not an exact multiple of 16 then the height of each tile in the
 * final row will also be smaller.  Each tile begins with a "subencoding" type
 * byte, which is a mask made up of a number of bits.  If the Raw bit is set
 * then the other bits are irrelevant; w*h pixel values follow (where w and h
 * are the width and height of the tile).  Otherwise the tile is encoded in a
 * similar way to RRE, except that the position and size of each subrectangle
 * can be specified in just two bytes.  The other bits in the mask are as
 * follows:
 *
 * BackgroundSpecified - if set, a pixel value follows which specifies
 *    the background colour for this tile.  The first non-raw tile in a
 *    rectangle must have this bit set.  If this bit isn't set then the
 *    background is the same as the last tile.
 *
 * ForegroundSpecified - if set, a pixel value follows which specifies
 *    the foreground colour to be used for all subrectangles in this tile.
 *    If this bit is set then the SubrectsColoured bit must be zero.
 *
 * AnySubrects - if set, a single byte follows giving the number of
 *    subrectangles following.  If not set, there are no subrectangles (i.e.
 *    the whole tile is just solid background colour).
 *
 * SubrectsColoured - if set then each subrectangle is preceded by a pixel
 *    value giving the colour of that subrectangle.  If not set, all
 *    subrectangles are the same colour, the foreground colour;  if the
 *    ForegroundSpecified bit wasn't set then the foreground is the same as
 *    the last tile.
 *
 * The position and size of each subrectangle is specified in two bytes.  The
 * Pack macros below can be used to generate the two bytes from x, y, w, h,
 * and the Extract macros can be used to extract the x, y, w, h values from
 * the two bytes.
 */

#define rfbHextileRaw			(1 << 0)
#define rfbHextileBackgroundSpecified	(1 << 1)
#define rfbHextileForegroundSpecified	(1 << 2)
#define rfbHextileAnySubrects		(1 << 3)
#define rfbHextileSubrectsColoured	(1 << 4)

#define rfbHextilePackXY(x,y) (((x) << 4) | (y))
#define rfbHextilePackWH(w,h) ((((w)-1) << 4) | ((h)-1))
#define rfbHextileExtractX(byte) ((byte) >> 4)
#define rfbHextileExtractY(byte) ((byte) & 0xf)
#define rfbHextileExtractW(byte) (((byte) >> 4) + 1)
#define rfbHextileExtractH(byte) (((byte) & 0xf) + 1)


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * zlib - zlib compressed Encoding.  We have an rfbZlibHeader structure
 * giving the number of bytes following.  Finally the data follows is
 * zlib compressed version of the raw pixel data as negotiated.
 */

typedef struct PACKED {
    uint32_t nBytes;
} rfbZlibHeader;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbZlibHeader 4
#else
#define sz_rfbZlibHeader sizeof(rfbZlibHeader)
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Tight Encoding.
 *
 *-- The first byte of each Tight-encoded rectangle is a "compression control
 *   byte". Its format is as follows (bit 0 is the least significant one):
 *
 *   bit 0:    if 1, then compression stream 0 should be reset;
 *   bit 1:    if 1, then compression stream 1 should be reset;
 *   bit 2:    if 1, then compression stream 2 should be reset;
 *   bit 3:    if 1, then compression stream 3 should be reset;
 *   bits 7-4: if 1000 (0x08), then the compression type is "fill",
 *             if 1001 (0x09), then the compression type is "jpeg",
 *             if 0xxx, then the compression type is "basic",
 *             values greater than 1001 are not valid.
 *
 * If the compression type is "basic", then bits 6..4 of the
 * compression control byte (those xxx in 0xxx) specify the following:
 *
 *   bits 5-4:  decimal representation is the index of a particular zlib
 *              stream which should be used for decompressing the data;
 *   bit 6:     if 1, then a "filter id" byte is following this byte.
 *
 *-- The data that follows after the compression control byte described
 * above depends on the compression type ("fill", "jpeg" or "basic").
 *
 *-- If the compression type is "fill", then the only pixel value follows, in
 * client pixel format (see NOTE 1). This value applies to all pixels of the
 * rectangle.
 *
 *-- If the compression type is "jpeg", the following data stream looks like
 * this:
 *
 *   1..3 bytes:  data size (N) in compact representation;
 *   N bytes:     JPEG image.
 *
 * Data size is compactly represented in one, two or three bytes, according
 * to the following scheme:
 *
 *  0xxxxxxx                    (for values 0..127)
 *  1xxxxxxx 0yyyyyyy           (for values 128..16383)
 *  1xxxxxxx 1yyyyyyy zzzzzzzz  (for values 16384..4194303)
 *
 * Here each character denotes one bit, xxxxxxx are the least significant 7
 * bits of the value (bits 0-6), yyyyyyy are bits 7-13, and zzzzzzzz are the
 * most significant 8 bits (bits 14-21). For example, decimal value 10000
 * should be represented as two bytes: binary 10010000 01001110, or
 * hexadecimal 90 4E.
 *
 *-- If the compression type is "basic" and bit 6 of the compression control
 * byte was set to 1, then the next (second) byte specifies "filter id" which
 * tells the decoder what filter type was used by the encoder to pre-process
 * pixel data before the compression. The "filter id" byte can be one of the
 * following:
 *
 *   0:  no filter ("copy" filter);
 *   1:  "palette" filter;
 *   2:  "gradient" filter.
 *
 *-- If bit 6 of the compression control byte is set to 0 (no "filter id"
 * byte), or if the filter id is 0, then raw pixel values in the client
 * format (see NOTE 1) will be compressed. See below details on the
 * compression.
 *
 *-- The "gradient" filter pre-processes pixel data with a simple algorithm
 * which converts each color component to a difference between a "predicted"
 * intensity and the actual intensity. Such a technique does not affect
 * uncompressed data size, but helps to compress photo-like images better. 
 * Pseudo-code for converting intensities to differences is the following:
 *
 *   P[i,j] := V[i-1,j] + V[i,j-1] - V[i-1,j-1];
 *   if (P[i,j] < 0) then P[i,j] := 0;
 *   if (P[i,j] > MAX) then P[i,j] := MAX;
 *   D[i,j] := V[i,j] - P[i,j];
 *
 * Here V[i,j] is the intensity of a color component for a pixel at
 * coordinates (i,j). MAX is the maximum value of intensity for a color
 * component.
 *
 *-- The "palette" filter converts true-color pixel data to indexed colors
 * and a palette which can consist of 2..256 colors. If the number of colors
 * is 2, then each pixel is encoded in 1 bit, otherwise 8 bits is used to
 * encode one pixel. 1-bit encoding is performed such way that the most
 * significant bits correspond to the leftmost pixels, and each raw of pixels
 * is aligned to the byte boundary. When "palette" filter is used, the
 * palette is sent before the pixel data. The palette begins with an unsigned
 * byte which value is the number of colors in the palette minus 1 (i.e. 1
 * means 2 colors, 255 means 256 colors in the palette). Then follows the
 * palette itself which consist of pixel values in client pixel format (see
 * NOTE 1).
 *
 *-- The pixel data is compressed using the zlib library. But if the data
 * size after applying the filter but before the compression is less then 12,
 * then the data is sent as is, uncompressed. Four separate zlib streams
 * (0..3) can be used and the decoder should read the actual stream id from
 * the compression control byte (see NOTE 2).
 *
 * If the compression is not used, then the pixel data is sent as is,
 * otherwise the data stream looks like this:
 *
 *   1..3 bytes:  data size (N) in compact representation;
 *   N bytes:     zlib-compressed data.
 *
 * Data size is compactly represented in one, two or three bytes, just like
 * in the "jpeg" compression method (see above).
 *
 *-- NOTE 1. If the color depth is 24, and all three color components are
 * 8-bit wide, then one pixel in Tight encoding is always represented by
 * three bytes, where the first byte is red component, the second byte is
 * green component, and the third byte is blue component of the pixel color
 * value. This applies to colors in palettes as well.
 *
 *-- NOTE 2. The decoder must reset compression streams' states before
 * decoding the rectangle, if some of bits 0,1,2,3 in the compression control
 * byte are set to 1. Note that the decoder must reset zlib streams even if
 * the compression type is "fill" or "jpeg".
 *
 *-- NOTE 3. The "gradient" filter and "jpeg" compression may be used only
 * when bits-per-pixel value is either 16 or 32, not 8.
 *
 *-- NOTE 4. The width of any Tight-encoded rectangle cannot exceed 2048
 * pixels. If a rectangle is wider, it must be split into several rectangles
 * and each one should be encoded separately.
 *
 */

#define rfbTightExplicitFilter         0x04
#define rfbTightFill                   0x08
#define rfbTightJpeg                   0x09
#define rfbTightMaxSubencoding         0x09

/* Filters to improve compression efficiency */
#define rfbTightFilterCopy             0x00
#define rfbTightFilterPalette          0x01
#define rfbTightFilterGradient         0x02


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * XCursor encoding. This is a special encoding used to transmit X-style
 * cursor shapes from server to clients. Note that for this encoding,
 * coordinates in rfbFramebufferUpdateRectHeader structure hold hotspot
 * position (r.x, r.y) and cursor size (r.w, r.h). If (w * h != 0), two RGB
 * samples are sent after header in the rfbXCursorColors structure. They
 * denote foreground and background colors of the cursor. If a client
 * supports only black-and-white cursors, it should ignore these colors and
 * assume that foreground is black and background is white. Next, two bitmaps
 * (1 bits per pixel) follow: first one with actual data (value 0 denotes
 * background color, value 1 denotes foreground color), second one with
 * transparency data (bits with zero value mean that these pixels are
 * transparent). Both bitmaps represent cursor data in a byte stream, from
 * left to right, from top to bottom, and each row is byte-aligned. Most
 * significant bits correspond to leftmost pixels. The number of bytes in
 * each row can be calculated as ((w + 7) / 8). If (w * h == 0), cursor
 * should be hidden (or default local cursor should be set by the client).
 */

typedef struct PACKED {
    uint8_t foreRed;
    uint8_t foreGreen;
    uint8_t foreBlue;
    uint8_t backRed;
    uint8_t backGreen;
    uint8_t backBlue;
} rfbXCursorColors;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbXCursorColors 6
#else
#define sz_rfbXCursorColors sizeof(rfbXCursorColors)
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  rfbEncodingARGBCursor encoding
 */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * WindowShape encoding
 */

typedef struct PACKED {
	uint32_t nrects;

	/* Followed by nrects * 4 * uint16_t, the clip for the shape
	   x_1, y_1, w_1, h_1, ... x_nrects, y_nrects, w_nrects, h_nrects */
} rfbWindowShapeHeader;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbWindowShapeHeader 4
#else
#define sz_rfbWindowShapeHeader sizeof(rfbWindowShapeHeader)
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Facade encoding
 */

typedef struct PACKED {
	uint32_t ncuts;

	/* Followed by ncut * 7 * uint32_t */
} rfbFacadesHeader;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbFacadesHeader 4
#else
#define sz_rfbFacadesHeader sizeof(rfbFacadesHeader)
#endif

/*-----------------------------------------------------------------------------
 * SetColourMapEntries - these messages are only sent if the pixel
 * format uses a "colour map" (i.e. trueColour false) and the client has not
 * fixed the entire colour map using FixColourMapEntries.  In addition they
 * will only start being sent after the client has sent its first
 * FramebufferUpdateRequest.  So if the client always tells the server to use
 * trueColour then it never needs to process this type of message.
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbSetColourMapEntries */
#if !defined(NO_RFB_PADDING_IN_METISSE)
    uint8_t pad;
#endif
    uint16_t firstColour;
    uint16_t nColours;

    /* Followed by nColours * 3 * uint16_t
       r1, g1, b1, r2, g2, b2, r3, g3, b3, ..., rn, bn, gn */

} rfbSetColourMapEntriesMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbSetColourMapEntriesMsg 6
#else
#define sz_rfbSetColourMapEntriesMsg sizeof(rfbSetColourMapEntriesMsg)
#endif

/*-----------------------------------------------------------------------------
 * Bell - ring a bell on the client if it has one.
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbBell */
} rfbBellMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbBellMsg 1
#else
#define sz_rfbBellMsg sizeof(rfbBellMsg)
#endif

/*-----------------------------------------------------------------------------
 * ServerCutText - the server has new text in its cut buffer.
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbServerCutText */
#if !defined(NO_RFB_PADDING_IN_METISSE)
    uint8_t pad1;
    uint16_t pad2;
#endif
    uint32_t length;
    /* followed by char text[length] */
} rfbServerCutTextMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbServerCutTextMsg 8
#else
#define sz_rfbServerCutTextMsg sizeof(rfbServerCutTextMsg)
#endif

/*-----------------------------------------------------------------------------
 * ConfigureWindow - 
 */

typedef struct PACKED {
	uint8_t type;			/* always rfbConfigureWindow */
#if !defined(NO_RFB_PADDING_IN_METISSE)
	uint8_t pad1;
	uint16_t pad2;
#endif
	uint32_t window;              /* window id */
	uint16_t xsgn;                /* 0: negative, 1: positive */
	uint16_t x;                   /* position */
	uint16_t ysgn;                /* 0: negative, 1: positive */
	uint16_t y;                   /* position */
	uint32_t width;               /* size of the window */
	uint32_t height;
	uint16_t isroot;              /* root window : 1, not root window : 0 */
} rfbConfigureWindowMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbConfigureWindowMsg 26
#else
#define sz_rfbConfigureWindowMsg sizeof(rfbConfigureWindowMsg)
#endif

/*-----------------------------------------------------------------------------
 * UnmapWindow - 
 */

typedef struct PACKED {
	uint8_t type;			/* always rfbUnmapWindow */
#if !defined(NO_RFB_PADDING_IN_METISSE)
	uint8_t pad1;
	uint16_t pad2;
#endif
	uint32_t window;              /* window id */
} rfbUnmapWindowMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbUnmapWindowMsg 8
#else
#define sz_rfbUnmapWindowMsg sizeof(rfbUnmapWindowMsg)
#endif

/*-----------------------------------------------------------------------------
 * DestroyWindow - 
 */

typedef struct PACKED {
	uint8_t type;			/* always rfbDestroyWindow */
#if !defined(NO_RFB_PADDING_IN_METISSE)
	uint8_t pad1;
	uint16_t pad2;
#endif
	uint32_t window;              /* window id */
} rfbDestroyWindowMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbDestroyWindowMsg 8
#else
#define sz_rfbDestroyWindowMsg sizeof(rfbDestroyWindowMsg)
#endif

/*-----------------------------------------------------------------------------
 * RestackWindow - 
 */

#define rfbWindowFlagsOverrideRedirect			(1 << 0)
#define rfbWindowFlagsInputOnly			        (1 << 1)
#define rfbWindowFlagsUnmanaged			        (1 << 2)
#define rfbWindowFlagsNetChecking			(1 << 3)
#define rfbWindowFlagsEwmhDesktop			(1 << 4)
#define rfbWindowFlagsTransient			        (1 << 5)
#define rfbWindowFlagsToolTips			        (1 << 6)
#define rfbWindowFlagsFacade                            (1 << 7)
#define rfbWindowFlagsDuplicate                         (1 << 8)
#define rfbWindowFlagsFacadeSaved                       (1 << 9)

typedef struct PACKED {
	uint8_t type;			/* always rfbRestackWindow */
#if !defined(NO_RFB_PADDING_IN_METISSE)
	uint8_t pad1;
	uint16_t pad2;
#endif
	uint32_t window;              /* window id */
	uint32_t nextWindow;
	uint32_t transientFor;
	uint32_t unmanagedFor;
	uint32_t grabWindow;
	uint32_t duplicateFor;
	uint32_t facadeReal;
	uint32_t flags;
} rfbRestackWindowMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbRestackWindowMsg 36
#else
#define sz_rfbRestackWindowMsg sizeof(rfbRestackWindowMsg)
#endif

/*****************************************************************************
 *
 * Message definitions (client -> server)
 *
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * SetPixelFormat - tell the RFB server the format in which the client wants
 * pixels sent.
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbSetPixelFormat */
#if !defined(NO_RFB_PADDING_IN_METISSE)
    uint8_t pad1;
    uint16_t pad2;
#endif
    rfbPixelFormat format;
} rfbSetPixelFormatMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbSetPixelFormatMsg (sz_rfbPixelFormat + 4)
#else
#define sz_rfbSetPixelFormatMsg sizeof(rfbSetPixelFormatMsg)
#endif

/*-----------------------------------------------------------------------------
 * FixColourMapEntries - when the pixel format uses a "colour map", fix
 * read-only colour map entries.
 *
 *    ***************** NOT CURRENTLY SUPPORTED *****************
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbFixColourMapEntries */
#if !defined(NO_RFB_PADDING_IN_METISSE)
    uint8_t pad;
#endif
    uint16_t firstColour;
    uint16_t nColours;

    /* Followed by nColours * 3 * uint16_t
       r1, g1, b1, r2, g2, b2, r3, g3, b3, ..., rn, bn, gn */

} rfbFixColourMapEntriesMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbFixColourMapEntriesMsg 6
#else
#define sz_rfbFixColourMapEntriesMsg sizeof(rfbFixColourMapEntriesMsg)
#endif

/*-----------------------------------------------------------------------------
 * SetEncodings - tell the RFB server which encoding types we accept.  Put them
 * in order of preference, if we have any.  We may always receive raw
 * encoding, even if we don't specify it here.
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbSetEncodings */
#if !defined(NO_RFB_PADDING_IN_METISSE)
    uint8_t pad;
#endif
    uint16_t nEncodings;
    /* followed by nEncodings * uint32_t encoding types */
} rfbSetEncodingsMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbSetEncodingsMsg 4
#else
#define sz_rfbSetEncodingsMsg sizeof(rfbSetEncodingsMsg)
#endif

/*-----------------------------------------------------------------------------
 * FramebufferUpdateRequest - request for a framebuffer update.  If incremental
 * is true then the client just wants the changes since the last update.  If
 * false then it wants the whole of the specified rectangle.
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbFramebufferUpdateRequest */
    uint8_t incremental;
#if !defined(NO_RFB_PADDING_IN_METISSE)
    uint16_t pad;
#endif
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} rfbFramebufferUpdateRequestMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbFramebufferUpdateRequestMsg 12
#else
#define sz_rfbFramebufferUpdateRequestMsg sizeof(rfbFramebufferUpdateRequestMsg)
#endif

/*-----------------------------------------------------------------------------
 * 
 */

typedef struct PACKED {
	uint8_t type;  /* always rfbWindowUpdateRequest */
	uint8_t incremental;
#if !defined(NO_RFB_PADDING_IN_METISSE)
  uint16_t pad2;
#endif
	uint32_t window;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
} rfbWindowUpdateRequestMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbWindowUpdateRequestMsg 16
#else
#define sz_rfbWindowUpdateRequestMsg sizeof(rfbWindowUpdateRequestMsg)
#endif

/*-----------------------------------------------------------------------------
 * KeyEvent - key press or release
 *
 * Keys are specified using the "keysym" values defined by the X Window System.
 * For most ordinary keys, the keysym is the same as the corresponding ASCII
 * value.  Other common keys are:
 *
 * BackSpace		0xff08
 * Tab			0xff09
 * Return or Enter	0xff0d
 * Escape		0xff1b
 * Insert		0xff63
 * Delete		0xffff
 * Home			0xff50
 * End			0xff57
 * Page Up		0xff55
 * Page Down		0xff56
 * Left			0xff51
 * Up			0xff52
 * Right		0xff53
 * Down			0xff54
 * F1			0xffbe
 * F2			0xffbf
 * ...			...
 * F12			0xffc9
 * Shift		0xffe1
 * Control		0xffe3
 * Meta			0xffe7
 * Alt			0xffe9
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbKeyEvent */
    uint8_t down;			/* true if down (press), false if up */
#if !defined(NO_RFB_PADDING_IN_METISSE)
    uint16_t pad;
#endif
    uint32_t key;			/* key is specified as an X keysym */
} rfbKeyEventMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbKeyEventMsg 8
#else
#define sz_rfbKeyEventMsg sizeof(rfbKeyEventMsg)
#endif

/*-----------------------------------------------------------------------------
 * PointerEvent - mouse/pen move and/or button press.
 */

typedef struct PACKED {
	uint8_t type;        /* always rfbPointerEvent */
#if !defined(NO_RFB_PADDING_IN_METISSE)
	uint8_t pad1;
	uint16_t pad2;
#endif
	uint32_t buttonMask;  /* bits 0-31 are buttons 1-32, 0=up, 1=down */
	uint8_t xsgn;                /* 0: negative, 1: positive */
#if !defined(NO_RFB_PADDING_IN_METISSE)
	uint8_t pad3;
#endif
	uint16_t x;
	uint8_t ysgn;              /* 0: negative, 1: positive */
#if !defined(NO_RFB_PADDING_IN_METISSE)
	uint8_t pad4;
#endif
	uint16_t y;
	uint32_t window;
} rfbPointerEventMsg;

#define rfbButton1Mask 1
#define rfbButton2Mask 2
#define rfbButton3Mask 4

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbPointerEventMsg 20
#else
#define sz_rfbPointerEventMsg sizeof(rfbPointerEventMsg)
#endif

/*-----------------------------------------------------------------------------
 * ClientCutText - the client has new text in its cut buffer.
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbClientCutText */
#if !defined(NO_RFB_PADDING_IN_METISSE)
    uint8_t pad1;
    uint16_t pad2;
#endif
    uint32_t length;
    /* followed by char text[length] */
} rfbClientCutTextMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbClientCutTextMsg 8
#else
#define sz_rfbClientCutTextMsg sizeof(rfbClientCutTextMsg)
#endif

/*****************************************************************************
 *
 * client <-> server message definitions
 *
 *****************************************************************************/
/*-----------------------------------------------------------------------------
 * NoOp - does nothing but the counter is useful for debugging
 */

typedef struct PACKED {
    uint8_t type;			/* always rfbNoOp */
    uint8_t ab;				/* always 0xAB */
    uint8_t cd;				/* always 0xCD */
    uint8_t ef;				/* always 0xEF */
    uint32_t counter;
} rfbNoOpMsg;

#if !defined(NO_RFB_PADDING_IN_METISSE)
#define sz_rfbNoOpMsg 8
#else
#define sz_rfbNoOpMsg sizeof(rfbNoOpMsg)
#endif

/*****************************************************************************
 *
 * message definition unions
 *
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Union of all server->client messages.
 */

typedef union {
	uint8_t type;
	rfbFramebufferUpdateMsg fu;
	rfbSetColourMapEntriesMsg scme;
	rfbBellMsg b;
	rfbServerCutTextMsg sct;
	rfbConfigureWindowMsg cw;
	rfbUnmapWindowMsg uw;
	rfbDestroyWindowMsg dw;
	rfbRestackWindowMsg rw;
	rfbNoOpMsg no;
} rfbServerToClientMsg;

/*-----------------------------------------------------------------------------
 * Union of all client->server messages.
 */

typedef union {
	uint8_t type;
	rfbSetPixelFormatMsg spf;
	rfbFixColourMapEntriesMsg fcme;
	rfbSetEncodingsMsg se;
	rfbFramebufferUpdateRequestMsg fur;
	rfbKeyEventMsg ke;
	rfbPointerEventMsg pe;
	rfbClientCutTextMsg cct;
	rfbWindowUpdateRequestMsg wur;
	rfbNoOpMsg no;
} rfbClientToServerMsg;

  /*-----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
