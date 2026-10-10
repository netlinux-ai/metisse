/*
 *
 * main/ACursor.H --
 *
 * Copyright (C) Nicolas Roussel
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef ACURSOR_H
#define ACURSOR_H

#include <nucleo/gl/glIncludes.H>
#include <nucleo/gl/scenegraph/sgNode.H>
#include <nucleo/gl/window/glWindow.H>

#include "libmetisse/proto.h"



class ACursor : public nucleo::sgNode
{
public:
	ACursor(nucleo::glWindow *win, std::string name, GLfloat size,
	        double zRot, int num = 0);

	void updatePosition(GLfloat x, GLfloat y);

	void handleXCursor(int xhot, int yhot, int width, int height,
	                   char *buf, const rfbXCursorColors &colors);

	void handleXCursorSW(int xhot, int yhot, int width, int height,
	                     char *buf, const rfbXCursorColors &colors);

	void handleARGBCursor(int xhot, int yhot, int width, int height,
	                      char *buf);

	void handleARGBCursorSW(int xhot, int yhot, int width, int height,
	                        char *buf);

	void handleInternalCursor(int xhot, int yhot, int width, int height);

	void setSoftwareCursor(bool setSoftwareCursor);

private:
	GLfloat _x, _y, _size;
	GLfloat _width, _height;
	GLfloat _xhot, _yhot;
	GLdouble _zRot;

	GLfloat _a,_r,_g,_b;
	rfbXCursorColors _colors;

	char *_Xbuf;
	char *_ARGBbuf;

	int _internal;
	GLUquadricObj *_qobj;

	nucleo::glWindow *_win;
	bool _useSoftwareCursor;

protected:
	void display(dlPolicy policy);
};

#endif
