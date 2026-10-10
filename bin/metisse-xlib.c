#if HAVE_CONFIG_H
#include "config-not-xserver.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>


Display *_dpy;
Atom _facadeAtom = None;
Atom _subFacadeAtom = None;
Atom _rmFacadeAtom = None;

void subfacade_property(Window id)
{
	XClientMessageEvent ev;

	ev.type = ClientMessage;
	ev.window = id;
	ev.message_type = _subFacadeAtom;
	ev.format = 32;
	ev.data.l[0] = 1;
	ev.data.l[1] = 0;
	ev.data.l[2] = 0;
	ev.data.l[3] = 0;
	ev.data.l[4] = 0;
			
	/* fprintf(stderr,"metisse-xlib: subfacade_property\n"); */
	XSendEvent (
		_dpy, DefaultRootWindow(_dpy), False,
		(SubstructureNotifyMask| SubstructureRedirectMask),
		(XEvent *) &ev);
	XFlush(_dpy);
}

void addfacade_pointer(
	Window dest_id, Window dest_cid, long pointer_x, long pointer_y)
{
	XClientMessageEvent ev;

	ev.type = ClientMessage;
	ev.window = dest_id;
	ev.message_type = _facadeAtom;
	ev.format = 32;

	ev.data.l[0] = 0;
	ev.data.l[1] = (long)pointer_x;
	ev.data.l[2] = (long)pointer_y;
	ev.data.l[3] = 0;
	ev.data.l[4] = 0;
	/* fprintf(stderr,"metisse-xlib: addfacade_pointer\n"); */
	XSendEvent(
		_dpy, dest_cid, True, 0L, (XEvent *) &ev);
	XFlush(_dpy);
	XSync(_dpy, False);
}

void addfacade(
	Window dest_id,  Window dest_cid, Window src_id,
	long x, long y, long w, long h)
{
	XClientMessageEvent ev;

	ev.type = ClientMessage;
	ev.window = dest_id;
	ev.message_type = _facadeAtom;
	ev.format = 32;

	ev.data.l[0] = src_id;
	ev.data.l[1] = (long)x;
	ev.data.l[2] = (long)y;
	ev.data.l[3] = (long)w;
	ev.data.l[4] = (long)h;
	/* fprintf(stderr,"metisse-xlib: addfacade\n"); */
	XSendEvent(
		_dpy, dest_cid, True, 0L, (XEvent *) &ev);
	XFlush(_dpy);
	XSync(_dpy, False);
}

void rmfacade(
	Window dest_id, Window dest_cid, Window src_id,
	long x, long y, long w, long h)
{
	XClientMessageEvent ev;

	ev.type = ClientMessage;
	ev.window = dest_id;
	ev.message_type = _rmFacadeAtom;
	ev.format = 32;

	ev.data.l[0] = src_id;
	ev.data.l[1] = (long)x;
	ev.data.l[2] = (long)y;
	ev.data.l[3] = (long)w;
	ev.data.l[4] = (long)h;
	/* fprintf(stderr,"metisse-xlib: rmfacade\n"); */
	XSendEvent(
		_dpy, dest_cid, True, 0L, (XEvent *) &ev);
	XFlush(_dpy);
	XSync(_dpy, False);
}

void set_name(Window dest_id, char *name)
{
	XTextProperty text_prop;

	if (XStringListToTextProperty(&name, 1, &text_prop) != 0) 
	{
		XSetWMName(_dpy,  dest_id, &text_prop);
		XSetWMIconName(_dpy,  dest_id, &text_prop);
		XSync(_dpy, True);
		/* fprintf(stderr,"metisse-xlib: set_name\n"); */
	}
}

int main(int argc, char * argv[])
{
	char *display_name = NULL;
	int i;
	unsigned long id,src_id, dest_id, dest_cid;
	long px,py,x,y,w,h;

	if ((_dpy = XOpenDisplay(display_name)) == NULL)
	{
		fprintf(
			stderr, "%s: couldn't connect to X server %s\n",
			argv[0], display_name);
		exit(1);
	}

	if (strcasecmp(argv[1], "SubFacadeProperty") == 0)
	{
		_subFacadeAtom = XInternAtom(
			_dpy, "_METISSE_NET_WM_FACADE_SOURCE", False);
		for (i = 2; i < argc; i++)
		{
			id = strtoul(argv[i], NULL, 0);
			subfacade_property((Window)id);
		}
	}
	else if (strcasecmp(argv[1], "AddFacade") == 0)
	{
		_facadeAtom = XInternAtom(
			_dpy, "_METISSE_NET_WM_FACADE", False);

		dest_id = strtoul(argv[2], NULL, 0);
		dest_cid = strtoul(argv[3], NULL, 0);
		src_id = strtoul(argv[4], NULL, 0);
		px = strtol(argv[5], NULL, 0);
		py = strtol(argv[6], NULL, 0);
		x = strtol(argv[7], NULL, 0);
		y = strtol(argv[8], NULL, 0);
		w = strtol(argv[9], NULL, 0);
		h = strtol(argv[10], NULL, 0);
		addfacade_pointer((Window)dest_id, (Window)dest_cid, px, py);
		addfacade(
			(Window)dest_id, (Window)dest_cid, (Window)src_id,
			x, y, w, h);
	}
	else if (strcasecmp(argv[1], "RmFacade") == 0)
	{
		_rmFacadeAtom = XInternAtom(
			_dpy, "_METISSE_NET_WM_FACADE_REMOVE", False);

		dest_id = strtoul(argv[2], NULL, 0);
		dest_cid = strtoul(argv[3], NULL, 0);
		src_id = strtoul(argv[4], NULL, 0);
		x = strtol(argv[5], NULL, 0);
		y = strtol(argv[6], NULL, 0);
		w = strtol(argv[7], NULL, 0);
		h = strtol(argv[8], NULL, 0);
		rmfacade(
			(Window)dest_id, (Window)dest_cid, (Window)src_id,
			x, y, w, h);
	}
	else if (strcasecmp(argv[1], "SetName") == 0)
	{
		dest_id = strtoul(argv[2], NULL, 0);
		set_name(dest_id, argv[3]);
	}
	exit(0);
}
