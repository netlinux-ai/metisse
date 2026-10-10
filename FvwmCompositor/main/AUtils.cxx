/*
 *
 * main/AUtils.cxx --
 *
 * Copyright (C) Nicolas Roussel
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config-not-xserver.h"
#endif

#include "AUtils.H"
#include "AScreen.H"

#include <nucleo/core/TimeStamp.H>

// FIXME
#include <sstream>
#include <string>
#include <cmath>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#define DEBUG_LEVEL 0

// --------------------------------------------------------------------

void
DefaultSettings::display(dlPolicy policy)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) ;
}

DefaultSettings::DefaultSettings(void) : sgNode("DefaultSettings")
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	//glDisable(GL_LIGHTING);
	//glShadeModel(GL_FLAT);

#if 0
	glEnable(GL_POINT_SMOOTH) ;
	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST) ;
#endif

#if 0
	glEnable(GL_LINE_SMOOTH) ;
	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST) ;
#endif

#if 0
	// unfortunately this cause rendering of the triangle polygone
	glEnable(GL_POLYGON_SMOOTH) ;
	glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST) ;
#endif

	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST) ;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glEnable(GL_DITHER);
	//glDisable(GL_DITHER);
}

// --------------------------------------------------------------------

AXorDraw::AXorDraw(std::string name, int *p, int num, int loop) : sgNode(name)
{
	clear();
	for(int i = 0; i < num-1; i = i+2)
	{
		_points.push_front(point(p[i],p[i+1], 0, 0));
	}
}

AXorDraw::AXorDraw(std::string name, float x, float y): sgNode(name)
{
	clear();
	_points.push_front(point(x,y, 0, 0));
}

#if 0
AXorDraw::~AXorDraw()
{
	_points.clear();
}
#endif

void AXorDraw::display(dlPolicy policy)
{	
	int size = _points.size();
	if (size < 2)
	{
		return;
	}

	glTranslatef(0,0,-0.1);

	std::list<AXorDraw::point >::iterator i;
	std::list<AXorDraw::point >::iterator j;

	glBegin(GL_LINES);

	int s = 0;
	for (i=_points.begin(); s < size - 1; i++)
	{
		j = i;
		j++;
		glColor4f(0,0,0,0.2);
		glVertex2d((*i).x-3,(*i).y-3); glVertex2d((*j).x-3,(*j).y-3);
		glVertex2d((*i).x+3,(*i).y+3); glVertex2d((*j).x+3,(*j).y+3);
		glColor4f(0,0,0,0.3);
		glVertex2d((*i).x-2,(*i).y-2); glVertex2d((*j).x-2,(*j).y-2);
		glVertex2d((*i).x+2,(*i).y+2); glVertex2d((*j).x+2,(*j).y+2);
		glColor4f(0,0,0,0.6);
		glVertex2d((*i).x-1,(*i).y-1); glVertex2d((*j).x-1,(*j).y-1);
		glVertex2d((*i).x+1,(*i).y+1); glVertex2d((*j).x+1,(*j).y+1);
		glColor4f(1,1,1,0.7);
		glVertex2d((*i).x,(*i).y); glVertex2d((*j).x,(*j).y);
		s = s+1;
	}
	glEnd() ;

}

void AXorDraw::clear(void)
{
	_points.clear();
}

void AXorDraw::add(float x, float y)
{
	_points.push_front(point(x,y,0,0));
	postRedisplay();
}

// --------------------------------------------------------------------

DropWindow::DropWindow(std::string name, WinRegions::region *r,
		       GLdouble cx, GLdouble cy) : sgNode(name)
{
	_x = _y = 0.0 ;
	_width = r->w;
	_height = r->h;
	_xhot = _yhot = 0;
	_r=_g=_b=_a=0.5;
}

void
DropWindow::display(dlPolicy policy)
{	
	glDisable(GL_DEPTH_TEST) ;

	glTranslatef(_x,_y,-0.01);
	glColor4f(_r,_g,_b,_a);
	glRectf(-_width/2, _height/2, _width/2, -_height/2);

	glColor4f(1,1,1, 1) ;
	glBegin(GL_LINE_LOOP);
	glVertex2d(-_width/2, _height/2);
	glVertex2d(_width/2,  _height/2);
	glVertex2d(_width/2,  -_height/2);
	glVertex2d(-_width/2, -_height/2);
	glEnd() ;
	glEnable(GL_DEPTH_TEST) ;
}

void
DropWindow::updatePosition(GLfloat x, GLfloat y)
{
	_x = x ;
	_y = y ;
	
	postRedisplay() ;
}

void
DropWindow::setColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
  _r = r; _g = g; _b = b; _a = a;
}


// --------------------------------------------------------------------

void
unproject(int x, int y, sgViewpoint *viewpoint, GLuint *selectionBuffer,
	  int selectionBufferSize,
	  GLdouble *ox, GLdouble *oy, GLdouble *oz) {
  static int first_time = 1;
  static int hack = 1;
  GLfloat z = -10.0 ;

  GLint viewport[4] ;
  glGetIntegerv(GL_VIEWPORT, viewport) ;

  viewpoint->applyTransforms() ;

  GLdouble projmatrix[16] ;
  glGetDoublev(GL_PROJECTION_MATRIX, projmatrix) ;

  for (int i=0; i<selectionBufferSize; ++i) {
    sgNode *o = sgNode::lookupId(selectionBuffer[i]) ;
    
#if 0
    std::cerr << o->getName() << " < " << std::flush ;
#endif
    o->applyTransformations() ;
  }

#if 0
  std::cerr << std::endl ;
#endif

  GLdouble mvmatrix[16] ;
  glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix) ;

  glReadBuffer(GL_FRONT) ;
  //glReadBuffer(GL_BACK) ;
  glReadPixels(x,viewport[3]-y-1, 1,1, GL_DEPTH_COMPONENT,GL_FLOAT, (GLvoid *)&z) ;

  glReadBuffer(GL_BACK) ;

  GLint ret;
  /* FIXME: !! */
  if (first_time == 1)
  {
	  if (z > 0 && z <= 0.04)
	  {
		  hack = 256;
	  }
	  first_time = 0;
  }
  ret = gluUnProject(x,viewport[3]-y-1,z*hack, mvmatrix, projmatrix, viewport, ox, oy, oz) ;
