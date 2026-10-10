/*
 * kbdptr.c - deal with keyboard and pointer device over TCP & UDP.
 *
 *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "mi.h"
#include "mipointer.h"

#include "X11/X.h"
#define NEED_EVENTS
#include "X11/Xproto.h"
#include "inputstr.h"
#define XK_CYRILLIC
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include "XF86keysym.h"

#include "rfb.h"

extern WindowPtr *WindowTable; /* Why isn't this in a header file? */

#define KEY_IS_PRESSED(keycode) \
    (kbdDevice->key->down[(keycode) >> 3] & (1 << ((keycode) & 7)))


static void XConvertCase(KeySym sym, KeySym *lower, KeySym *upper);

static DeviceIntPtr kbdDevice;

unsigned char ptrAcceleration = 50;

#define MIN_KEY_CODE		8
#define MAX_KEY_CODE		255
#define NO_OF_KEYS		(MAX_KEY_CODE - MIN_KEY_CODE + 1)
#define GLYPHS_PER_KEY		2

static KeySym kbdMap[] = {
/* first the shift key */
    XK_Shift_L,		NoSymbol,
#define SHIFT_L_KEY_CODE	(MIN_KEY_CODE)
    XK_Shift_R,		NoSymbol,
#define SHIFT_R_KEY_CODE	(MIN_KEY_CODE + 1)
#if 1
    XK_a,		XK_A,
    XK_b,		XK_B,
    XK_c,		XK_C,
    XK_d,		XK_D,
    XK_e,		XK_E,
    XK_f,		XK_F,
    XK_g,		XK_G,
    XK_h,		XK_H,
    XK_i,		XK_I,
    XK_j,		XK_J,
    XK_k,		XK_K,
    XK_l,		XK_L,
    XK_m,		XK_M,
    XK_n,		XK_N,
    XK_o,		XK_O,
    XK_p,		XK_P,
    XK_q,		XK_Q,
    XK_r,		XK_R,
    XK_s,		XK_S,
    XK_t,		XK_T,
    XK_u,		XK_U,
    XK_v,		XK_V,
    XK_w,		XK_W,
    XK_x,		XK_X,
    XK_y,		XK_Y,
    XK_z,		XK_Z,
#endif
/* These are directly mapped from DOS scanset 0 */
/*      1     8 */	 XK_Escape, NoSymbol,
/*      2     9 */	 XK_1,	XK_exclam,
/*      3    10 */	 XK_2,	XK_at,
/*      4    11 */	 XK_3,	XK_numbersign,
/*      5    12 */	 XK_4,	XK_dollar,
/*      6    13 */	 XK_5,	XK_percent,
/*      7    14 */	 XK_6,	XK_asciicircum,
/*      8    15 */	 XK_7,	XK_ampersand,
/*      9    16 */	 XK_8,	XK_asterisk,
/*     10    17 */	 XK_9,	XK_parenleft,
/*     11    18 */	 XK_0,	XK_parenright,
/*     12    19 */	 XK_minus,	XK_underscore,
/*     13    20 */	 XK_equal,	XK_plus,
/*     14    21 */	 XK_BackSpace,	NoSymbol,
/*     15    22 */	 XK_Tab,	NoSymbol,
#if 0
/*     16    23 */	 XK_Q,	NoSymbol,
/*     17    24 */	 XK_W,	NoSymbol,
/*     18    25 */	 XK_E,	NoSymbol,
/*     19    26 */	 XK_R,	NoSymbol,
/*     20    27 */	 XK_T,	NoSymbol,
/*     21    28 */	 XK_Y,	NoSymbol,
/*     22    29 */	 XK_U,	NoSymbol,
/*     23    30 */	 XK_I,	NoSymbol,
/*     24    31 */	 XK_O,	NoSymbol,
/*     25    32 */	 XK_P,	NoSymbol,
#endif
/*     26    33 */	 XK_bracketleft,	XK_braceleft,
/*     27    34 */	 XK_bracketright,	XK_braceright,
/*     28    35 */	 XK_Return,	NoSymbol,
/*     29    36 */	 XK_Control_L,	NoSymbol,
#if 0
/*     30    37 */	 XK_A,	NoSymbol,
/*     31    38 */	 XK_S,	NoSymbol,
/*     32    39 */	 XK_D,	NoSymbol,
/*     33    40 */	 XK_F,	NoSymbol,
/*     34    41 */	 XK_G,	NoSymbol,
/*     35    42 */	 XK_H,	NoSymbol,
/*     36    43 */	 XK_J,	NoSymbol,
/*     37    44 */	 XK_K,	NoSymbol,
/*     38    45 */	 XK_L,	NoSymbol,
#endif
/*     39    46 */	 XK_semicolon,	XK_colon,
/*     40    47 */	 XK_apostrophe,	XK_quotedbl,
/*     41    48 */	 XK_grave,	XK_asciitilde,
#if 0
/*     42    49 */	 XK_Shift_L,	NoSymbol,
#endif
/*     43    50 */	 XK_backslash,	XK_bar,
#if 0
/*     44    51 */	 XK_Z,	NoSymbol,
/*     45    52 */	 XK_X,	NoSymbol,
/*     46    53 */	 XK_C,	NoSymbol,
/*     47    54 */	 XK_V,	NoSymbol,
/*     48    55 */	 XK_B,	NoSymbol,
/*     49    56 */	 XK_N,	NoSymbol,
/*     50    57 */	 XK_M,	NoSymbol,
#endif
/*     51    58 */	 XK_comma,	XK_less,
/*     52    59 */	 XK_period,	XK_greater,
/*     53    60 */	 XK_slash,	XK_question,
/*     54    61 */	 /* XK_Shift_R,	NoSymbol, */
/*     55    62 */	 XK_KP_Multiply,	NoSymbol,
/*     56    63 */	 XK_Alt_L,	XK_Meta_L,
/*     57    64 */	 XK_space,	NoSymbol,
/*     58    65 */	 XK_Caps_Lock,	NoSymbol,
/*     59    66 */	 XK_F1,	NoSymbol,
/*     60    67 */	 XK_F2,	NoSymbol,
/*     61    68 */	 XK_F3,	NoSymbol,
/*     62    69 */	 XK_F4,	NoSymbol,
/*     63    70 */	 XK_F5,	NoSymbol,
/*     64    71 */	 XK_F6,	NoSymbol,
/*     65    72 */	 XK_F7,	NoSymbol,
/*     66    73 */	 XK_F8,	NoSymbol,
/*     67    74 */	 XK_F9,	NoSymbol,
/*     68    75 */	 XK_F10,	NoSymbol,
/*     69    76 */	 XK_Break,	XK_Pause,
/*     70    77 */	 XK_Scroll_Lock,	NoSymbol,
/*     71    78 */	 XK_KP_Home,	XK_KP_7,
/*     72    79 */	 XK_KP_Up,	XK_KP_8,
/*     73    80 */	 XK_KP_Page_Up,	XK_KP_9,
/*     74    81 */	 XK_KP_Subtract,	NoSymbol,
/*     75    82 */	 XK_KP_Left,	XK_KP_4,
/*     76    83 */	 XK_KP_5,	NoSymbol,
/*     77    84 */	 XK_KP_Right,	XK_KP_6,
/*     78    85 */	 XK_KP_Add,	NoSymbol,
/*     79    86 */	 XK_KP_End,	XK_KP_1,
/*     80    87 */	 XK_KP_Down,	XK_KP_2,
/*     81    88 */	 XK_KP_Page_Down,	XK_KP_3,
/*     82    89 */	 XK_KP_Insert,	XK_KP_0,
/*     83    90 */	 XK_KP_Delete,	XK_KP_Decimal,
/*     84    91 */     NoSymbol,	NoSymbol,
/*     85    92 */     NoSymbol,	NoSymbol,
/*     86    93 */     NoSymbol,	NoSymbol,
/*     87    94 */	 XK_F11,	NoSymbol,
/*     88    95 */	 XK_F12,	NoSymbol,
    
/* These are remapped from the extended set (using ExtendMap) */
    
/*     89    96 */	 XK_Control_R,	NoSymbol,
/*     90    97 */	 XK_KP_Enter,	NoSymbol,
/*     91    98 */	 XK_KP_Divide,	NoSymbol,
/*     92    99 */	 XK_Sys_Req,	XK_Print,
/*     93   100 */	 XK_Alt_R,	XK_Meta_R,
/*     94   101 */	 XK_Num_Lock,	NoSymbol,
/*     95   102 */	 XK_Home,	NoSymbol,
/*     96   103 */	 XK_Up,		NoSymbol,
/*     97   104 */	 XK_Page_Up,	NoSymbol,
/*     98   105 */	 XK_Left,	NoSymbol,
/*     99   106 */	 XK_Right,	NoSymbol,
/*    100   107 */	 XK_End,	NoSymbol,
/*    101   108 */	 XK_Down,	NoSymbol,
/*    102   109 */	 XK_Page_Down,	NoSymbol,
/*    103   110 */	 XK_Insert,	NoSymbol,
/*    104   111 */	 XK_Delete,	NoSymbol,
/*    105   112 */	 XK_Super_L,	NoSymbol,
/*    106   113 */	 XK_Super_R,	NoSymbol,
/*    107   114 */	 XK_Menu,	NoSymbol,
/*    108   115 */	 NoSymbol,	NoSymbol,
/*    109   116 */	 NoSymbol,	NoSymbol,
/*    110   117 */	 NoSymbol,	NoSymbol,
/*    111   118 */	 NoSymbol,	NoSymbol,
/*    112   119 */	 NoSymbol,	NoSymbol,
    XF86XK_Mail, NoSymbol,
    XF86XK_WWW, NoSymbol,
    XF86XK_Launch0, NoSymbol,
    XF86XK_Launch1, NoSymbol,
    XF86XK_Launch2, NoSymbol,
};

