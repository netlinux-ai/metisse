/*
 *
 * main/ACursor.cxx --
 *
 * Copyright (C) Nicolas Roussel
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config-not-xserver.h"
#endif

#include "ACursor.H"

#include <X11/Xlib.h>

#ifndef __APPLE__
#include <nucleo/gl/window/glWindow_GLX.H>
#endif

#ifdef HAVE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#else
typedef unsigned int XcursorPixel;
#endif

using namespace nucleo;



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



ACursor::ACursor(glWindow *win, std::string name, GLfloat size, double zRot,
	int num)
	: sgNode(name)
{
  _x = _y = 0.0 ;
  _size = size ;
  _ARGBbuf = 0;
  _Xbuf = 0;
  _internal = 0;
  _r = _g = _b = 0;
  _internal = num;
  switch (num)
  {
  case 1:
	  _r = 1;
	  break;
  case 2:
	  _b = 1;
	  break;
  case 3:
	  _g = 1;
	  break;
  default:
	  _r = 1;
  }
  _a = .70;
  _qobj = 0;
  _width = _height = 0;
  _xhot = _yhot = 0;
  _zRot = zRot;
  _win = win;
  _useSoftwareCursor = true;
}

void
ACursor::display(dlPolicy policy)
{
	if (!_useSoftwareCursor)
		return;

	glPushMatrix();
	glTranslatef(_x,_y,0);

	//glPushMatrix();
	glRotated(-_zRot, 0,0, _zRot);
	//glPopMatrix();

	glColor4f(1,1,1,1);
  
	if (_ARGBbuf)
	{
		glBegin(GL_POINTS);
		unsigned char *data = (unsigned char *)_ARGBbuf;
		int k = 0;
		for(GLfloat i = 0; i < _height; i++)
		{
			for(GLfloat j = 0; j < _width; j++, k = k + 4)
			{
				// in fact we have "premultiplied" BGRA
				GLfloat f = (GLfloat)data[k+3]/255.0;
				f = (f==0)? 0:1/f;
				glColor4f(
					((GLfloat)data[k+2]*f/255.0), 
					((GLfloat)data[k+1]*f/255.0),
					((GLfloat)data[k]*f/255.0),
					(GLfloat)data[k+3]/255.0);
				GLfloat u = j - _xhot;
				GLfloat v = -i + _yhot;
				glVertex2f(u,v);
			}
		}
		glEnd();
	}
	else if (_Xbuf)
	{
		int bytesPerRow = ((int)_width + 7) / 8;
		int bytesData = bytesPerRow * (int)_height;
	  
		int k = 0, l = 0, m = 0;
		glBegin(GL_POINTS);
		for(GLfloat i = 0; i < _height; i++, k = k + bytesPerRow)
		{
			l = 0;
			m = k;
			for(GLfloat j = 0; j < _width; j++)
			{
				glColor4ub(
					(_Xbuf[m] & (1 << l))? 
					_colors.foreRed: _colors.backRed,
					(_Xbuf[m] & (1 << l))?
					_colors.foreGreen: _colors.backGreen,
					(_Xbuf[m] & (1 << l))?
					_colors.foreBlue: _colors.backBlue,
					(_Xbuf[bytesData+m] & (1 << l))? 255:0);
				GLfloat u = j - _xhot;
				GLfloat v = -i + _yhot;
				glVertex2f(u,v);
				l++;
				if (l>7)
				{
					l = 0;
					m++;
				}
			}
		}
		glEnd();
	}
	else if (_internal > 0 && _qobj)
	{
		gluQuadricDrawStyle(_qobj, GLU_FILL);
		glColor4f(1,1,1,_a);
		gluDisk(_qobj, _width/4, _width/4+1, 25, (int)(1));
		glColor4f(_r,_g,_b,_a);
		gluDisk(_qobj, 1+(_width/4), (_width/2), 40, (int)(1));
		glColor4f(1,1,1,_a);
		gluDisk(_qobj, (_width/2), 1+(_width/2), 40, (int)(1));		
	}
	else
	{
		// draw nothing
	}

	glPopMatrix();
}



void
ACursor::updatePosition(GLfloat x, GLfloat y)
{
	// std::cerr << "ACursor::updatePosition: " << x << "," << y << std::endl;
	_x = x ;
	_y = y ;

	if (_useSoftwareCursor)
		postRedisplay();
}



void ACursor::handleXCursor(
	int xhot, int yhot, int width, int height, char *buf,
	const rfbXCursorColors &colors)
{
	size_t bytesPerRow, bytesData;

	bytesPerRow = (width + 7) / 8;
	bytesData = bytesPerRow * height;

	for (unsigned int i = 0; i < bytesData * 2; i++)
	{
		buf[i] = (char)_reverse_byte[(int)buf[i] & 0xFF];
	}

	if (_useSoftwareCursor)
	{
		handleXCursorSW(xhot, yhot, width, height, buf, colors);
		return;
	}

#ifdef __APPLE__
	handleXCursorSW(xhot, yhot, width, height, buf, colors);
#else
	XColor bg, fg;
	Drawable dr;
	Display *dpy;
	unsigned int wret = 0, hret = 0;
	Pixmap source, mask;
	Cursor cursor;
	static Bool prevXCursorSet = False;
	static Cursor prevXCursor;
	glWindow_GLX *castedWin;

	castedWin = dynamic_cast<glWindow_GLX *>(_win);
	if (castedWin == NULL)
	{
		std::cerr <<"MetisseDesktop::HandleXCursor GLX cast fail! ("
			  << __FILE__ << ", " << __LINE__ << ")" << std::endl;
	}
	dpy = (Display*)castedWin->getDisplay();
	dr = XDefaultRootWindow(dpy);

	if (width * height)
	{
		XQueryBestCursor(dpy, dr, width, height, &wret, &hret);
	}

	if (wret < (unsigned int)width || hret < (unsigned int)height)
	{
		// Switch to software cursor! (FIXME)
		std::cerr << "Metisse switch to software cursor!" << std::endl;
		handleXCursorSW(xhot, yhot, width, height, buf, colors);
		return;
	}

	if (buf == NULL || width * height == 0)
	{
		/* Free resources */
		if (buf != NULL)
		{
			delete[] buf;
		}
		width = height = 1;
		buf = new char[2];
		buf[0] = buf[1] = 0;
		bytesPerRow = 1;
		bytesData = 1;
	}
	if (xhot >= width)
	{
		xhot = width - 1;
	}
	else if (xhot < 0)
	{
		xhot = 0;
	}
	if (yhot > height)
	{
		yhot = height - 1;
	}
	else if (yhot < 0)
	{
		yhot = 0;
	}

	bg.red   = (unsigned short)colors.backRed   << 8 | colors.backRed;
	bg.green = (unsigned short)colors.backGreen << 8 | colors.backGreen;
	bg.blue  = (unsigned short)colors.backBlue  << 8 | colors.backBlue;
	fg.red   = (unsigned short)colors.foreRed   << 8 | colors.foreRed;
	fg.green = (unsigned short)colors.foreGreen << 8 | colors.foreGreen;
	fg.blue  = (unsigned short)colors.foreBlue  << 8 | colors.foreBlue;

	source = XCreateBitmapFromData(dpy, dr, buf, width, height);
	mask = XCreateBitmapFromData(
		dpy, dr, &buf[bytesData], width, height);
	cursor = XCreatePixmapCursor(
		dpy, source, mask, &fg, &bg, xhot, yhot);
	XFreePixmap(dpy, source);
	XFreePixmap(dpy, mask);
	delete[] buf;

	if (castedWin->getWindowID())
	{
		XDefineCursor(dpy, castedWin->getWindowID(), cursor);
	}
	if (prevXCursorSet)
	{
		XFreeCursor(dpy, prevXCursor);
		prevXCursorSet = False;
	}
	prevXCursor = cursor;
	prevXCursorSet = True;
