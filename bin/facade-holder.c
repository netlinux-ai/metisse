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

#include "libutils/utils.h"
#include "libutils/flist.h"
#include "libutils/safemalloc.h"

#ifndef max
#define max(a,b) ((a>b)? a:b)
#endif
#ifndef min
#define min(a,b) ((a<b)? a:b)
#endif


Display *_dpy;
static char *_appname;
Window _win;
unsigned int _width, _height;
Atom _type;
Bool _hasPos = False;
Bool _saved = False;
unsigned long _facade_real = 0;
int _usX = 0;
int _usY = 0;

char *_dupliacte_Name = "Duplicate";
char *_facade_Name = "Facade";
char *_optName = NULL;
Bool _doLayoutAtStartUp = True;

#define METISSE_WIN_TYPE_DUPLICATE 0
#define METISSE_WIN_TYPE_FACADE    1

Atom ATOM_WM_DELETE_WINDOW = None;
Atom _METISSE_NET_WM_DUPLICATE = None;
Atom _METISSE_NET_WM_FACADE = None;
Atom _METISSE_NET_WM_FACADE_REMOVE = None;
Atom _METISSE_NET_WM_FACADE_SAVED = None;

Atom _WM_STATE = None;
#define XIA(a) XInternAtom(_dpy,a,False)

int Development = 1;

typedef struct
{
	XID fid;  /* frame id */
	XID cid;  /* window client id */
	int src_x;
	int src_y;
	int src_w;
	int src_h;
	int dest_x;
	int dest_y;
	int lb; /* line break */
	int sl; /* start line */
} cut;

flist *Facade = NULL;


int myErrorHandler(Display *dpy, XErrorEvent *e)
{
	char msg[256];

	/* some errors are acceptable BadAtom? */

	if (e->error_code == BadWindow ||
	    e->request_code == X_GetGeometry ||
	    e->error_code == BadDrawable)
	{
		return 0;
	}

	XGetErrorText(dpy, e->error_code, msg, sizeof(msg));
	fprintf(stderr, "%s XError: %d (%s)\n", _appname, e->error_code, msg);
	fprintf(stderr, "   Major opcode of failed request:  %d\n",
		e->request_code);
	fprintf(stderr, "   Minor opcode of failed request:  %d \n",
		e->minor_code);
	fprintf(stderr, "   Resource id of failed request:  0x%lx \n",
		e->resourceid);

	if (Development)
	{
		fprintf(stderr, " Leaving a core dump now\n");
		abort();
		/* exit if this fails */
		exit(99);
	}

	return 0 ;
}

char *get_window_title(Display *disp, Window win, int *is_net)
{
	char *name;
	*is_net = 0;
	XTextProperty text_prop;
	char **list;
	int num;

	if (XGetWMName(disp, win, &text_prop) != 0 &&
	    text_prop.encoding == XA_STRING)
	{
		/* STRING encoding, use this as it is */
		name = (char *)text_prop.value;
		return name;
	}

	/* not STRING encoding, try to convert XA_COMPOUND_TEXT */
	if (XmbTextPropertyToTextList(disp, &text_prop, &list, &num) >= Success
	    && num > 0 && *list)
	{
		/* Does not consider the conversion is REALLY succeeded:
		 * XmbTextPropertyToTextList return 0 (== Success) on success,
		 * a negative int if it fails (and in this case we are not
		 * here), the number of unconvertible char on "partial"
		 * success*/
		XFree(text_prop.value); /* return of XGetWM(Icon)Name() */
		name = strdup(*list);
		/*ret_name->name_list = list;*/
	}
	else
	{
		name = (char *)text_prop.value;
	}
	if (list)
	{
		XFreeStringList(list);
	}
	if (name == NULL)
	{
		name = strdup("NoTitle");
	}
	else
	{
		/* ok */
	}
	return name;
}



void *get_simple_property(
	Display *disp, Window win, Atom atom, Atom type)
{
	unsigned int size;
	void *p;

	p = utils_atom_get_property(disp, win, atom, type, &size);
	return p;
}

Bool get_wm_state(Display *disp, Window wind, unsigned long *state)
{
	Atom *val;
	if ((val = (Atom *)get_simple_property(
		     disp, wind, _WM_STATE, _WM_STATE)) == NULL)
	{
		return False;
	}

	*state = (Atom)val[0];
	free(val);

	return True;
}

Bool get_client_window(Display *disp, Window wind, Window *tl)
{
	Window junkroot;
	Window parent;
	Window *filles = NULL;
	unsigned int nfilles;
	int i;
	unsigned long s;

	if (get_wm_state(disp, wind, &s))
	{
		*tl = wind;
		return True;
	}

	if (!XQueryTree(disp, wind, &junkroot, &parent, &filles, &nfilles))
	{
		if (filles)
			XFree(filles);
		return False;
	}

	for(i = 0; i < nfilles; i++)
	{
		if (get_wm_state(disp, filles[i], &s))
		{
			*tl = filles[i];
			if (filles)
				XFree(filles);
			return True;
		}
	}
	for(i = 0; i < nfilles; i++)
	{
		if (get_client_window(disp, filles[i], tl))
		{
			if (filles)
				XFree(filles);
			return True;
		}
	}
	if (filles)
		XFree(filles);

	return False;
}