#define N_PREDEFINED_KEYS (sizeof(kbdMap) / (sizeof(KeySym) * GLYPHS_PER_KEY))


/* stolen from kdrive */

typedef struct {
    KeySym  modsym;
    int	    modbit;
} KeySymModsRec;

const KeySymModsRec KeySymMods[] = {
  {  XK_Control_L,	ControlMask },
  {  XK_Control_R, ControlMask },
  {  XK_Shift_L,	ShiftMask },
  {  XK_Shift_R,	ShiftMask },
  {  XK_Caps_Lock,	LockMask },
  {  XK_Shift_Lock, LockMask },
  {  XK_Alt_L,	Mod1Mask },
  {  XK_Alt_R,	Mod1Mask },
  {  XK_Meta_L,	Mod1Mask },
  {  XK_Meta_R,	Mod1Mask },
  {  XK_Num_Lock,	Mod2Mask },
  {  XK_Super_L,	Mod4Mask },
  {  XK_Super_R,	Mod4Mask },
  {  XK_Hyper_L,	Mod4Mask },
  {  XK_Hyper_R,	Mod4Mask },
  {  XK_Mode_switch,    Mod5Mask },
};

#define NUM_SYM_MODS (sizeof(KeySymMods) / sizeof(KeySymMods[0]))