#endif
}



void ACursor::handleXCursorSW(
	int xhot, int yhot, int width, int height, char *buf,
	const rfbXCursorColors &colors)
{
	if (_ARGBbuf)
	{
		delete _ARGBbuf;
		_ARGBbuf = NULL;
	}
	if (_Xbuf)
	{
		delete _Xbuf;
		_Xbuf = NULL;
	}
	_internal = 0;
	_xhot = xhot;
	_yhot = yhot;
	_width = width;
	_height = height;
	_colors = colors;
	_Xbuf = buf;
	postRedisplay();
}



void ACursor::handleARGBCursor(
	int xhot, int yhot, int width, int height, char *buf)
{
	if (_useSoftwareCursor)
	{
		handleARGBCursorSW(xhot, yhot, width, height, buf);
		return;
	}

#if !defined (__APPLE__) && defined (HAVE_XCURSOR)
	static Bool prevXCursorSet = False;
	static Cursor prevXCursor;
	Cursor cursor;
	glWindow_GLX *castedWin;
	Display *dpy;
	XcursorImage *image;
	Window xw;

	castedWin = dynamic_cast<glWindow_GLX *>(_win);
	if (castedWin == 0)
	{
		std::cerr <<"MetisseDesktop::HandleXCursor GLX cast fail! ("
			  << __FILE__ << ", " << __LINE__ << ")" << std::endl;
	}
	dpy = (Display*)castedWin->getDisplay();

	image = XcursorImageCreate (width, height);

	image->xhot = xhot;	/* hot spot x (must be inside image) */
	image->yhot = yhot;	/* hot spot y (must be inside image) */
	image->pixels = (XcursorPixel*) buf; /* pointer to pixels */

	cursor = XcursorImageLoadCursor(dpy, image);
	xw = (Window)castedWin->getWindowID();

	if (xw)
	{
	  XDefineCursor(dpy, xw, cursor);
	}
	if (prevXCursorSet)
	{
		XFreeCursor(dpy, prevXCursor);
		prevXCursorSet = False;
	}
	prevXCursor = cursor;
	prevXCursorSet = True;

	if (buf)
	{
		delete buf;
		image->pixels = NULL;
	}

	if (image)
	{
		XcursorImageDestroy(image); 
	}
#else
	handleARGBCursorSW(xhot, yhot, width, height, buf);
#endif
}