#if 0
  GLint d;
  glGetIntegerv(GL_DEPTH_BITS, &d);
  std::cerr << "depth bits: " << d << std::endl ;
  std::cerr << " x=" << x << " y=" << y << " (" << viewport[3]-y-1 << ") z=" << z << std::endl ;
  std::cerr << " ox=" << *ox << " oy=" << *oy << " oz=" << *oz << " ("<< ret << ")" << std::endl ;
#endif
}

void
unproject(
	int x, int y, float z, sgViewpoint *viewpoint, GLuint *selectionBuffer,
	int selectionBufferSize, GLdouble *ox, GLdouble *oy, GLdouble *oz)
{

	GLint viewport[4] ;
	glGetIntegerv(GL_VIEWPORT, viewport) ;

	viewpoint->applyTransforms() ;

	GLdouble projmatrix[16] ;
	glGetDoublev(GL_PROJECTION_MATRIX, projmatrix) ;

	for (int i=0; i<selectionBufferSize; ++i) {
	  sgNode *o = sgNode::lookupId(selectionBuffer[i]) ;
		o->applyTransformations() ;
	}

	GLdouble mvmatrix[16] ;
	glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix) ;

	glReadBuffer(GL_BACK) ;

	GLint ret;
	ret = gluUnProject(
		x,viewport[3]-y-1,z, mvmatrix, projmatrix, viewport, ox, oy, oz);
#if 0
	std::cerr << " x=" << x << " y=" << y << " (" << viewport[3]-y-1
		  << ") z=" << z << std::endl ;
	std::cerr << " ox=" << *ox << " oy=" << *oy << " oz=" << *oz
		  << " ("<< ret << ")" << std::endl ;
#endif
}

// --------------------------------------------------------------------

std::ostream &operator<<(std::ostream &s, const Point &p)
{
	s << p.x() << "," << p.y();
}

// --------------------------------------------------------------------