static void
InitModMap (CARD8 *pModMap)
{
    int	    key_code;
    int	    row;
    int	    width;
    KeySym  *syms;
    int	    i;

    width = GLYPHS_PER_KEY;
    for (key_code = MIN_KEY_CODE; key_code <= MAX_KEY_CODE; key_code++)
    {
	pModMap[key_code] = 0;
	syms = kbdMap + (key_code - MIN_KEY_CODE) * width;
	for (row = 0; row < width; row++, syms++)
	{
	    for (i = 0; i < NUM_SYM_MODS; i++) 
	    {
		if (*syms == KeySymMods[i].modsym) 
		    pModMap[key_code] |= KeySymMods[i].modbit;
	    }
	}
    }
}

void
PtrDeviceInit()
{
}


void
KbdDeviceInit(pDevice, pKeySyms, pModMap)
    DeviceIntPtr pDevice;
    KeySymsPtr pKeySyms;
    CARD8 *pModMap;
{
    int i;

    kbdDevice = pDevice;

    for (i = 0; i < MAP_LENGTH; i++)
	pModMap[i] = NoSymbol;

    InitModMap (pModMap);

    pKeySyms->minKeyCode = MIN_KEY_CODE;
    pKeySyms->maxKeyCode = MAX_KEY_CODE;
    pKeySyms->mapWidth = GLYPHS_PER_KEY;

    pKeySyms->map = (KeySym *)xalloc(sizeof(KeySym)
				     * MAP_LENGTH * GLYPHS_PER_KEY);

    if (!pKeySyms->map) {
	rfbLog("xalloc failed\n");
	exit(1);
    }

    for (i = 0; i < MAP_LENGTH * GLYPHS_PER_KEY; i++)
	pKeySyms->map[i] = NoSymbol;

    for (i = 0; i < N_PREDEFINED_KEYS * GLYPHS_PER_KEY; i++) {
	pKeySyms->map[i] = kbdMap[i];
    }
}



