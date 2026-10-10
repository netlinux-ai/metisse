/* -*-c-*- */
/* Copyright (C) 2005  Olivier Chapuis */
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

#include "config.h"

#include <stdio.h>

#include <X11/keysym.h>

#include "libs/fvwmlib.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"

#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "execcontext.h"
#include "commands.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "commands.h"
#include "ewmh.h"
#include "decorations.h"
#include "functions.h"
#include "fvwmsignal.h"
#include "execcontext.h"
#include "geometry.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

extern Window PressedW;

/* ---------------------------- local functions ---------------------------- */

static int get_best_fold_cursor(FvwmWindow *fw, Window w)
{
	if (FW_W_SIDE(fw,0) == w)
	{
		return CRS_FOLD_TOP;
	}
	else if (FW_W_SIDE(fw,1) == w)
	{
		return CRS_FOLD_RIGHT;
	}
	else if (FW_W_SIDE(fw,2) == w)
	{
		return CRS_FOLD_BOTTOM;
	}
	else if (FW_W_SIDE(fw,3) == w)
	{
		return CRS_FOLD_LEFT;
	}
	else if (FW_W_CORNER(fw,0) == w)
	{
		return CRS_FOLD_TOP_LEFT;
	}
	else if (FW_W_CORNER(fw,1) == w)
	{
		return CRS_FOLD_TOP_RIGHT;
	}
	else if (FW_W_CORNER(fw,2) == w)
	{
		return CRS_FOLD_BOTTOM_LEFT;
	}
	else if (FW_W_CORNER(fw,3) == w)
	{
		return CRS_FOLD_BOTTOM_RIGHT;
	}

	return CRS_FOLD;
	
}


/* ---------------------------- interface functions ------------------------ */

void metisse_handle_module_input(void)
{
	extern fd_set_size_t fd_width;
	extern int x_fd;
	fd_set in_fdset, out_fdset;
	Window targetWindow;
	int num_fd;
	int i;

	FD_ZERO(&in_fdset);
	FD_ZERO(&out_fdset);
	FD_SET(x_fd, &in_fdset);

	for (i=0; i<npipes; i++)
	{
		if (readPipes[i]>=0)
		{
			FD_SET(readPipes[i], &in_fdset);
		}
		if (!FQUEUE_IS_EMPTY(&pipeQueue[i]))
		{
			FD_SET(writePipes[i], &out_fdset);
		}
	}

	num_fd = fvwmSelect(
		fd_width, &in_fdset, &out_fdset, 0, NULL);

	if (num_fd > 0)
	{
		for (i=0; i<npipes; i++)
		{
			if ((readPipes[i] >= 0) &&
			    FD_ISSET(readPipes[i], &in_fdset))
			{
				if (read(readPipes[i], &targetWindow,
					 sizeof(Window)) > 0)
				{
					/* Add one module message to the queue */
					HandleModuleInput(
						targetWindow, i, NULL, True);
				}
				else
				{
					KillModule(i);
				}
			}
			if ((writePipes[i] >= 0) &&
			    FD_ISSET(writePipes[i], &out_fdset))
			{
				FlushMessageQueue(i);
			}
		}

		/* execute any commands queued up */
		ExecuteCommandQueue();
	}
}

void metisse_check_pointer_pos(exec_context_t *pexc, int *x, int *y)
{
	FvwmWindow *fw,*ofw;
	rectangle ocg, cg;
	flist *l;
	win_cut_t *found_wc = NULL;

	if (!Scr.bo.MetisseWorkaround ||
	    pexc->w.ofw == NULL || pexc->w.fw == NULL)
	{
		return;
	}

	fw = pexc->w.fw;
	ofw = pexc->w.ofw;
	
	get_client_geometry(fw, &cg);
	get_client_geometry(ofw, &ocg);

	if (fw->duplicateFor == FW_W_FRAME(ofw))
	{
		/* FIXME: check that *x,*y is inside ocg, but if this not the
		 * case we should not be here ... for now! */
		*x = *x - (ocg.x - cg.x);
		*y = *y - (ocg.y - cg.y);
		pexc->w.ofw = NULL;
		return;
	}

	if (fw->facade == NULL || !IS_FACADE(fw))
	{
		return;
	}

	l = fw->facade;
	
	while(l != NULL)
	{
		win_cut_t *wc = (win_cut_t *)l->object;

		if (FW_W_FRAME(ofw) != wc->win)
		{
			l = l->next;
			continue;
		}
		/* coordinate of the source is vs the frame */
		if (*x > wc->sx + ofw->frame_g.x &&
		    *x <= wc->sx + ofw->frame_g.x + wc->sw &&
		    *y > wc->sy + ofw->frame_g.y &&
		    *y <= wc->sy + ofw->frame_g.y + wc->sh)
		{
			found_wc = wc;
			break;
		}
		l = l->next;
	}

	if (found_wc == NULL)
	{
		return;
	}

	pexc->w.ofw = NULL;
	/* ... and coordinate of the destination is vs the client */
	*x = *x - (ofw->frame_g.x + found_wc->sx - cg.x - found_wc->dx);
	*y = *y - (ofw->frame_g.y + found_wc->sy - cg.y - found_wc->dy);
}