Window get_toplevel_window(Display *disp, Window wind)
{
	Window junkroot;
	Window parent;
	Window *filles = NULL;
	unsigned int nfilles;
	Window toplevel = wind;

	while(XQueryTree(disp, toplevel, &junkroot, &parent, &filles, &nfilles)
	      && parent != None && parent != junkroot)
		/*&& !isVirtualRoot(disp, parent) */
	{
		toplevel = parent;
		if (filles)
		{
			XFree(filles);
		}
		filles = NULL;
	}

	if (filles)
	{
		XFree(filles);
	}

	return toplevel;
}

/*
 *
 */


cut *get_cut(XID fid, int src_x, int src_y, int src_w, int src_h)
{
	flist *l = Facade;

	while(l != NULL)
	{
		cut *c = (cut *)l->object;
		if (c->fid == fid &&  c->src_x == src_x &&
		    c->src_y == src_y &&  c->src_w == src_w  &&
		    c->src_h == src_h)
		{
			return c;
		}
		l = l->next;
	}

	return NULL;
}

static
void resizeMe(int w, int h)
{
	XSizeHints *size_hints;

	size_hints = XAllocSizeHints();
	
	_width = w;
	_height = h;

	size_hints->flags       = PSize | PMinSize | PMaxSize;
	size_hints->width   = _width;
	size_hints->height  = _height;
	size_hints->min_width   = _width;
	size_hints->min_height  = _height;
	size_hints->max_width   = _width;
	size_hints->max_height  = _height;
	XSetWMNormalHints(_dpy, _win, size_hints);
	XResizeWindow(_dpy, _win, _width, _height);
	
	XFree(size_hints);
}

void
setup_layout(int startup)
{
	int max_width = 0;
	int max_height = 0;
	int cur_x = 0;
	int prev_y = 0;
	flist *l = Facade;
	cut *prev_cut;
	_height = 0;
	_width = 0;
	while(l != NULL)
	{
		cut *c = (cut *)l->object;
		max_width = max(max_width, c->src_w);
		max_height = max(max_height, c->src_h);
		l = l->next;
	}

	l = Facade;
	prev_cut = NULL;
	while(l != NULL)
	{
		cut *c = (cut *)l->object;
		_width = max(_width, c->src_w);
		if (cur_x > 0 && cur_x + c->src_w < max_width)
		{
			c->dest_x = cur_x;
			c->dest_y = prev_y;
			_height = max(_height, prev_y+c->src_h);
			cur_x = cur_x + c->src_w;
			c->sl = 0;
		}
		else
		{
			if (prev_cut)
			{
				prev_cut->lb = 1;
			}
			c->dest_x = 0;
			c->sl = 1;
			prev_y = c->dest_y = _height;
			cur_x = c->src_w;
			_height = _height +  c->src_h;
		}
		c->lb = 0;
		prev_cut = c;
		l = l->next;
		if (l == NULL)
		{
			prev_cut->lb = 1;	
		}
	}

	if (startup == 0)
	{
		if (_width == 0)
		{
			_width = 100;
		}
		if (_height == 0)
		{
			_height = 50;
		}
		resizeMe(_width, _height);
	}
}

void update_layout(void)
{
	flist *l = Facade;
	cut *prev_cut;
	int next_x = 0;
	int next_y = 0;

	_height = 0;
	_width = 0;

	l = Facade;
	prev_cut = NULL;
	while(l != NULL)
	{
		cut *c = (cut *)l->object;
		if (prev_cut != NULL && prev_cut->lb == 1)
		{
			/* start a new line */
			c->dest_x = 0;
			c->sl = 1;
			c->dest_y = next_y = _height;
			next_x = c->src_w;
			_height = _height + c->src_h;
			_width = max(_width, c->src_w);
		}
		else
		{
			if (prev_cut == NULL)
			{
				c->sl = 1;	
			}
			else
			{
				c->sl = 0;
			}
			c->dest_x = next_x;
			c->dest_y = next_y;
			next_x = next_x + c->src_w;
			_width = max(_width, next_x);
			_height = max(_height, c->dest_y + c->src_h);
		}
		prev_cut = c;
		l = l->next;
	}

	if (_width == 0)
	{
		_width = 100;
	}
	if (_height == 0)
	{
		_height = 50;
	}
	resizeMe(_width, _height);
}

#if 0
static
void clearWindows(void)
{
	flist *l = Facade;

	while(l != NULL)
	{
		l = l->next;
		cut *c = (cut *)l->object;
		XClearArea(_dpy, c->cid, 0, 0, 0, 0, True);
	}
}
#endif

