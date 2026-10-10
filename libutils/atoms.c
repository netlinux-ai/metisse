/* -*-c-*- */
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

#if HAVE_CONFIG_H
#include "config-not-xserver.h"
#endif

#include "safemalloc.h"

#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>

static int atom_size(int format)
{
	if (format == 32)
	{
		return sizeof(long);
	}
	else
	{
		return (format >> 3);
	}
}

void *utils_atom_get_property(
	Display *dpy, Window win, Atom to_get, Atom type, unsigned int *size)
{
	unsigned char *retval;
	Atom  type_ret;
	unsigned long  bytes_after, num_ret;
	long length;
	int  format_ret;
	void *data;
	int ok;

	retval = NULL;
	length = 0x7fffffff;
	ok = XGetWindowProperty(
		dpy, win, to_get, 0L, length, False, type, &type_ret,
		&format_ret, &num_ret, &bytes_after, &retval);

	if ((ok == Success) && (retval) && (num_ret > 0) && (format_ret > 0))
	{
		int asize;

		asize = atom_size(format_ret);
		data = (void *)safemalloc(num_ret * asize);
		if (format_ret == 32 && asize * 8 != format_ret)
		{
			int i;

			for (i = 0; i < num_ret; i++)
			{
				((CARD32 *)data)[i] = ((long *)retval)[i];
			}
		}
		else
		{
			if (data)
			{
				memcpy(data, retval, num_ret * asize);
			}
		}
		XFree(retval);
		*size = num_ret * (format_ret >> 3);

		return data;
	}
	if (retval)
	{
		XFree(retval);
	}

	return NULL;
}