Bool metisse_is_unmap_forbided(FvwmWindow *fw)
{
	FvwmWindow *t;
	int count = 0;
	int i = 0;
	Window *wl;

	if (!Scr.bo.MetisseWorkaround)
	{
		return False;
	}

	if (IS_DUPLICATE(fw) || IS_FACADE(fw))
	{
		/* no recursivness with facades */
		return False;
	}

	if (IS_FACADE_SOURCE(fw))
	{
		return True;
	}
	/* counting */
	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (t->duplicateFor != 0)
		{
			count++;
		}
		else if (t->facade != NULL)
		{
			flist *l = t->facade;
			while(l != NULL)
			{
				if (l->object)
				{
					count++;
				}
				l = l->next;
			}
		}
	}

	if (count == 0)
	{
		return False;
	}

	wl = (unsigned long *)safemalloc(count*sizeof(unsigned long));
	/* store */
	i = 0;
	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (t->duplicateFor != 0)
		{
			wl[i++] = t->duplicateFor;
		}
		else if (t->facade != NULL)
		{
			flist *l = t->facade;
			while(l != NULL)
			{
				win_cut_t *wc = (win_cut_t *)l->object;
				if (wc)
				{
					wl[i++] = wc->win;
				}
				l = l->next;
			}
		}
	}

	/* check */
	for (i = 0; i < count; i++)
	{
		if (FW_W(fw) == wl[i] || FW_W_FRAME(fw) == wl[i])
		{
			free(wl);
			return True;
		}
	}

	free(wl);
	return False;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_MetisseSelectedWindow(F_CMD_ARGS)
{
	char *token;

	/* Get window ID */
	action = GetNextToken(action, &token);

	if (token)
	{
		/* SunOS doesn't have strtoul */
		Scr.MetisseSelectedWin = (unsigned long)strtol(token, NULL, 0);
		free(token);
	}
	else
	{
		Scr.MetisseSelectedWin = 0;
	}
	
	/* fprintf(
	   stderr, "Metisse Selection: 0x%lx\n", Scr.MetisseSelectedWin); */

}

void CMD_MetisseTransform(F_CMD_ARGS)
{
	FvwmWindow * const fw = exc->w.fw;
	unsigned long data0, data1, data2;
	char *arg = NULL;
	char *rest;
	
	rest = GetNextToken(action, &arg);
	action = SkipSpaces(action, NULL, 0);
	/* check correctness ?? */
	{
		float f[18];
		int num;
		
		if (StrEquals(arg,"Set") || StrEquals(arg,"State") ||
		    StrEquals(arg,"Mark"))
		{
			/* ok */
		}
		else
		{
			fvwm_msg(
				ERR, "MetisseTransform",
				"First arg should be State, Mark or Set");
			goto end;
		}
		num = sscanf(
			rest,
			"%f %f %f %f %f %f "
			"%f %f %f %f %f %f "
			"%f %f %f %f %f %f",
			&f[0], &f[1], &f[2], &f[3], &f[4], &f[5],
			&f[6], &f[7], &f[8], &f[9], &f[10], &f[11],
			&f[12], &f[13], &f[14], &f[15], &f[16], &f[17]);
		if (num != 18)
		{
			fvwm_msg(
				ERR, "MetisseTransform",
				"Take 18 float arguments");
			goto end;
		}
	}

	/* Forward to modules */
	if (fw)
	{
		/* Modules may need to know which window this applies to */
		data0 = FW_W(fw);
		data1 = FW_W_FRAME(fw);
		data2 = (unsigned long)fw;
	}
	else
	{
		data0 = 0;
		data1 = 0;
		data2 = 0;
	}
	if (StrEquals(arg,"State"))
	{
		BroadcastName(MX_METISSE_TRANSFORM, data0, data1, data2, rest);
	}
	else if (StrEquals(arg,"Mark"))
	{
		BroadcastName(
			MX_METISSE_TRANSFORM_MARK, data0, data1, data2, rest);
	}
	else /* Set */
	{
		BroadcastName(
			MX_METISSE_SET_TRANSFORM, data0, data1, data2, rest);
	}

 end:
	if (arg)
	{
		free(arg);
	}
}

