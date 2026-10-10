/*
 *
 * main/AWidget.H --
 *
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef AWIDGET_H
#define AWIDGET_H

#include <nucleo/core/TimeKeeper.H>
#include <nucleo/gl/scenegraph/sgNode.H>
#include <nucleo/gl/scenegraph/sgViewpoint.H>

#ifdef HAVE_GLFT
#include <glft/FontManager.hpp>
#endif

#include "desktop/MetisseWindow.H"
#include "desktop/MetisseSource.H"

using namespace nucleo ;

class AScreen;
class MetisseWindow;

// ---------------------------------------------------------------------------
#define STACKW_ACTION_NONE    0
#define STACKW_ACTION_UNMAP   1
#define STACKW_ACTION_STACK   2

class ScrollLeafingWidget : public sgNode
{
private:
	AScreen *_ascr;
	sgNode *_parent;
	GLfloat _x, _y, _wx, _wy;
	GLfloat _width, _height;
	bool _add_desktop;
	bool _entred;
	int _num_cell;
	int _cur_cell;
	int _action;
	bool _should_umap;
	std::list<MetisseWindow *> _win_list;
	std::list<std::list<MetisseWindow *> *> _stack_list;

protected:
	void display(dlPolicy policy) ;

public:
	ScrollLeafingWidget(
		AScreen *ascr, sgNode *parent, WinRegions::region *r,
		bool add_desktop = false, bool restack = true);
	~ScrollLeafingWidget(void);
	void _detachFromParent(void);
	int  getAction();
	void updatePosition(GLfloat x, GLfloat y) ;
};

// ---------------------------------------------------------------------------

#define TRAILING_ACTION_NONE 0
#define TRAILING_ACTION_TL   1
#define TRAILING_ACTION_TR   2
#define TRAILING_ACTION_BL   3
#define TRAILING_ACTION_BR   4

class TrailWidget : public sgNode, public ReactiveObject
{

public:
	struct timed_point
	{
		double x,y ;
		unsigned int t;
		timed_point(GLfloat a, GLfloat b, unsigned int c) :
			x(a), y(b), t(c) {}
	};

private:
	AScreen *_ascr;
	GLfloat _x, _y;
	std::list<TrailWidget::timed_point > _points;
	GLfloat _width, _height,_cap_x,_cap_y,_hsx,_hsy;
	GLfloat _a,_r,_g,_b;
	GLUquadric *_qobj;
	TimeKeeper *_tk;
	TimeKeeper *_untk;
	int _captured;
	int _action;
	sgNode *_child;
	
	void _checkAction(GLfloat x, GLfloat y);

protected:
	void display(dlPolicy policy) ;
	void react(Observable *obs);

public:
	TrailWidget(AScreen *ascr, WinRegions::region *r);
	~TrailWidget(void);
	bool isCaptured(void);
	void popupMenu();
	int  getAction();
	void unCapture(void);
	void setChild(sgNode *sg);
	void updatePosition(GLfloat x, GLfloat y) ;
	void setColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
} ;

// ---------------------------------------------------------------------------

#define AUTILS_MAX_BUTTONS 8

class VMouse : public sgNode, public ReactiveObject
{
private:
	GLfloat _x, _y;
	GLfloat _width, _height;
	TimeKeeper *_tk[AUTILS_MAX_BUTTONS];
	bool _buttons[AUTILS_MAX_BUTTONS];

protected:
	void react(Observable *obs);
	void display(dlPolicy policy);

public:
	void updateState(int button, bool on);
	bool checkClick(float x, float y);
	void move(int dx, int dy);
	VMouse(
		std::string name,
		GLdouble x, GLdouble y, GLdouble w, GLdouble h);
	~VMouse(void);
};

// ---------------------------------------------------------------------------

class PerfMeter : public sgNode {
private:
	Chronometer _chrono ;
protected:
	void display(dlPolicy policy) ;
public:
	PerfMeter(void) : sgNode("PerfMeter",0) {}
	void restart(void) {
		_chrono.start() ;
	}
};

#endif