void
KbdDeviceOn()
{
}


void
KbdDeviceOff()
{
}


void
PtrDeviceOn(pDev)
    DeviceIntPtr pDev;
{
    ptrAcceleration = (char)pDev->ptrfeed->ctrl.num;
}


void
PtrDeviceOff()
{
}


void
PtrDeviceControl(dev, ctrl)
    DevicePtr dev;
    PtrCtrl *ctrl;
{
	if (!ctrl)
          return;

	ptrAcceleration = (char)ctrl->num;
	if (udpSockConnected) {
		if (write(udpSock, &ptrAcceleration, 1) <= 0) {
			rfbLogPerror("PtrDeviceControl: UDP input: write");
			rfbDisconnectUDPSock();
		}
	}
}


void
KbdAddEvent(down, keySym, cl)
    Bool down;
    KeySym keySym;
    rfbClientPtr cl;
{
    xEvent ev, fake;
    KeySymsPtr keySyms = &kbdDevice->key->curKeySyms;
    int i;
    int keyCode = 0;
    int freeIndex = -1;
    unsigned long time;
    Bool fakeShiftPress = FALSE;
    Bool fakeShiftLRelease = FALSE;
    Bool fakeShiftRRelease = FALSE;
    Bool shiftMustBeReleased = FALSE;
    Bool shiftMustBePressed = FALSE;

    if (down) {
	ev.u.u.type = KeyPress;
    } else {
	ev.u.u.type = KeyRelease;
    }

    /* First check if it's one of our predefined keys.  If so then we can make
       some attempt at allowing an xmodmap inside a METISSE desktop behave
       something like you'd expect - e.g. if keys A & B are swapped over and
       the METISSE client sends an A, then map it to a B when generating the X
       event.  We don't attempt to do this for keycodes which we make up on the
       fly because it's too hard... */

    for (i = 0; i < N_PREDEFINED_KEYS * GLYPHS_PER_KEY; i++) {
	if (keySym == kbdMap[i]) {
	    keyCode = MIN_KEY_CODE + i / GLYPHS_PER_KEY;

	    if (kbdMap[(i/GLYPHS_PER_KEY) * GLYPHS_PER_KEY + 1] != NoSymbol) {

		/* this keycode has more than one symbol associated with it,
		   so shift state is important */
		if ((i % GLYPHS_PER_KEY) == 0)
		    shiftMustBeReleased = TRUE;
		else
		    shiftMustBePressed = TRUE;
	    }
	    break;
	}
    }

    if (!keyCode) {

	/* not one of our predefined keys - see if it's in the current keyboard
           mapping (i.e. we've already allocated an extra keycode for it) */

	if (keySyms->mapWidth < 2) {
	    rfbLog("KbdAddEvent: Sanity check failed - Keyboard mapping has "
		   "less than 2 keysyms per keycode (KeySym 0x%x)\n", keySym);
	    return;
	}

	for (i = 0; i < NO_OF_KEYS * keySyms->mapWidth; i++) {
	    if (keySym == keySyms->map[i]) {
		keyCode = MIN_KEY_CODE + i / keySyms->mapWidth;

		if (keySyms->map[(i / keySyms->mapWidth)
					* keySyms->mapWidth + 1] != NoSymbol) {

		    /* this keycode has more than one symbol associated with
		       it, so shift state is important */

		    if ((i % keySyms->mapWidth) == 0)
			shiftMustBeReleased = TRUE;
		    else
			shiftMustBePressed = TRUE;
		}
		break;
	    }
	    if ((freeIndex == -1) && (keySyms->map[i] == NoSymbol)
		&& (i % keySyms->mapWidth) == 0)
	    {
		freeIndex = i;
	    }
	}
    }

    if (!keyCode) {
	KeySym lower, upper;

	/* we don't have an existing keycode - make one up on the fly and add
	   it to the keyboard mapping.  Thanks to Vlad Harchev for pointing
	   out problems with non-ascii capitalisation. */

	if (freeIndex == -1) {
	    rfbLog("KbdAddEvent: ignoring KeySym 0x%x - no free KeyCodes\n",
		   keySym);
	    return;
	}

	keyCode = MIN_KEY_CODE + freeIndex / keySyms->mapWidth;

	XConvertCase(keySym, &lower, &upper);

	if (lower == upper) {
	    keySyms->map[freeIndex] = keySym;

	} else {
	    keySyms->map[freeIndex] = lower;
	    keySyms->map[freeIndex+1] = upper;

	    if (keySym == lower)
		shiftMustBeReleased = TRUE;
	    else
		shiftMustBePressed = TRUE;
	}

	SendMappingNotify(MappingKeyboard, keyCode, 1, serverClient);

	rfbLog("KbdAddEvent: unknown KeySym 0x%x - allocating KeyCode %d\n",
	       keySym, keyCode);
    }

    time = GetTimeInMillis();

    if (down) {
	if (shiftMustBePressed && !(kbdDevice->key->state & ShiftMask)) {
	    fakeShiftPress = TRUE;
	    fake.u.u.type = KeyPress;
	    fake.u.u.detail = SHIFT_L_KEY_CODE;
	    fake.u.keyButtonPointer.time = time;
	    mieqEnqueue(&fake);
	}
	if (shiftMustBeReleased && (kbdDevice->key->state & ShiftMask)) {
	    if (KEY_IS_PRESSED(SHIFT_L_KEY_CODE)) {
		fakeShiftLRelease = TRUE;
		fake.u.u.type = KeyRelease;
		fake.u.u.detail = SHIFT_L_KEY_CODE;
		fake.u.keyButtonPointer.time = time;
		mieqEnqueue(&fake);
	    }
	    if (KEY_IS_PRESSED(SHIFT_R_KEY_CODE)) {
		fakeShiftRRelease = TRUE;
		fake.u.u.type = KeyRelease;
		fake.u.u.detail = SHIFT_R_KEY_CODE;
		fake.u.keyButtonPointer.time = time;
		mieqEnqueue(&fake);
	    }
	}
    }

    ev.u.u.detail = keyCode;
    ev.u.keyButtonPointer.time = time;
    mieqEnqueue(&ev);

    if (fakeShiftPress) {
	fake.u.u.type = KeyRelease;
	fake.u.u.detail = SHIFT_L_KEY_CODE;
	fake.u.keyButtonPointer.time = time;
	mieqEnqueue(&fake);
    }
    if (fakeShiftLRelease) {
	fake.u.u.type = KeyPress;
	fake.u.u.detail = SHIFT_L_KEY_CODE;
	fake.u.keyButtonPointer.time = time;
	mieqEnqueue(&fake);
    }
    if (fakeShiftRRelease) {
	fake.u.u.type = KeyPress;
	fake.u.u.detail = SHIFT_R_KEY_CODE;
	fake.u.keyButtonPointer.time = time;
	mieqEnqueue(&fake);
    }
}