void set_facade_property(Atom atom_type)
{
	long *duplicate_hints;
	int duplicate_count;
	int i;
	flist *l;

	duplicate_count = 0;

	l = Facade;
	i = 0;
	while(l != NULL)
	{
		i++;
		l = l->next;
	}

	duplicate_hints = (long  *)safemalloc(7*i*sizeof(long));

	l = Facade;	
	while(l != NULL)
	{
		cut *c = (cut *)l->object;
		duplicate_hints[duplicate_count++] = (CARD32)c->fid;
		duplicate_hints[duplicate_count++] = (CARD32)c->src_x;
		duplicate_hints[duplicate_count++] = (CARD32)c->src_y;
		duplicate_hints[duplicate_count++] = (CARD32)c->src_w;
		duplicate_hints[duplicate_count++] = (CARD32)c->src_h;
		duplicate_hints[duplicate_count++] = (CARD32)c->dest_x;
		duplicate_hints[duplicate_count++] = (CARD32)c->dest_y;
		l = l->next;
	}


	XChangeProperty(
		_dpy, _win, atom_type, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)duplicate_hints,
		duplicate_count);

	if (_saved)
	{
		unsigned long tmp[1];
		
		tmp[0] = _facade_real;
		XChangeProperty(
			_dpy, _win, _METISSE_NET_WM_FACADE_SAVED,
			XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)tmp, 1);
	}
	else
	{
		XDeleteProperty(_dpy, _win, _METISSE_NET_WM_FACADE_SAVED);
	}

	free(duplicate_hints);
}

cut *check_pos(int rx, int ry, unsigned int *rgrav)
{
	flist *l;
	cut *rc = NULL;
	Bool north = False, south = False, west = False, east = False,
		xcenter = False, ycenter = False;
	unsigned int grav = 0;
	int min_sdist = -1;
	int west_dx = 0, east_dx = 0, south_dy = 0, north_dy = 0;

	l = Facade;	
	while(l != NULL)
	{
		cut *c = (cut *)l->object;
		if (rx >= c->dest_x && rx <= c->dest_x + c->src_w &&
		    ry >= c->dest_y && ry <= c->dest_y + c->src_h)
		{
			north = False; south = False; west = False;
			east = False; xcenter = False; ycenter = False;
			west_dx = east_dx = 0;
			south_dy = north_dy = 0;
			if (rx < (c->dest_x + (c->src_w/3)))
			{
				west = True;
				west_dx = rx - c->dest_x;
			}
			else if (rx > (c->dest_x + 2*c->src_w/3))
			{
				east_dx = c->dest_x + c->src_w - rx;
				east = True;
			}
			else
			{
				west_dx = rx - c->dest_x;
				east_dx = c->dest_x + c->src_w - rx;
				xcenter = True;
			}
			if (ry <= (c->dest_y + c->src_h/3))
			{
				south_dy = ry - c->dest_y;
				south = True;
			}
			else if (ry >= (c->dest_y + 2*c->src_h/3))
			{
				north_dy = c->dest_y + c->src_h - ry;
				north = True;
			}
			else
			{
				south_dy = ry - c->dest_y;
				north_dy = c->dest_y + c->src_h - ry;
				ycenter = True;
			}
			rc = c;
			break;
		}
		else
		{
			int dx,dy;
			Bool tmp_north = False, tmp_south = False,
				tmp_west = False, tmp_east = False,
				tmp_xcenter = False, tmp_ycenter = False;
			int tmp_west_dx = 0, tmp_east_dx = 0, 
				tmp_south_dy = 0, tmp_north_dy = 0;
			if (rx >= c->dest_x && rx <= c->dest_x + c->src_w)
			{
				if (rx <= (c->dest_x + (c->src_w/3)))
				{
					tmp_west_dx = rx - c->dest_x;
					tmp_west = True;
				}
				else if (rx >= (c->dest_x + 2*c->src_w/3))
				{
					tmp_east_dx = c->dest_x + c->src_w - rx;
					tmp_east = True;
				}
				else
				{
					tmp_west_dx = rx - c->dest_x;
					tmp_east_dx = c->dest_x + c->src_w - rx;
					tmp_xcenter = True;
				}
				dx = 0;
			}
			else if (rx <= c->dest_x)
			{
				tmp_west = True;
				tmp_west_dx = dx = c->dest_x - rx;
			}
			else
			{
				tmp_east = True;
				tmp_east_dx = dx = c->dest_x + c->src_w -rx;
			}

			if (ry >= c->dest_y && ry <= c->dest_y + c->src_h)
			{
				if (ry <= (c->dest_y + c->src_h/3))
				{
					tmp_south_dy = ry - c->dest_y;
					tmp_south = True;
				}
				else if (ry >= (c->dest_y + 2*c->src_h/3))
				{
					tmp_north_dy = c->dest_y + c->src_h - ry;
					tmp_north = True;
				}
				else
				{
					tmp_south_dy = ry - c->dest_y;
					tmp_north_dy = c->dest_y + c->src_h - ry;
					tmp_ycenter = True;
				}
				dy = 0;
			}
			else if (ry < c->dest_y)
			{
				tmp_south = True;
				tmp_south_dy = dy = c->dest_y - ry;
			}
			else
			{
				tmp_north = True;
				tmp_north_dy = dy = c->dest_y + c->src_h -ry;
			}

			if (min_sdist < 0 || dx*dx + dy*dy < min_sdist)
			{
				rc = c;
				min_sdist = dx*dx + dy*dy;
				north = tmp_north; south = tmp_south;
				west = tmp_west; east = tmp_east;
				xcenter = tmp_xcenter; ycenter = tmp_ycenter;
				west_dx = tmp_west_dx; east_dx = tmp_east_dx;
				south_dy = tmp_south_dy; north_dy = tmp_north_dy;
			}
		}
		l = l->next;
	}

	if (rc == NULL)
	{
		/* fprintf(stderr,"CP: not found\n"); */
		return NULL;
	}
	
	if (north)
	{
		if (west)
		{
			if (north_dy < west_dx)
			{
				/* fprintf(stderr,"NorthGravity"); */
				grav = NorthGravity;
			}
			else
			{
				/* fprintf(stderr,"WestGravity"); */
				grav = WestGravity;
			}
		}
		else if (east)
		{
			if (north_dy < east_dx)
			{
				/* fprintf(stderr,"NorthGravity"); */
				grav = NorthGravity;
			}
			else
			{
				/* fprintf(stderr,"EastGravity"); */
				grav = EastGravity;
			}
		}
		else
		{
			/* fprintf(stderr,"NorthGravity"); */
			grav = NorthGravity;
		}
	}
	else if (south)
	{
		if (west)
		{
			if (south_dy < west_dx)
			{
				/* fprintf(stderr,"SouthGravity"); */
				grav = SouthGravity;
			}
			else
			{
				/* fprintf(stderr,"WestGravity"); */
				grav = WestGravity;
			}
		}
		else if (east)
		{
			if (south_dy < east_dx)
			{
				/* fprintf(stderr,"SouthGravity"); */
				grav = SouthGravity;
			}
			else
			{
				/* fprintf(stderr,"EastGravity"); */
				grav = EastGravity;
			}
		}
		else
		{
			/* fprintf(stderr,"SouthGravity"); */
			grav = SouthGravity;
		}
	}
	else
	{
		if (south_dy < north_dy)
		{
			if (east_dx < west_dx)
			{
				if (south_dy < east_dx)
				{
					/* fprintf(stderr,"SouthGravity"); */
					grav = SouthGravity;
				}
				else
				{
					/* fprintf(stderr,"EastGravity"); */
					grav = EastGravity;
				}
			}
			else
			{
				if (south_dy < west_dx)
				{
					/* fprintf(stderr,"SouthGravity"); */
					grav = SouthGravity;
				}
				else
				{
					/* fprintf(stderr,"WestGravity"); */
					grav = WestGravity;
				}
			}
		}
		else
		{
			if (east_dx < west_dx)
			{
				if (north_dy < east_dx)
				{
					/* fprintf(stderr,"NorthGravity"); */
					grav = NorthGravity;
				}
				else
				{
					/* fprintf(stderr,"EastGravity"); */
					grav = EastGravity;
				}
			}
			else
			{
				if (north_dy < west_dx)
				{
					/* fprintf(stderr,"NorthGravity"); */
					grav = NorthGravity;
				}
				else
				{
					/* fprintf(stderr,"WestGravity"); */
					grav = WestGravity;
				}
			}
		}
	}

	/* fprintf(stderr,"CP Found: %i\n", grav); */
	/* fprintf(stderr,"cut: %i %i %i %i %i %i\n",
				rc->src_x, rc->src_y,
				rc->src_w, rc->src_h,
				rc->dest_x, rc->dest_y); */

	*rgrav = grav;
	return rc;
}