/*
 *
 * ModuleWindowOperation Module OpType Cursor rest
 *
 */

void CMD_ModuleWindowOperation(F_CMD_ARGS)
{
	char *module_name = NULL;
	char *cursor_name = NULL;
	char *op_type = NULL;
	char *rest = NULL;
	char cmd[1024];
	int cursor;
	unsigned int button_mask = 0;
	FvwmWindow *fw = exc->w.fw;
	Window w;
	int origDragX, origDragY, dupDragX, dupDragY, DragX, DragY;
	int FinalX, FinalY, XOffset, YOffset;
	/* int xl, yt; */
	unsigned int DragWidth, DragHeight;
	Bool keyboard_grabed = False;
	Bool finished = False;
	unsigned int net_op = _NET_START_STOP_METISSE_WINDOW_OP_UNKNOWN;
	unsigned int net_end = _NET_START_STOP_DONE;
	XEvent e;

	action = GetNextToken(action, &module_name);
	if (module_name == NULL || *module_name == '\0')
	{
		fvwm_msg(WARN,"ModuleWindowOperation", "No arguments, abort");
		goto done;
	}
	action = GetNextToken(action, &op_type);
	if (op_type == NULL || *op_type == '\0')
	{
		fvwm_msg(WARN,"ModuleWindowOperation", "No op type, abort");
		goto done;
	}
	action = GetNextToken(action, &cursor_name);
	if (cursor_name == NULL || *cursor_name == '\0')
	{
		fvwm_msg(WARN,"ModuleWindowOperation", "No cursor name, abort");
		goto done;
	}

	if (StrEquals(op_type, "Move"))
	{
		if (!is_function_allowed(F_MOVE, NULL, fw, True, False))
		{
			fvwm_msg(WARN,"ModuleWindowOperation",
				 "Move not allowed");
			goto done;
		}
		net_op = _NET_START_STOP_METISSE_INTERACTIVE_MOVE;
	}

	/* gotta have a window */
	w = FW_W_FRAME(fw);
	if (IS_ICONIFIED(fw))
	{
		if (FW_W_ICON_PIXMAP(fw) != None)
		{
			w = FW_W_ICON_PIXMAP(fw);
			/* ? do not care for now */
			XUnmapWindow(dpy,FW_W_ICON_TITLE(fw));
		}
		else
		{
			w = FW_W_ICON_TITLE(fw);
		}
	}

	fev_get_evpos_or_query(
		dpy, Scr.Root, exc->x.elast, &DragX, &DragY);
	metisse_check_pointer_pos((exec_context_t *)exc, &DragX, &DragY);

	MyXGrabServer(dpy);
	if (!XGetGeometry(
		    dpy, w, &JunkRoot, &origDragX, &origDragY,
		    (unsigned int *)&DragWidth, (unsigned int *)&DragHeight,
		    &JunkBW,  &JunkDepth))
	{
		MyXUngrabServer(dpy);
		fvwm_msg(WARN,"ModuleWindowOperation",
			 "XGetGeometry fails");
		goto done;
	}

	if (fw->duplicateFor != 0 && exc->w.ofw != 0)
	{
		if (!XGetGeometry(
			    dpy, fw->duplicateFor, &JunkRoot,
			    &dupDragX, &dupDragY,
			    (unsigned int *)&DragWidth,
			    (unsigned int *)&DragHeight,
			    &JunkBW,  &JunkDepth))
		{
			MyXUngrabServer(dpy);
			fvwm_msg(WARN,"ModuleWindowOperation",
				 "XGetGeometry fails");
			goto done;
		}
		#if 0
		fprintf(stderr,"Translat %i %i\n",
			origDragX - dupDragX, origDragY- dupDragY);
		#endif
		#if 0
		DragX = DragX + (origDragX - dupDragX);
		DragY = DragY + (origDragY - dupDragY);
		#endif
	}
	MyXUngrabServer(dpy);

	MyXGrabKeyboard(dpy);
	keyboard_grabed = True;

	FinalX = origDragX;
	FinalY = origDragY;
	XOffset = origDragX - DragX;
	YOffset = origDragY - DragY;
	
	if (StrEquals(cursor_name, "Fold"))
	{
		cursor = get_best_fold_cursor(fw, PressedW);
		net_op = _NET_START_STOP_METISSE_INTERACTIVE_FOLD;
	}
	else
	{
		cursor = cursor_name_to_index(cursor_name);
	}
	
	if (!GrabEm(cursor, GRAB_NORMAL))
	{
		XBell(dpy, 0);
		fvwm_msg(WARN,"ModuleWindowOperation",
			 "GrabEm Fail");
		goto done;
	}
#if 0
	if (FQueryPointer(
		    dpy, Scr.Root, &JunkRoot, &JunkChild, &xl, &yt,
		    &JunkX, &JunkY, &button_mask) == False)
	{
		/* pointer is on a different screen */
		xl = 0;
		yt = 0;
	}
	else
	{
		xl += XOffset;
		yt += YOffset;
	}
	fprintf(stderr,"QueryPointer %i %i / %i %i\n",
		xl, yt, XOffset, YOffset);
#endif

	EWMH_SendStartStopOperation(FW_W(fw), True, net_op, 0);
	sprintf(
		cmd, "SendToModule %s Operation %s %i %i %s", module_name,
		op_type, DragX, DragY, (action)? action:"");
	execute_function_override_window(NULL, NULL, cmd, 0, fw);

	memset(&e, 0, sizeof(e));
	while (!finished)
	{
		/* block until an event arrives */
		/* FIXME: we should be able to handle module input */
		if (!FCheckMaskEvent(
			    dpy,
			    ButtonPressMask | ButtonReleaseMask | KeyPressMask,
			    &e))
		{
			metisse_handle_module_input();
		}
		else
		{
			switch (e.type)
			{
			case KeyPress:
			{
				KeySym keysym = XLookupKeysym(&(e.xkey),0);

				if (keysym == XK_Escape ||
				    keysym == XK_Return ||
				    keysym == XK_KP_Enter ||
				    keysym == XK_space)
				{
					finished = True;
				}
				if (keysym == XK_Escape)
				{
					net_end = _NET_START_STOP_CANCEL;
				}
				break;
			}
			case ButtonPress:
				if (e.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS
				    &&
				    ((Button1Mask << (e.xbutton.button - 1)) &
				     button_mask))
				{
					/* No new button was pressed, just a
					 * delayed event */
					break;
				}
				if ((e.xbutton.button == 2 &&
				     !Scr.gs.do_emulate_mwm) ||
				    (e.xbutton.button == 1 &&
				     Scr.gs.do_emulate_mwm &&
				     (e.xbutton.state & ShiftMask)))
				{
					/* Fallthrough to button-release */
				}
				else if (e.xbutton.button == 3)
				{
					/* Fallthrough to button-release */
				}
				else
				{
					/* Abort the move if
					 *  - the move started with a pressed
					 *  button and another button was pressed
					 *  during the operation
					 *  - no button was pressed at the
					 *  beginning and any button except
					 *  button 1 was pressed. */
					if (button_mask ||
					    (e.xbutton.button != 1))
					{
						finished = True;
					}
					break;
				}
			case ButtonRelease:
				finished = True;
				break;
			default:
				/* cannot happen */
				break;
			}
		}
	}

	if (IS_ICONIFIED(fw))
	{
		SET_ICON_MOVED(fw, 1);
	}
	UngrabEm(GRAB_NORMAL);
	EWMH_SendStartStopOperation(FW_W(fw), False, net_op, net_end);


 done:
	if (keyboard_grabed)
	{
		MyXUngrabKeyboard(dpy);
	}
	if (module_name)
		free(module_name);
	if (cursor_name)
		free(cursor_name);
	if (op_type)
		free(op_type);
	if (rest)
		free(rest);	
}


/* Not useful anymore ?? */
void CMD_ModuleInteractiveMove(F_CMD_ARGS)
{
	int toggle;

	toggle = ParseToggleArgument(action, &action, 1, False);

	switch(toggle)
	{
	case 1:
		Scr.ModuleInteractiveMove = True;
		break;
	case 0:
		Scr.ModuleInteractiveMove = False;
		break;
	case -1:
		if (Scr.ModuleInteractiveMove)
			Scr.ModuleInteractiveMove = False;
		else
			Scr.ModuleInteractiveMove = True;
		break;
	default:
		/* should not happen */
		break;
	}
	return;
}