void
PtrAddEvent(unsigned long buttonMask, Window win, int x, int y, rfbClientPtr cl)
{
	xEvent ev;
	int i;
	unsigned long time;
	static int oldButtonMask = 0;

	time = GetTimeInMillis();

	miPointerAbsoluteCursorOverWindow(win, x, y, time);

	if (buttonMask == oldButtonMask)
	{
		return;
	}

	for (i = 0; i < 31; i++)
	{
		if ((buttonMask ^ oldButtonMask) & (1<<i))
		{
			if (buttonMask & (1<<i))
			{
				ev.u.u.type = ButtonPress;
				ev.u.u.detail = i + 1;
				ev.u.keyButtonPointer.time = time;
				ev.u.keyButtonPointer.event = win;
				mieqEnqueue(&ev);
			}
			else
			{
				ev.u.u.type = ButtonRelease;
				ev.u.u.detail = i + 1;
				ev.u.keyButtonPointer.time = time;
				ev.u.keyButtonPointer.event = win;
				mieqEnqueue(&ev);
			}
		}
	}

	oldButtonMask = buttonMask;
}

void
KbdReleaseAllKeys()
{
    int i, j;
    xEvent ev;
    unsigned long time = GetTimeInMillis();

    for (i = 0; i < DOWN_LENGTH; i++) {
	if (kbdDevice->key->down[i] != 0) {
	    for (j = 0; j < 8; j++) {
		if (kbdDevice->key->down[i] & (1 << j)) {
		    ev.u.u.type = KeyRelease;
		    ev.u.u.detail = (i << 3) | j;
		    ev.u.keyButtonPointer.time = time;
		    mieqEnqueue(&ev);
		}
	    }
	}
    }
}


