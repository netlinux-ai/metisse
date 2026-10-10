/* -*-c-*- */
/*
 *
 * fvwm/Mfvwm.H --
 *
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef MFVWM_H
#define MFVWM_H

#ifdef HAVE_FVWM

extern "C" {
#define FEVENT_C
#undef PACKAGE
#undef VERSION
#include "libs/defaults.h"
#include "libs/Module.h"
//#include "fvwm_fvwm.h"
#define INCLUDE_FROM_ELSEWERE
#include "fvwm/fvwm.h"
#include "libs/vpacket.h"
#undef PACKAGE
#undef VERSION
}

#else

typedef struct ConfigWinPacket
{
	Window             w;
	Window             frame;
	Window             *fvwmwin; // not true
	unsigned long int  frame_x;
	unsigned long int  frame_y;
	unsigned long int  frame_width;
	unsigned long int  frame_height;
	unsigned long int  desk;
	unsigned long int  layer;

	unsigned short int title_height;
	unsigned short int border_width;
	unsigned long int  hints_base_width;
	unsigned long int  hints_base_height;
	unsigned long int  hints_width_inc;
	unsigned long int  hints_height_inc;
	unsigned long int  hints_min_width;
	unsigned long int  hints_min_height;
	unsigned long int  hints_max_width;
	unsigned long int  hints_max_height;
	Window             icon_w;
	Window             icon_pixmap_w;
	unsigned long int  hints_win_gravity;
	unsigned long int  TextPixel;
	unsigned long int  BackPixel;
	unsigned long int  ewmh_hint_layer;
	unsigned long int  ewmh_hint_desktop;
	unsigned long int  ewmh_window_type;

	unsigned long int flags; // not true

} ConfigWinPacket;

#define IS_ICONIFIED(x) 0
#define IS_STICKY_ACROSS_DESKS(x) 1

#endif

#endif