void remove_cut(cut *c)
{
	flist *l;
	cut *prev_cut = NULL;
	Bool found = False;

	if (c == NULL)
	{
		return;
	}

	l = Facade;
	while(l != NULL)
	{
		cut *tc = (cut *)l->object;
		if (c == tc)
		{
			found = True;
			break;
		}
		prev_cut = c;
		l = l->next;
	}

	if (!found)
	{
		return;
	}

	if (c->lb == 1 && prev_cut)
	{
		prev_cut->lb = 1;	
	}

	Facade = flist_remove_obj(Facade, c);
	//setup_layout(0);
	update_layout();
	set_facade_property(_METISSE_NET_WM_FACADE);
	XFlush(_dpy);
	XLowerWindow(_dpy,_win);
	XFlush(_dpy);
	XRaiseWindow(_dpy,_win);
	XFlush(_dpy);
	XClearArea(_dpy, _win, 0, 0, 0, 0, True);
}

void add_cut(cut *new_c, int cx, int cy)
{
	Window child_w;
	int rx, ry;
	cut *cp = NULL;
	unsigned int grav;
	Bool found_pos = False;
	int pos = 1;

	if (!XTranslateCoordinates(
		    _dpy, DefaultRootWindow(_dpy), _win, cx, cy, &rx, &ry,
		    &child_w))
	{
		/* fprintf(stderr,"XTC: FAIL\n"); */
	}
	else
	{
		cp = check_pos(rx, ry, &grav);
	}

	new_c->lb = 0;

	if (cp != NULL)
	{
		cut *prev_cut = NULL;
		flist *l = Facade;
		while(l != NULL)
		{
			cut *c = (cut *)l->object;
			if (cp == c)
			{
				if (grav == EastGravity)
				{
					/* put it before */
					pos = pos-1;
					found_pos = True;
					break;
				}
				else if (grav == WestGravity)
				{
					/* ok, put it after */
					if (cp->lb == 1)
					{
						new_c->lb = 1;
						cp->lb = 0;
					}
					found_pos = True;
					break;
				}
				else if (grav == NorthGravity)
				{
					int tdist = 0;
					int abs_dist = 0;
					Bool dist_seted = False;
					cut *close_cut = 0;
					int p;
					/* goto next line */
					while(l != NULL)
					{
						cut *cc;
						cc = (cut *)l->object;

						prev_cut = cc;
						l = l->next;
						pos++;
						if (cc->lb == 1)
						{
							break;
						}
					}
					/* we are at the beginning of the
					 * next line */
					if (l == NULL)
					{
						/* no next line */
						prev_cut->lb = 1; 
						new_c->lb = 1;
						break;
					}
					p = pos;
					/* go throught the next line */
					while(l != NULL)
					{
						cut *cc =
							(cut *)l->object;
						tdist = min(
							abs(rx - cc->dest_x),
							abs(rx - cc->dest_x
							    -cc->src_w));
						if (!dist_seted ||
						    abs_dist > tdist)
						{
							p = pos;
							close_cut = cc;
							dist_seted = True;
							abs_dist = tdist;
						}
						prev_cut = cc;
						l = l->next;
						pos++;
						if (cc->lb == 1)
						{
							break;
						}
					}
					if (rx <
					    close_cut->dest_x+
					    close_cut ->src_w/2)
					{
						/* avant*/
						pos = p;
					}
					else
					{
						/* apres */
						if (close_cut->lb == 1)
						{
							close_cut->lb = 0;
							new_c->lb = 1;
						}
						pos = p+1;
					}
					found_pos = True;
					break;
				}
				else /* SouthGravity */
				{
					int tdist = 0;
					int abs_dist = 0;
					Bool dist_seted = False;
					cut *close_cut = 0;
					int p;
					/* prev line */

					/* goto to the end of prev line */
					while(l != NULL)
					{
						cut *cc;
						cc = (cut *)l->object;
						if (cc->sl == 1)
						{
							pos--;
							l = l->prev;
							break;
						}
						pos--;
						l = l->prev;
					}
					/* goto to the begining of prev line */
					while(l != NULL)
					{
						cut *cc;
						cc = (cut *)l->object;
						
						if (cc->sl == 1)
						{
							break;
						}
						pos--;
						l = l->prev;
					}
					if (l == NULL)
					{
						pos = 0;
						found_pos = True;
						new_c->lb = 1;
						break;
					}
					p = pos;
					/* go throught the prev line */
					while(l != NULL)
					{
						cut *cc =
							(cut *)l->object;
						tdist = min(
							abs(rx - cc->dest_x),
							abs(rx - cc->dest_x
							    -cc->src_w));
						if (!dist_seted ||
						    abs_dist > tdist)
						{
							p = pos;
							close_cut = cc;
							dist_seted = True;
							abs_dist = tdist;
						}
						prev_cut = cc;
						l = l->next;
						pos++;
						if (cc->lb == 1)
						{
							break;
						}
					}
					if (rx < close_cut->dest_x +
					    close_cut ->src_w/2)
					{
						/* avant */
						pos = p-1;
					}
					else
					{
						/* apres */
						if (close_cut->lb == 1)
						{
							close_cut->lb = 0;
							new_c->lb = 1;
						}
						pos = p;
					}
					found_pos = True;
					break;
				}
			}

			if (l == NULL || found_pos)
			{
				/* fprintf(stderr,"Error"); */
				break;
			}
			prev_cut = c;
			l = l->next;
			pos++;
		}
		
	}

	if (found_pos)
	{
		/* fprintf(stderr,"Position found: %i\n", pos); */
		Facade = flist_insert_obj(Facade, new_c, pos);
	}
	else
	{
		new_c->lb = 1;
		Facade = flist_append_obj(Facade, new_c);
	}

	update_layout();

	XSelectInput(
		_dpy, new_c->fid, StructureNotifyMask);
	XSelectInput(
		_dpy, new_c->cid, StructureNotifyMask);

	set_facade_property(_METISSE_NET_WM_FACADE);
	XFlush(_dpy);
	XLowerWindow(_dpy,_win);
	XFlush(_dpy);
	XRaiseWindow(_dpy,_win);
	XFlush(_dpy);
}


