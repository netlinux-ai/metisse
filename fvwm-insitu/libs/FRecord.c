/* -*-c-*- */
/* Copyright (C) 2004  Olivier Chapuis */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *
 * A lot of code from xnee (http://www.gnu.org/software/xnee/)
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Henrik Sandklef
 *
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include "safemalloc.h"
#include "flist.h"
#include "FRecord.h"

/* ---------------------------- local definitions -------------------------- */


/* ---------------------------- local macros ------------------------------- */

#define VERBOSE(x)
#define SYNC_VERBOSE(x)
#define SUPER_VERBOSE(x)

#ifndef max
#define max(a,b) ((a > b)? a:b)
#endif

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static Bool FRecordExtensionSupported = False;
static int FRecordErrorBase = -10000;
static int FRecordMajorOpCode = -10000;

static Bool FRecordExtensionInitialized = False;

static flist *FRecordDescriptionList = NULL;

static Display *display_control = None;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static
int FRecordGetMaxRange(FRecordRecordDescription *frd) 
{
	int i=0;
	int max_val=0;

	for ( i=0 ; i < FRECORD_NR_OF_TYPES ; i++)
	{
		max_val = max(frd->data_ranges[i], max_val);
	}

	return max_val;
}

static
void FRecordDummyDispatcher(XPointer xpointer_data, FRecordInterceptData *data)
{
	FRecordRecordDescription *frd =
		(FRecordRecordDescription *)xpointer_data;
	frd->has_dispatched = True;
}

/* ---------------------------- interface functions ------------------------ */

Bool FRecordInit(Display *dpy)
{
	int event_basep;

	if (FRecordExtensionInitialized)
	{
		return FRecordExtensionSupported;
	}

	FRecordExtensionInitialized = True;

	if(!XRecordSupport || !XQueryExtension(
		   dpy, "RECORD", &FRecordMajorOpCode, &event_basep,
		   &FRecordErrorBase))
	{
		fprintf(stderr,"[FVWM]: WARN -- No RECORD X extension\n");
		return FRecordExtensionSupported;
	}

	VERBOSE((stderr, "Record extension Initialized %i %i %i\n",
		 FRecordMajorOpCode, event_basep, FRecordErrorBase));

	FRecordExtensionSupported = True;
	return FRecordExtensionSupported;
}

int FRecordGetErrorCodeBase(void)
{
	return FRecordErrorBase;
}

int FRecordGetMajorOpCode(void)
{
	return FRecordMajorOpCode;
}

Bool FRecordGetExtensionSupported(Display *dpy)
{
	if (!FRecordExtensionInitialized)
	{
		return FRecordInit(dpy);
	}
	return FRecordExtensionSupported;
}


/*
 *
 */

int FRecordAddRange (
	FRecordRecordDescription *frd, FRecordDataType type, int start, int stop)
{
	int max_index = 0;
	int alloc_nr;
	FRecordRange *r_range;

	if (!XRecordSupport || frd == NULL)
	{
		return 0;
	}

	if (type > FRECORD_NR_OF_TYPES)
	{
		fprintf(stderr,
			"[FRecordAddRange] -- ERROR: type out of range \n");
		return 0;
	}

	VERBOSE((stderr, "[FRecordAddRange] add range %d %d %d\n", type, start, 
		 stop));

	/* increment the counter to ensure we allocate enough memory */
	alloc_nr = frd->data_ranges[type]  ;
	max_index = FRecordGetMaxRange(frd) - 1 ;

	if (alloc_nr > max_index)
	{
		if (alloc_nr == 0) 
		{
			frd->range_array =
				(FRecordRange**)calloc(1, sizeof(FRecordRange*));
		}
		else 
		{
			frd->range_array = 
				(FRecordRange**)realloc(
					frd->range_array,
					(alloc_nr+1)*sizeof(FRecordRange*));
		}
		r_range =  FRecordAllocRange();
		memset (r_range, 0, sizeof(FRecordRange));
		frd->range_array[alloc_nr] = r_range; 
	}

  

	/* is it single value */
	if (stop==0) 
	{
		stop=start;
	}

	if (type == FRECORD_DELIVERED_EVENT) 
	{
		frd->range_array[alloc_nr]->delivered_events.first = start;
		frd->range_array[alloc_nr]->delivered_events.last = stop;

		/* Workaround for problem with crashing X server*/
		frd->data_ranges[FRECORD_ERROR]++;
		frd->range_array[alloc_nr]->errors.first = BadCursor;
		frd->range_array[alloc_nr]->errors.last = BadCursor;
	}
	else if (type == FRECORD_REQUEST) 
	{
		frd->range_array[alloc_nr]->core_requests.first = start;
		frd->range_array[alloc_nr]->core_requests.last = stop;
	}
	else if (type == FRECORD_ERROR)
	{
		frd->range_array[alloc_nr]->errors.first = start;
		frd->range_array[alloc_nr]->errors.last = stop;

		/* Workaround for problem with crashing X server*/
		frd->range_array[alloc_nr]->delivered_events.first =33 ;
		frd->range_array[alloc_nr]->delivered_events.last = 33;
		frd->data_ranges[FRECORD_DELIVERED_EVENT]++;
	}
	else if (type == FRECORD_REPLY) 
	{
		frd->range_array[alloc_nr]->core_replies.first = start;
		frd->range_array[alloc_nr]->core_replies.last = stop;
	}
	else if (type == FRECORD_EXT_REQUEST_MAJOR) 
	{
		frd->range_array[alloc_nr]->ext_requests.ext_major.first = start;
		frd->range_array[alloc_nr]->ext_requests.ext_major.last = stop;
	}
	else if (type == FRECORD_EXT_REQUEST_MINOR) 
	{
		frd->range_array[alloc_nr]->ext_requests.ext_minor.first = start;
		frd->range_array[alloc_nr]->ext_requests.ext_minor.last = stop;
	}
	else if (type == FRECORD_EXT_REPLY_MAJOR) 
	{
		frd->range_array[alloc_nr]->ext_replies.ext_major.first = start;
		frd->range_array[alloc_nr]->ext_replies.ext_major.last = stop;
	}
	else if (type == FRECORD_EXT_REPLY_MINOR) 
	{
		frd->range_array[alloc_nr]->ext_replies.ext_minor.first = start;
		frd->range_array[alloc_nr]->ext_replies.ext_minor.last = stop;
	}
	else if (type == FRECORD_DEVICE_EVENT) 
	{
		frd->range_array[alloc_nr]->device_events.first = start;
		frd->range_array[alloc_nr]->device_events.last = stop;
	}

 
	frd->data_ranges[type]++;
	return 1;
  
}

FRecordRecordDescription *FRecordCreateDescription(Display *dpy)
{
	FRecordRecordDescription *frd = NULL;
	int i;


	if (!XRecordSupport || !FRecordInit(dpy))
	{
		return NULL;
	}

	frd = (FRecordRecordDescription *)safemalloc(
		sizeof(FRecordRecordDescription));

	memset(frd, 0, sizeof(FRecordRecordDescription));

	if (display_control == None &&
	    !(display_control = XOpenDisplay("")))
	{
		fprintf(stderr,"[FRecordCreateDescription] Error -- "
			"Cannot open a connection to the X server\n");
		free(frd);
		return NULL;	
	}

	if (!(frd->dpy_data = XOpenDisplay("")))
	{
		
		fprintf(stderr,"[FRecordCreateDescription] Error -- "
			"Cannot open a connection to the X server\n");
		free(frd);
		return NULL;
	}
	frd->dpy_control = display_control;

	VERBOSE((stderr,"[FRecordStartRecording] displays: %i %i\n",
		(int)frd->dpy_control, (int)frd->dpy_data));

	/* setup some default */

	frd->data_flags =
		FRecordFromServerTime |
		FRecordFromClientTime |
		FRecordFromClientSequence;

	frd->xids = (FRecordClientSpec *)safemalloc(sizeof(FRecordClientSpec *));
	frd->xids[0] = FRecordAllClients;
	frd->nclients = 1;

	frd->dispatch = FRecordDummyDispatcher;
	frd->closure = (XPointer)frd;

	frd->range_array = NULL;
	frd->do_record = False;

	for (i=0 ; i < FRECORD_NR_OF_TYPES ; i++)
	{
		frd->data_ranges[i] = 0;
	}

	FRecordDescriptionList = flist_append_obj(FRecordDescriptionList, frd);

	return frd;
}

void FRecordStartRecording(FRecordRecordDescription *frd)
{
	if (!XRecordSupport || frd == NULL)
	{
		return;
	}

	if (frd->do_record)
	{
		fprintf(stderr,"[FRecordStartRecording] Warning -- "
			"Recording already started\n");
		return;
	}

	if (frd->Context == None)
	{
		frd->Context = FRecordCreateContext(
			frd->dpy_control, frd->data_flags, 
			frd->xids, frd->nclients, 
			frd->range_array, FRecordGetMaxRange(frd));
		if (frd->Context == None)
		{
			fprintf(stderr,"[FRecordStartRecording] Error -- "
				"FRecordCreateContext failed\n");
			return;
		}
	}

	SYNC_VERBOSE((stderr,"[FRecordStartRecording] Before XSync\n"));
	XSync(frd->dpy_control, True);
	SYNC_VERBOSE((stderr,"[FRecordStartRecording] After XSync\n"));

	if (!FRecordEnableContextAsync(
		    frd->dpy_data, frd->Context, frd->dispatch, frd->closure))
	{
		fprintf(stderr,"[FRecordStartRecording] Error -- "
			"FRecordEnableContextAsync failed\n");
		return;
	}
	SYNC_VERBOSE((stderr,"[FRecordStartRecording] Started\n"));

	frd->do_record = True;
}

void FRecordStopRecording(FRecordRecordDescription *frd)
{
	if (!XRecordSupport || frd == NULL)
	{
		return;
	}

	if (!frd->do_record)
	{
		fprintf(stderr,"[FRecordStopRecording] Warning -- "
			"We do not record\n");
		return;
	}

	if (frd->Context == None)
	{
		fprintf(stderr,"[FRecordStopRecording] Warning -- "
			"No Recording Context\n");
		return;
	}

	if (!FRecordDisableContext(frd->dpy_control, frd->Context))
	{
		fprintf(stderr,"[FRecordStopRecording] Error -- "
			"FRecordDisableContext failed\n");
		return;
	}
	SYNC_VERBOSE((stderr,"[FRecordStopRecording] Before XSync\n"));
	XSync(frd->dpy_control, True);
	SYNC_VERBOSE((stderr,"[FRecordStopRecording] Stoped\n"));
	frd->do_record = False;
}

void FRecordFreeRecordDescription(FRecordRecordDescription *frd)
{

	if (!XRecordSupport || frd == NULL)
	{
		return;
	}

	XSync(frd->dpy_control, True);
	if (frd->Context != None)
	{
		if(!FRecordFreeContext(frd->dpy_control, frd->Context))
		{
			fprintf(stderr,"[FRecordFreeRecordDescription] Error -- "
				"FRecordFreeContext failed\n");
		}
		XSync(frd->dpy_control, True);
	}
	free(frd->xids);
	if (frd->range_array != NULL)
	{
		free(frd->range_array);
	}
	XCloseDisplay(frd->dpy_data);

	FRecordDescriptionList = flist_remove_obj(FRecordDescriptionList, frd);
	free(frd);
}

void FRecordMyProcessReplies(FRecordRecordDescription *frd)
{
	if (!XRecordSupport || frd == NULL)
	{
		return;
	}

	if (frd->dpy_data != None)
	{
		SUPER_VERBOSE((stderr,"[FRecordMyProcessReplies] ..."));
		FRecordProcessReplies(frd->dpy_data);
		SUPER_VERBOSE((stderr," done\n"));
	}
}

/*
 * list stuff
 */

void FRecordProcessRepliesAll(void)
{
	flist *l = FRecordDescriptionList;

	if (!XRecordSupport)
	{
		return;
	}

	while(l != NULL)
	{
		FRecordRecordDescription *frd =
			(FRecordRecordDescription *)l->object;

		if (frd->do_record)
		{
			FRecordMyProcessReplies(frd);
		}
		l = l->next;
	}
}

Bool FRecordShouldProcessReplies(void)
{
	flist *l = FRecordDescriptionList;

	if (!XRecordSupport)
	{
		return False;
	}

	while(l != NULL)
	{
		FRecordRecordDescription *frd =
			(FRecordRecordDescription *)l->object;

		if (frd->do_record)
		{
			return True;
		}
		l = l->next;
	}
	return False;
}

void FRecordPrintListInfo(void)
{
	flist *l = FRecordDescriptionList;
	int i = 1;

	if (!XRecordSupport)
	{
		fprintf(stderr,"No RECORD Support\n");
		return;
	}

	while(l != NULL)
	{
		FRecordRecordDescription *frd =
			(FRecordRecordDescription *)l->object;
		fprintf(stderr,"Record %i:\n", i);
		fprintf(stderr,"  Do Record: %i\n", frd->do_record);
		/* etc ... */
		l = l->next;
		i++;
	}
	if (i == 1)
	{
		fprintf(stderr,"No Record\n");	
	}
}

/*
 * some specialized frd
 */

FRecordRecordDescription *FRecordCreateDescriptionS(
	Display *dpy, unsigned long mask)
{

	FRecordRecordDescription *frd;

	if (!XRecordSupport)
	{
		return NULL;
	}

	frd = FRecordCreateDescription(dpy);

	if (frd == NULL)
	{
		return NULL;
	}

	if (mask & ButtonReleaseMask)
	{
		FRecordAddRange(
			frd, FRECORD_DELIVERED_EVENT, ButtonRelease,
			ButtonRelease);
	}
	if (mask & KeyPressMask)
	{
		FRecordAddRange(
			frd, FRECORD_DELIVERED_EVENT, KeyPressMask,
			KeyPressMask);
	}

	return frd;
}
