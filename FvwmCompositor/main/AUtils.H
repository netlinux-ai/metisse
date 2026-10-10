#ifndef AUtils_H
#define AUtils_H

/*
 *
 * AUtils.H --
 *
 * Copyright (C) Nicolas Roussel Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <nucleo/core/TimeKeeper.H>
#include <nucleo/gl/scenegraph/sgNode.H>
#include <nucleo/gl/scenegraph/sgViewpoint.H>

#ifdef HAVE_GLFT
#include <glft/FontManager.hpp>
#endif

#include <eigen/vector.h>
#include <eigen/matrix.h>

#include "desktop/MetisseWindow.H"
#include "desktop/MetisseSource.H"

using namespace nucleo ;

class AScreen;

// ---------------------------------------------------------------------------

class DefaultSettings : public sgNode {
protected:
  void display(dlPolicy policy) ;
public:
  DefaultSettings(void) ;
} ;


// ---------------------------------------------------------------------------

class AXorDraw : public sgNode {

public:

struct point
{
  float x,y;
  float w;
  unsigned int flags;
  point(float a, float b, float c, unsigned int d) :
    x(a), y(b), w(c), flags(d) {}
  point(void) { point(0.0,0.0,0.0,0); }
};

private:
  std::list<AXorDraw::point > _points;

protected:
  void display(dlPolicy policy);

public:
  AXorDraw(std::string name, int *points, int num, int loop = 0);
  AXorDraw(std::string name, float x, float y);
  //~AXorDraw();

  void clear(void);
  void add(float x, float y);
} ;

// ---------------------------------------------------------------------------

class DropWindow : public sgNode {
private:
  GLfloat _x, _y;
  GLfloat _xhot, _yhot, _width, _height;
  GLfloat _a,_r,_g,_b;

protected:
  void display(dlPolicy policy) ;

public:
  DropWindow(std::string name, WinRegions::region *r, GLdouble cx, GLdouble cy);
  void updatePosition(GLfloat x, GLfloat y) ;
  void setColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
} ;


// ---------------------------------------------------------------------------

void
unproject(int x, int y,
	  sgViewpoint *viewpoint, GLuint *selectionBuffer, int selectionBufferSize,
	  GLdouble *ox, GLdouble *oy, GLdouble *oz) ;

void
unproject(int x, int y, float z, sgViewpoint *viewpoint, GLuint *selectionBuffer,
	  int selectionBufferSize,
	  GLdouble *ox, GLdouble *oy, GLdouble *oz);

int wc2scX(GLfloat x, GLfloat hw, GLfloat rx);
int wc2scY(GLfloat y, GLfloat hh, GLfloat ry);

// ---------------------------------------------------------------------------

typedef Eigen::Vector2f Point;
std::ostream &operator<<(std::ostream &s, const Point &p);

typedef Eigen::Matrix3f Transform2D;

// ---------------------------------------------------------------------------

class Rect
{
public:
	Rect() : _x(0), _y(0), _width(0), _height(0) {}
	Rect(float x, float y, float width, float height);
	Rect(const Rect &r) :
		_x(r._x), _y(r._y), _width(r._width), _height(r._height) {}
	Rect(const Point&, const Point&, const Point&, const Point&);

	bool isNull();
	Rect intersection(const Rect&) const;
	Rect bounding(const Rect&) const;
	Point center() const;
	void moveCenter(const Point&);

	float x() const { return _x; }
	float y() const { return _y; }
	float width() const { return _width; }
	float height() const { return _height; }

	float &rx() { return _x; }
	float &ry() { return _y; }
	float &rwidth() { return _width; }
	float &rheight() { return _height; }

	float bottom() const { return _y+_height; }
	float left() const { return _x; }
	float right() const { return _x+_width; }
	float top() const { return _y; }

	Point bottomLeft() const { return Point(left(), bottom()); }
	Point bottomRight() const { return Point(right(), bottom()); }
	Point topLeft() const { return Point(left(), top()); }
	Point topRight() const { return Point(right(), top()); }

	Transform2D findTransformTo(const Rect&) const;

private:
	float _x, _y, _width, _height;
};

std::ostream &operator<<(std::ostream &s, const Rect &r);
bool operator==(const Rect &r1, const Rect &r2);
bool operator!=(const Rect &r1, const Rect &r2);

// ---------------------------------------------------------------------------

#if 0
void *get_property(
	Display *disp, Window win, Atom atom, Atom type, unsigned int *size);
#endif

#endif