void Xloop(void)
{
	int prev_cx = 0;
	int prev_cy = 0;

	while (1)
	{
		XEvent ev;

		/* Sit and wait for an event to happen */
		XNextEvent(_dpy,&ev);

		switch(ev.type)
		{
		case ConfigureNotify:
		{
			cut *c;
			while (XCheckTypedWindowEvent(
				       _dpy, ev.xconfigure.window,
				       ConfigureNotify, &ev))
			{
			}
			if (_type == METISSE_WIN_TYPE_FACADE || Facade == NULL)
			{
				break;
			}
			c = (cut *)Facade->object;
			if (ev.xconfigure.window == c->cid &&
			    (_width != ev.xconfigure.width ||
			     _height != ev.xconfigure.height))
			{
				_width = ev.xconfigure.width;
				_height = ev.xconfigure.height;
				/* resizeMe(_width, _height); */
				XResizeWindow(_dpy, _win, _width, _height);
				XSync(_dpy, True);
				/*fprintf(stderr,"XConfigure\n");*/
			}
			else if (ev.xconfigure.window == _win &&
			    (_width != ev.xconfigure.width ||
			     _height != ev.xconfigure.height))
			{
				/*fprintf(stderr,"XConfigure %i\n", count);*/
			}
			break;
		}
		case ClientMessage:
			if (ev.xclient.format == 32 &&
			    ev.xclient.data.l[0] == ATOM_WM_DELETE_WINDOW)
			{
				exit(0);
			}
			else if (ev.xclient.message_type ==
				 _METISSE_NET_WM_FACADE &&
				 ev.xclient.data.l[0] == 0)
			{
				prev_cx = ev.xclient.data.l[1];
				prev_cy = ev.xclient.data.l[2];
			}
			else if (ev.xclient.message_type ==
				 _METISSE_NET_WM_FACADE)
			{
				XID fid = ev.xclient.data.l[0];
				XID cid;
				cut *c;

				/* fprintf(stderr,"_METISSE_NET_WM_FACADE\n"); */
				fid = get_toplevel_window(_dpy,(Window)fid);
				if (fid == None)
				{
					break;
				}
				if (!get_client_window(_dpy, (Window)fid, &cid))
				{
					break;
				}
				
				c = (cut *)safemalloc(sizeof(cut));

				c->src_x = ev.xclient.data.l[1];
				c->src_y = ev.xclient.data.l[2];
				c->src_w = ev.xclient.data.l[3];
				c->src_h = ev.xclient.data.l[4];
				c->dest_x = 0;
				c->dest_y = 0;
				c->fid = fid;
				c->cid = cid;
				_saved = False;
				add_cut(c, prev_cx, prev_cy);
				#if 0
				fprintf(stderr,"cut: %i %i %i %i %i %i 0x%lx "
					"0x%lx\n",
					c->src_x, c->src_y,
					c->src_w, c->src_h,
					c->dest_x, c->dest_y,
					c->fid, c->cid);
				#endif
			}
			else if (ev.xclient.message_type ==
				 _METISSE_NET_WM_FACADE_REMOVE)
			{
				XID fid = ev.xclient.data.l[0];
				cut *c;

				fid = get_toplevel_window(_dpy,(Window)fid);
				if (fid == None)
				{
					break;
				}
				c = get_cut(
					fid,
					ev.xclient.data.l[1],
					ev.xclient.data.l[2],
					ev.xclient.data.l[3],
					ev.xclient.data.l[4]);
				_saved = False;
				remove_cut(c);
			}
			break;
		default:
			break;
		}
	}
}

