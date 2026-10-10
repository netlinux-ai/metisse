#ifndef _M_UTILS_
#define _M_UTILS_

void *utils_atom_get_property(
	Display *dpy, Window win, Atom to_get, Atom type, unsigned int *size);

#endif