/* copied from Xlib source */

static void XConvertCase(KeySym sym, KeySym *lower, KeySym *upper)
{
    *lower = sym;
    *upper = sym;
    switch(sym >> 8) {
    case 0: /* Latin 1 */
	if ((sym >= XK_A) && (sym <= XK_Z))
	    *lower += (XK_a - XK_A);
	else if ((sym >= XK_a) && (sym <= XK_z))
	    *upper -= (XK_a - XK_A);
	else if ((sym >= XK_Agrave) && (sym <= XK_Odiaeresis))
	    *lower += (XK_agrave - XK_Agrave);
	else if ((sym >= XK_agrave) && (sym <= XK_odiaeresis))
	    *upper -= (XK_agrave - XK_Agrave);
	else if ((sym >= XK_Ooblique) && (sym <= XK_Thorn))
	    *lower += (XK_oslash - XK_Ooblique);
	else if ((sym >= XK_oslash) && (sym <= XK_thorn))
	    *upper -= (XK_oslash - XK_Ooblique);
	break;
    case 1: /* Latin 2 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym == XK_Aogonek)
	    *lower = XK_aogonek;
	else if (sym >= XK_Lstroke && sym <= XK_Sacute)
	    *lower += (XK_lstroke - XK_Lstroke);
	else if (sym >= XK_Scaron && sym <= XK_Zacute)
	    *lower += (XK_scaron - XK_Scaron);
	else if (sym >= XK_Zcaron && sym <= XK_Zabovedot)
	    *lower += (XK_zcaron - XK_Zcaron);
	else if (sym == XK_aogonek)
	    *upper = XK_Aogonek;
	else if (sym >= XK_lstroke && sym <= XK_sacute)
	    *upper -= (XK_lstroke - XK_Lstroke);
	else if (sym >= XK_scaron && sym <= XK_zacute)
	    *upper -= (XK_scaron - XK_Scaron);
	else if (sym >= XK_zcaron && sym <= XK_zabovedot)
	    *upper -= (XK_zcaron - XK_Zcaron);
	else if (sym >= XK_Racute && sym <= XK_Tcedilla)
	    *lower += (XK_racute - XK_Racute);
	else if (sym >= XK_racute && sym <= XK_tcedilla)
	    *upper -= (XK_racute - XK_Racute);
	break;
    case 2: /* Latin 3 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Hstroke && sym <= XK_Hcircumflex)
	    *lower += (XK_hstroke - XK_Hstroke);
	else if (sym >= XK_Gbreve && sym <= XK_Jcircumflex)
	    *lower += (XK_gbreve - XK_Gbreve);
	else if (sym >= XK_hstroke && sym <= XK_hcircumflex)
	    *upper -= (XK_hstroke - XK_Hstroke);
	else if (sym >= XK_gbreve && sym <= XK_jcircumflex)
	    *upper -= (XK_gbreve - XK_Gbreve);
	else if (sym >= XK_Cabovedot && sym <= XK_Scircumflex)
	    *lower += (XK_cabovedot - XK_Cabovedot);
	else if (sym >= XK_cabovedot && sym <= XK_scircumflex)
	    *upper -= (XK_cabovedot - XK_Cabovedot);
	break;
    case 3: /* Latin 4 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Rcedilla && sym <= XK_Tslash)
	    *lower += (XK_rcedilla - XK_Rcedilla);
	else if (sym >= XK_rcedilla && sym <= XK_tslash)
	    *upper -= (XK_rcedilla - XK_Rcedilla);
	else if (sym == XK_ENG)
	    *lower = XK_eng;
	else if (sym == XK_eng)
	    *upper = XK_ENG;
	else if (sym >= XK_Amacron && sym <= XK_Umacron)
	    *lower += (XK_amacron - XK_Amacron);
	else if (sym >= XK_amacron && sym <= XK_umacron)
	    *upper -= (XK_amacron - XK_Amacron);
	break;
    case 6: /* Cyrillic */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Serbian_DJE && sym <= XK_Serbian_DZE)
	    *lower -= (XK_Serbian_DJE - XK_Serbian_dje);
	else if (sym >= XK_Serbian_dje && sym <= XK_Serbian_dze)
	    *upper += (XK_Serbian_DJE - XK_Serbian_dje);
	else if (sym >= XK_Cyrillic_YU && sym <= XK_Cyrillic_HARDSIGN)
	    *lower -= (XK_Cyrillic_YU - XK_Cyrillic_yu);
	else if (sym >= XK_Cyrillic_yu && sym <= XK_Cyrillic_hardsign)
	    *upper += (XK_Cyrillic_YU - XK_Cyrillic_yu);
        break;
    case 7: /* Greek */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Greek_ALPHAaccent && sym <= XK_Greek_OMEGAaccent)
	    *lower += (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
	else if (sym >= XK_Greek_alphaaccent && sym <= XK_Greek_omegaaccent &&
		 sym != XK_Greek_iotaaccentdieresis &&
		 sym != XK_Greek_upsilonaccentdieresis)
	    *upper -= (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
	else if (sym >= XK_Greek_ALPHA && sym <= XK_Greek_OMEGA)
	    *lower += (XK_Greek_alpha - XK_Greek_ALPHA);
	else if (sym >= XK_Greek_alpha && sym <= XK_Greek_omega &&
		 sym != XK_Greek_finalsmallsigma)
	    *upper -= (XK_Greek_alpha - XK_Greek_ALPHA);
        break;
    }
}