static
void parseArgs(int argc, char * argv[])
{
	int i;
	int x_r,y_r,ret;
	unsigned int h_r,w_r;
	XID prevID = 0;
	cut *prevCut = NULL;

	for (i = 1; i < argc; i++)
	{
		if (strcasecmp(argv[i], "--id") == 0 ||
		    strcasecmp(argv[i], "-i") == 0)
		{
			i++;
			/*_facade[_num_facade].fid = strtoul(argv[i], NULL, 0);*/
			prevID = strtoul(argv[i], NULL, 0);
			/* _facade[_num_facade].fid;*/
		}
		else if (strcasecmp(argv[i], "--facade") == 0 ||
			 strcasecmp(argv[i], "-c") == 0)
		{
			_type = METISSE_WIN_TYPE_FACADE;
		}
		else if (strcasecmp(argv[i], "--duplicate") == 0 ||
			 strcasecmp(argv[i], "-d") == 0)
		{
			_type = METISSE_WIN_TYPE_DUPLICATE;
		}
		else if (strcasecmp(argv[i], "--saved") == 0 ||
			 strcasecmp(argv[i], "-a") == 0)
		{
			_saved = True;
			i++;
			_facade_real = strtoul(argv[i], NULL, 0);
		}
		else if (strcasecmp(argv[i], "--name") == 0 ||
			 strcasecmp(argv[i], "-n") == 0)
		{
			i++;
			_optName = (char *)malloc(sizeof(argv[i])+1);
			_optName = strdup(argv[i]);
		}
		else if (strcasecmp(argv[i], "--position") == 0 ||
			 strcasecmp(argv[i], "-p") == 0)
		{
			i++;
			ret = XParseGeometry(argv[i], &x_r, &y_r, &w_r, &h_r);
			if ((ret & XValue) && (ret & YValue))
			{
				_hasPos = True;
				_usX = x_r;
				_usY = y_r;
			}
		}
		else if (strcasecmp(argv[i], "--geometry") == 0 ||
			 strcasecmp(argv[i], "-g") == 0)
		{
			cut *c;

			i++;
			ret = XParseGeometry(argv[i], &x_r, &y_r, &w_r, &h_r);
			if (!((ret & WidthValue) && (ret & HeightValue) &&
			      (ret & XValue) && (ret & YValue)))
			{
				fprintf(stderr, "%s: invalid --geometry args\n",
					_appname);
				exit(1);
			}

			c = (cut *)safemalloc(sizeof(cut));
			c->src_x = x_r;
			c->src_y = y_r;
			c->src_w = w_r;
			c->src_h = h_r;
			c->dest_x = 0;
			c->dest_y = 0;
			c->fid = prevID;
			c->sl = 1;
			c->lb = 1;
			Facade = flist_append_obj(Facade, c);
			prevCut = c;
		}
		else if (strcasecmp(argv[i], "--source") == 0 ||
			 strcasecmp(argv[i], "-s") == 0)
		{
			cut *c;

			i++;
			c = (cut *)safemalloc(sizeof(cut));
			c->src_x = strtol(argv[i++], NULL, 0);
			c->src_y = strtol(argv[i++], NULL, 0);
			c->src_w = strtol(argv[i++], NULL, 0);
			c->src_h = strtol(argv[i++], NULL, 0);
			c->dest_x = strtol(argv[i++], NULL, 0);
			c->dest_y = strtol(argv[i], NULL, 0);
			c->fid = prevID;
			if (c->dest_x == 0)
			{
				if (prevCut)
				{
					prevCut->lb = 1;
				}
				c->sl = 1;
			}
			else
			{
				c->sl = 0;
			}
			c->lb = 0;
			Facade = flist_append_obj(Facade, c);
			prevCut = c;
			_doLayoutAtStartUp = False;
		}
	}

	if (Facade == NULL && prevID != 0)
	{
		cut *c = (cut *)safemalloc(sizeof(cut));
		c->fid = prevID;
		Facade = flist_append_obj(Facade, c);
		_type = METISSE_WIN_TYPE_DUPLICATE;
	}
}