Rect::Rect(float x, float y, float width, float height) :
	_x(x),
	_y(y),
	_width(width),
	_height(height)
{
}


Rect::Rect(const Point &p1, const Point &p2, const Point &p3, const Point &p4)
{
	const float xmin = std::min(std::min(p1.x(), p2.x()),
				    std::min(p3.x(), p4.x()));
	const float xmax = std::max(std::max(p1.x(), p2.x()),
				    std::max(p3.x(), p4.x()));

	const float ymin = std::min(std::min(p1.y(), p2.y()),
				    std::min(p3.y(), p4.y()));
	const float ymax = std::max(std::max(p1.y(), p2.y()),
				    std::max(p3.y(), p4.y()));

	_x = xmin;
	_y = ymin;
	_width = xmax - xmin;
	_height = ymax - ymin;
}


bool
Rect::isNull()
{
	return (_width < 0.01) && (_height < 0.01);
}


Rect
Rect::intersection(const Rect &rect) const
{
	const float x = std::max(_x, rect._x);
	const float y = std::max(_y, rect._y);

	const float x2 = std::min(_x+_width,  rect._x+rect._width);
	const float y2 = std::min(_y+_height, rect._y+rect._height);

	const float width  = x2 - x;
	const float height = y2 - y;

	/*std::cerr << "intersection: x=" << x
		<< " y=" << y
		<< " width=" << width
		<< " height=" << height << std::endl;*/

	if (width <= 0 || height <= 0)
		return Rect(0, 0, 0, 0);
	else
		return Rect(x, y, width, height);
}


Rect
Rect::bounding(const Rect &rect) const
{
	const float x = std::min(_x, rect._x);
	const float y = std::min(_y, rect._y);

	const float x2 = std::max(_x+_width,  rect._x+rect._width);
	const float y2 = std::max(_y+_height, rect._y+rect._height);

	const float width  = x2 - x;
	const float height = y2 - y;

	/*std::cerr << "bounding: x=" << x
		<< " y=" << y
		<< " width=" << width
		<< " height=" << height << std::endl;*/

	return Rect(x, y, width, height);
}


Point Rect::center() const
{
	const float x = _x + _width/2.0;
	const float y = _y + _height/2.0;
	return Point(x, y);
}


void Rect::moveCenter(const Point& newCenter)
{
	_x += newCenter.x() - center().x();
	_y += newCenter.y() - center().y();
}


Transform2D Rect::findTransformTo(const Rect &r) const
{
	Eigen::Matrix3f Vects1;
	Vects1(0,0) = topLeft().x();
	Vects1(1,0) = topLeft().y();
	Vects1(2,0) = 1;
	Vects1(0,1) = topRight().x();
	Vects1(1,1) = topRight().y();
	Vects1(2,1) = 1;
	Vects1(0,2) = bottomLeft().x();
	Vects1(1,2) = bottomLeft().y();
	Vects1(2,2) = 1;

	Eigen::Matrix3f Vects2;
	Vects2(0,0) = r.topLeft().x();
	Vects2(1,0) = r.topLeft().y();
	Vects2(2,0) = 1;
	Vects2(0,1) = r.topRight().x();
	Vects2(1,1) = r.topRight().y();
	Vects2(2,1) = 1;
	Vects2(0,2) = r.bottomLeft().x();
	Vects2(1,2) = r.bottomLeft().y();
	Vects2(2,2) = 1;

	return Vects2 * Vects1.inverse();
}


std::ostream &operator<<(std::ostream &s, const Rect &r)
{
	s << r.width() << "x" << r.height()
		<< "\tTP=" << r.x() << "," << r.y() << "\tC=" << r.center();
}


bool operator==(const Rect &r1, const Rect &r2)
{
	return (r1.x() == r2.x()) &&
		(r1.y() == r2.y()) &&
		(r1.width() == r2.width()) &&
		(r1.height() == r2.height());
}


bool operator!=(const Rect &r1, const Rect &r2)
{
	return (r1.x() != r2.x()) ||
		(r1.y() != r2.y()) ||
		(r1.width() != r2.width()) ||
		(r1.height() != r2.height());
}