void ACursor::handleARGBCursorSW(
	int xhot, int yhot, int width, int height, char *buf)
{
	if (_ARGBbuf)
	{
		delete _ARGBbuf;
		_ARGBbuf = NULL;
	}
	if (_Xbuf)
	{
		delete _Xbuf;
		_Xbuf = NULL;
	}
	_internal = 0;
	_xhot = xhot;
	_yhot = yhot;
	_width = width;
	_height = height;
	_ARGBbuf = buf;
	postRedisplay() ;
}



void ACursor::handleInternalCursor(
	int xhot, int yhot, int width, int height)
{
	if (_ARGBbuf)
	{
		delete _ARGBbuf;
		_ARGBbuf = NULL;
	}
	if (_Xbuf)
	{
		delete _Xbuf;
		_Xbuf = NULL;
	}
	if (_qobj == 0)
	{
		_qobj = gluNewQuadric();
		gluQuadricDrawStyle(_qobj, GLU_SILHOUETTE);
		gluQuadricNormals(_qobj, GLU_NONE);
	}
	_xhot = xhot;
	_yhot = yhot;
	_width = width;
	_height = height;
	postRedisplay() ;
}



void ACursor::setSoftwareCursor(bool setSoftwareCursor)
{
	_useSoftwareCursor = setSoftwareCursor;
	_win->setCursorVisible(!_useSoftwareCursor);
}