int main(int argc, char * argv[])
{
	int screen_num;
	Atom atom_type = None;
	char *window_name; /*= "dummy win";*/
	char *icon_name;   /*= "dummy win";*/

	Window junkRoot;
	int frame_x, frame_y;
	unsigned int frame_width, frame_height, frame_BW, frame_depth;
	int client_x, client_y;
	unsigned int client_width, client_height, client_BW, client_depth;

	char *display_name = NULL;
	
	XSizeHints *size_hints;
	XWMHints   *wm_hints;
	XClassHint *class_hints;
	XTextProperty windowName, iconName;
	XColor bg_color;

	flist *l;

	fprintf(stderr, "Start facade-holder\n");
	_appname = argv[0];
	
	/*  Set initial window size and position, and create it  */
	_width  = 200;
	_height = 200;

	parseArgs(argc, argv);

	if (_type == METISSE_WIN_TYPE_FACADE && Facade == NULL)
	{
		fprintf(stderr, "%s: no cuts defined\n", _appname);
		exit(1);
	}
	else if (Facade == NULL)
	{
		fprintf(stderr, "%s: no duplicate win given\n", _appname);
		exit(1);
	}


	/*  Connect to X server  */

	if ((_dpy = XOpenDisplay(display_name)) == NULL)
	{
		fprintf(
			stderr, "%s: couldn't connect to X server %s\n",
			_appname, display_name);
		exit(1);
	}

	XSetErrorHandler(myErrorHandler);

	if (!(size_hints = XAllocSizeHints()) ||
	    !(wm_hints = XAllocWMHints())     ||
	    !(class_hints = XAllocClassHint()))
	  {
		fprintf(stderr, "%s: couldn't allocate memory.\n", _appname);
		exit(1);
	}

	_WM_STATE = XIA("WM_STATE");

	
	l = Facade;
	while(l != NULL)
	{
		cut *c = (cut *)l->object;

		if (!XGetGeometry(
			    _dpy, (Window)c->fid, &junkRoot, &frame_x,
			    &frame_y,
			    &frame_width, &frame_height, &frame_BW,
			    &frame_depth))
		{
			fprintf(
				stderr, "%s: window 0x%lx does not exit\n",
				_appname, c->fid);
			exit(1);
		}

		c->fid =
			get_toplevel_window(_dpy,(Window)c->fid);

		if (!XGetGeometry(
			    _dpy, (Window)c->fid, &junkRoot, &frame_x,
			    &frame_y, &frame_width, &frame_height, &frame_BW,
			    &frame_depth))
		{
			fprintf(
				stderr, "%s: window 0x%lx does not exit\n",
				_appname, c->fid);
			exit(1);
		}

		if (!get_client_window(
			    _dpy, (Window)c->fid, &c->cid))
		{
			fprintf(
				stderr, "%s: couldn't get client window of "
				"0x%lx\n",_appname, c->fid);
			exit(1);
		}

		if (!XGetGeometry(
			    _dpy, (Window)c->cid, &junkRoot, &client_x,
			    &client_y, &client_width, &client_height,
			    &client_BW,  &client_depth))
		{
			fprintf(
				stderr, "%s: client window 0x%lx does "
				"not exit\n", _appname, c->cid);
			exit(1);
		}

		l = l->next;
	}

	_METISSE_NET_WM_FACADE = XIA("_METISSE_NET_WM_FACADE");
	_METISSE_NET_WM_FACADE_REMOVE = XIA("_METISSE_NET_WM_FACADE_REMOVE");
	_METISSE_NET_WM_DUPLICATE = XIA("_METISSE_NET_WM_DUPLICATE");
	_METISSE_NET_WM_FACADE_SAVED = XIA("_METISSE_NET_WM_FACADE_SAVED");

	if (_type == METISSE_WIN_TYPE_FACADE)
	{
		if (_optName)
		{
			icon_name = _optName;
			window_name = _optName;
		}
		else
		{
			icon_name = _facade_Name;
			window_name = _facade_Name;
		}
		class_hints->res_class  = "metisse facade";
		/* FIXME */
		_height = 0;
		_width = 0;
		l = Facade;
		while(l != NULL)
		{
			cut *c = (cut *)l->object;

			if (_doLayoutAtStartUp)
			{
				_width = max(_width, c->src_w);
				c->dest_x = 0;
				c->dest_y = _height;
				c->lb = c->sl = 1;
				_height = _height +  c->src_h;
			}
			else
			{
				_height =
					max(_height,
					    c->dest_y +
					    c->src_h);
				_width = max(_width,
					     c->dest_x +
					     c->src_w);
			}
			#if 0
			fprintf(stderr,"cut: %i %i %i %i %i %i\n",
				c->src_x, c->src_y,
				c->src_w, c->src_h,
				c->dest_x, c->dest_y);
			#endif

			l = l->next;
		}
			
		atom_type = _METISSE_NET_WM_FACADE;

		size_hints->flags = PSize | PMinSize | PMaxSize;
	}
	else
	{
		char *name;
		int is_net;
		cut *c = (cut *)Facade->object;
		name = get_window_title(_dpy, (Window)c->cid, &is_net);
		icon_name = (name)? name:_dupliacte_Name;
		window_name  = (name)? name:_dupliacte_Name;
		class_hints->res_class  = "metisse duplicate";
		
		_width = client_width;
		_height = client_height;
	
		atom_type = _METISSE_NET_WM_DUPLICATE;

		size_hints->flags = PSize;
	}

	size_hints->width = _width;
	size_hints->height = _height;
	size_hints->min_width   = _width;
	size_hints->min_height  = _height;
	size_hints->max_width   = _width;
	size_hints->max_height  = _height;
	
	if (_hasPos)
	{
		size_hints->flags = USPosition;
		size_hints->x = _usX;
		size_hints->y = _usY;
	}

	wm_hints->flags = StateHint;
	wm_hints->initial_state = NormalState;
	/* wm_hints->input         = True; */

	class_hints->res_name = _appname;

	/*  Get screen size from display structure macro  */
	screen_num = DefaultScreen(_dpy);
	
	bg_color.red = (int)65535/2;
	bg_color.green = (int)65535/2;
	bg_color.green = (int)65535/2;
	XAllocColor(_dpy, DefaultColormap(_dpy,screen_num), &bg_color);

	_win = XCreateSimpleWindow(
		_dpy, XDefaultRootWindow(_dpy),
		_usX, _usY, _width, _height, 0,
		bg_color.pixel, bg_color.pixel);
	/* WhitePixel(_dpy, screen_num), WhitePixel(_dpy, screen_num));
	 * bg_color.pixel,
	 * bg_color.pixel);
	 * BlackPixel(_dpy, screen_num));
	 * WhitePixel(_dpy, screen_num)); */

	if ( XStringListToTextProperty(&window_name, 1, &windowName) == 0 ) 
	{
		fprintf(
			stderr,
			"%s: structure allocation for windowName failed.\n",
			_appname);
		exit(1);
	}

	if (XStringListToTextProperty(&icon_name, 1, &iconName) == 0 )
	{
		fprintf(
			stderr,
			"%s: structure allocation for iconName failed.\n",
			_appname);
		exit(1);
	}


	XSetWMProperties(
		_dpy, _win, &windowName, &iconName, argv, argc,
		size_hints, wm_hints, class_hints);


	ATOM_WM_DELETE_WINDOW = XIA("WM_DELETE_WINDOW");
	{
		Atom proto[1];
		int j = 0;

		proto[j++] = ATOM_WM_DELETE_WINDOW;

		XSetWMProtocols(_dpy, _win, proto, j);
	}


	set_facade_property(atom_type);

	/*  Choose which events we want to handle  */
	XSelectInput(_dpy, _win, StructureNotifyMask);

	l = Facade;
	while(l != NULL)
	{
		cut *c = (cut *)l->object;

		XSelectInput(
			_dpy, c->fid, StructureNotifyMask);
		XSelectInput(
			_dpy, c->cid, StructureNotifyMask);
		l = l->next;
	}

	/* map */
	XMapWindow(_dpy, _win);

	Xloop();
	
	return 0;   /*  We shouldn't get here  */
}
