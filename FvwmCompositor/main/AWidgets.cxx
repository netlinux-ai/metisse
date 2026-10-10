/*
 *
 * main/AWidget.cxx --
 *
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config-not-xserver.h"
#endif

#include "AWidgets.H"

#include "main/AScreen.H"
#include "desktop/MetisseWindow.H"


// --------------------------------------------------------------------

ScrollLeafingWidget::ScrollLeafingWidget(
	AScreen *ascr, sgNode *parent, WinRegions::region *r, bool add_desktop,
	bool restack) : 
	sgNode("stacking widget")
{
	_ascr = ascr;
	_parent = parent;
	_wx = _x = r->x;
	_wy = _y = r->y;
	_width = r->w;
	_height = r->h;
	_add_desktop = add_desktop;
	_entred = false;
	_num_cell = 0;
	_cur_cell = -1;
	_should_umap = false;
	_action = STACKW_ACTION_NONE;
	//

	_win_list = _ascr->root->getWindowOnCurrentPage();
	_stack_list = _ascr->root->getWindowOnCurrentPageByStack(add_desktop);
	_num_cell = _stack_list.size();
	if (_num_cell == 0)
	{
		_num_cell = 1;
	}
	// FIXME
	if (false)
	{
		_wx = _x + _width + _width;
		_wy = _y - _num_cell*_height/2;
	}
	else
	{
		_wx = _x - _width/2;
		_wy = _y - _num_cell*_height + _height/2;
	}
}

void ScrollLeafingWidget::_detachFromParent(void)
{
	if (_parent == 0)
		return;

	TrailWidget *tw = dynamic_cast<TrailWidget *>(_parent);

	if (tw != 0)
	{
		tw->setChild(0);
	}
}

ScrollLeafingWidget::~ScrollLeafingWidget(void)
{
	_detachFromParent();

	std::list<std::list<MetisseWindow *> *>::iterator  it;
	for (it=_stack_list.begin(); it!= _stack_list.end(); ++it)
	{
		delete (*it);
	}
}

int ScrollLeafingWidget::getAction()
{
	return _action;
}

void ScrollLeafingWidget::display(dlPolicy policy)
{
	int i;
	int j;

	glPushMatrix();
	glTranslatef(_wx, _wy, -0.1);

	for (j=1; j <4; j++)
	{
		glColor4f(0, 0, 0, 0.7/j);
		glBegin(GL_LINE_LOOP);
		glVertex2d(-j, -j);
		glVertex2d(_width+j,  -j);
		glVertex2d(_width+j,_num_cell*_height+j);
		glVertex2d(-j, _num_cell*_height+j);
		glEnd() ;
	}

	for (i = 0; i < _num_cell; i++)
	{
		if (i == _cur_cell)
		{
			glColor4f(0.9, 0.9, 0.9, 0.7);
		}
		else
		{
			glColor4f(0, 0, 0, 0.7);
		}
		glRectf(0, 0, _width, _height);
		glColor4f(1,1,1, 1) ;
		glBegin(GL_LINE_LOOP);
		glVertex2d(0, 0);
		glVertex2d(_width,  0);
		glVertex2d(_width,_height);
		glVertex2d(0, _height);
		glEnd() ;
		glTranslatef(0,_height,0);
	}
	glPopMatrix();

}

void
ScrollLeafingWidget::updatePosition(GLfloat x, GLfloat y)
{
	_x = x ;
	_y = y ;
	
	int nc = -1;

	GLfloat rx = _x - _wx;
	GLfloat ry = _y - _wy;

	if (rx >= 0 && rx <= _width && ry >= 0 && ry <= _num_cell*_height)
	{
		nc = (int)(ry/_height);
	}

	if (!_entred && nc != -1)
	{
		_entred = true;
	}
	else if (_entred && nc == -1)
	{
		_action = STACKW_ACTION_UNMAP;
		_entred = false;
	}
	if (_entred && nc != -1 && nc != _cur_cell)
	{
		//_action = STACKW_ACTION_STACK;
		int c = 0;
		std::list<MetisseWindow * >::iterator win_it;
		std::list<std::list<MetisseWindow *> *>::iterator  it;
		MetisseWindow *win = 0;
		std::list<MetisseWindow *> *wl = 0;
		for (it=_stack_list.begin(); it!= _stack_list.end(); ++it)
		{
			if (c == nc)
			{
				wl = (*it);
				break;
			}
			c++;
		}
		if (wl)
		{
			for (win_it=wl->begin(); win_it!= wl->end(); ++win_it)
			{
				win = (*win_it);
				if (win->isEwmhDesktop())
				{
					_ascr->sendFvwmCmd(
						"MyShowDesktopOn Nop Nop", 0);
				}
				else if (_add_desktop)
				{
					// FIXME: do it only if needed
					fprintf(stderr,"MyShowDesktopOff\n");
					_ascr->sendFvwmCmd(
						"MyShowDesktopOff", 0);
				}
				_ascr->sendFvwmCmd("Silent Raise", win);
			}
		}
	}
	if (_entred)
	{
		_cur_cell = nc;
	}
	postRedisplay() ;
}

// --------------------------------------------------------------------

#define TRAIL_DIFF_TIME 60
#define TRAIL_DIST      50
#define TRAIL_CAPTURE_TIME 200
#define TRAIL_UNCAPTURE_TIME TRAIL_CAPTURE_TIME+200
#define TRAIL_UNCAPTURE_TIME_LONG 50000

TrailWidget::TrailWidget(
	AScreen *ascr, WinRegions::region *r) : sgNode("trail widget")
{
	_ascr = ascr;
	_points.clear();
	_x = r->x;
	_y = r->y;
	_hsx = _hsy = 0;
	_width = r->w;
	_height = r->h;
	_r=_g=_b=_a=0.5;
	_qobj = gluNewQuadric();
	gluQuadricDrawStyle(_qobj, GLU_FILL);
	//gluQuadricNormals(_qobj, GLU_NONE);
	_tk = TimeKeeper::create();
	_untk = TimeKeeper::create();
	_tk->arm(TRAIL_DIFF_TIME);
	subscribeTo(_tk);
	subscribeTo(_untk);
	_captured = 0;
	_action = TRAILING_ACTION_NONE;
	_child = 0;
}

TrailWidget::~TrailWidget(void)
{
	gluDeleteQuadric(_qobj);
	delete _tk;
	delete _untk;
}

bool TrailWidget::isCaptured(void)
{
	return (_captured == 3);
}

void TrailWidget::popupMenu()
{
	if (_child != 0)
	{
		return;
	}
	_captured = 3;
	_cap_x = _x;
	_cap_y = _y;
	_tk->disarm();
	_untk->disarm();
	_untk->arm(TRAIL_UNCAPTURE_TIME_LONG);
	_r = 0;
	_g = 1;
	_b = 0;
	_changed = true;
}

int TrailWidget::getAction()
{
	return _action;
}

void TrailWidget::unCapture(void)
{
	_captured = 0;
	_action = TRAILING_ACTION_NONE;
	_r = 1;
	_g = 0;
	_b = 0;
}

void TrailWidget::react(Observable *obs)
{
	//fprintf(stderr, "REACT\n");
	if (obs == _tk)
	{
		if (_captured == 0)
		{
			_tk->disarm();
			//_tk->arm(TRAIL_CAPTURE_TIME);
		}
		else if (_captured == 1)
		{
			fprintf(stderr, "CAPTURE == 3\n");
			_captured = 3;
			_cap_x = _x;
			_cap_y = _y;
			_r = 0;
			_g = 1;
			_b = 0;
			_untk->disarm();
			_untk->arm(TRAIL_UNCAPTURE_TIME_LONG);
		}
	}
	else if (obs == _untk)
	{
		fprintf(stderr, "CAPTURE == 0\n");
		unCapture();
	}
	postRedisplay();
}

void TrailWidget::_checkAction(GLfloat x, GLfloat y)
{
	_action = TRAILING_ACTION_NONE;

	#if 0
	fprintf(stderr,
		"_checkAction %f %f %f %f %f %f\n",_x,_y,_cap_x,_cap_y, x,y);
	#endif

	if (!(fabs(x) >= 4*_width && fabs(y) >= 4*_width))
	{
		return;
	}
	
	if (x > 0 && y < 0)
	{
		_action = TRAILING_ACTION_BR;
	}
	else if (x > 0 && y > 0)
	{
		_action = TRAILING_ACTION_TR;
	}
	else if (x < 0 && y < 0)
	{
		_action = TRAILING_ACTION_BL;
	}
	else if (x < 0 && y > 0)
	{
		_action = TRAILING_ACTION_TL;
	}
		
}

void TrailWidget::display(dlPolicy policy)
{
	int size = _points.size();
	if (size < 1)
	{
		return;
	}

	GLfloat x,y;
	x = _x;
	y = _y;

	if (!_captured)
	{
		std::list<TrailWidget::timed_point >::iterator i;
		TimeStamp now ;
		TimeStamp::inttype now_as_int = now.getAsInt();
		for (i=_points.begin(); i != _points.end(); i++)
		{
			if (i == _points.begin())
			{
				x = (*i).x;
				y = (*i).y;
				continue;
			}
			//fprintf(stderr, "DT: %i\n",now_as_int - (*i).t);
			if ((int)now_as_int - (int)(*i).t <= TRAIL_DIFF_TIME)
			{
				x = (*i).x;
				y = (*i).y;
			}
			else
			{
				break;
			}
		}
	}

	// FIXME: remove the no more needed points ...

	if (_captured == 3)
	{
		//x = _cap_x;
		//y = _cap_y;
	}

	glTranslatef(x,y,0);

	if (_captured >= 2)
	{
		if (_captured == 3 || true)
		{
			glTranslatef(_cap_x - x, _cap_y - y,0);
		}
		GLfloat w = 8*_width;
		int loop = 360;
		glColor4f(1,1,0, 1); // yellow
		gluPartialDisk(_qobj, _width+1, w, loop, 1,0,90);
		glColor4f(1,0,1, 0); // trans cancle
		gluPartialDisk(_qobj, _width+1, w, loop, 1,90,90);
		glColor4f(1,0,1, 1); // 
		gluPartialDisk(_qobj, _width+1, w, loop, 1,180,90);
		glColor4f(0,1,1, 1); // cyan
		gluPartialDisk(_qobj, _width+1, w, loop, 1,270,90);
		glColor4f(0.5,0.5,0.5,1); 
		// shadows
		gluPartialDisk(_qobj, w, w+1, loop, 1, 180,270);
		glColor4f(0.5,0.5,0.5,0.7); 
		gluPartialDisk(_qobj, w+1, w+2, loop, 1, 180,270);
		glColor4f(0.5,0.5,0.5,0.4); 
		gluPartialDisk(_qobj, w+2, w+3, loop, 1, 180,270);
		glColor4f(0.5,0.5,0.5,0.3); 
		gluPartialDisk(_qobj, w+3, w+4, loop, 1, 180,270);
		// FIXME: lines
		// activation zone
		glColor4f(1,1,1,1); 
		gluPartialDisk(_qobj, w/2, 1 + w/2, loop, 1, 180,270);
		glTranslatef(-_cap_x + x, -_cap_y + y,0);
	}

	glColor4f(_r,_g,_b,_a);
	gluDisk(_qobj, 0, _width, 16, 1);
	glColor4f(1,1,1,0.5);
	gluDisk(_qobj, _width, _width+1, 16, 1);

	if (!_captured && _child == 0)
	{
		#if 0
		fprintf(stderr, "ACTION ");
		if (x+TRAIL_DIST  < _x)
			fprintf(stderr, "X ");
		if (y-TRAIL_DIST  >_y)
			fprintf(stderr, "Y ");
		fprintf(stderr, "\n");
		#endif
		if (x+TRAIL_DIST < _x && y-TRAIL_DIST > _y)
		{
			_r = 0;
			_g = 0;
			_b = 1;
			_hsx = _x;
			_hsy = _y;
			_captured = 1;
			_tk->disarm();
			_tk->arm(TRAIL_CAPTURE_TIME);
			_untk->disarm();
			_untk->arm(TRAIL_UNCAPTURE_TIME);
			fprintf(stderr, "CAPTURE == 1\n");
		}
	}
	else if (_captured == 1)
	{
		//_tk->disarm();
		//_tk->arm(TRAIL_CAPTURE_TIME);
	}
}

void TrailWidget::setChild(sgNode *sg)
{
	_child = sg;
}

void TrailWidget::updatePosition(GLfloat x, GLfloat y)
{
	_x = x ;
	_y = y ;

	TimeStamp now ;
	TimeStamp::inttype now_as_int = now.getAsInt() ;
	if (_captured == 1)
	{
		// need a stop 
		_tk->disarm();
		_tk->arm(TRAIL_CAPTURE_TIME);
		if (fabs(_hsx -_x) > 20 || fabs(_hsy -_y) > 20)
		{
			unCapture();
		}
	}
	if (_captured >= 2 || _child != 0)
	{
		_points.push_front(timed_point(x,y,now_as_int));
	}
	else
	{
		_points.push_front(
			timed_point(x+TRAIL_DIST,y-TRAIL_DIST,now_as_int));
	}
	if (_captured >= 3)
	{
		// should check action !
		_checkAction(x - _cap_x, y - _cap_y);
	}
	//fprintf(stderr, "%i %i %i\n",(int)x,(int)y,now_as_int);
	postRedisplay() ;
}

void
TrailWidget::setColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
  _r = r; _g = g; _b = b; _a = a;
}

// --------------------------------------------------------------------
void
VMouse::display(dlPolicy policy)
{	
	glDisable(GL_DEPTH_TEST) ;

	glTranslatef(_x,_y,-0.01);
	glColor4f(0.5,0.5,0.5,0.5);

	glRectf(-_width/2, _height/2, _width/2, -_height/2);

	glColor4f(1,1,1, 1) ;
	glBegin(GL_LINE_LOOP);
	glVertex2d(-_width/2, _height/2);
	glVertex2d(_width/2,  _height/2);
	glVertex2d(_width/2,  -_height/2);
	glVertex2d(-_width/2, -_height/2);
	glEnd() ;

	GLdouble w = _width/3;
	GLdouble h = _height/3;

	for (int i = 0; i < 3; i++)
	{
		glColor4f(1,1,1, 1) ;
		glBegin(GL_LINE_LOOP);
		glVertex2d(-_width/2 + (w*i),      _height/2);
		glVertex2d(-_width/2 + (w*(i+1)),  _height/2);
		glVertex2d(-_width/2 + (w*(i+1)),  (_height/2) - h);
		glVertex2d(-_width/2 + (w*i),      (_height/2) - h);
		glEnd();
		
		if (_buttons[i])
		{
			glColor4f(1,0,0, 0.7);
			glRectf(
				-_width/2 + (w*i), _height/2, -_width/2 + (w*(i+1)),
				(_height/2) - h);
		}
	}

	GLdouble ww = _width/10;
	GLdouble hw = _height/10;

	for (int i = 0; i < 2; i++)
	{
		glColor4f(1,1,1, 1) ;
		glBegin(GL_LINE_LOOP);
		glVertex2d(-ww, ((i==0)? 1:-1)*hw);
		glVertex2d(ww,  ((i==0)? 1:-1)*hw);
		glVertex2d(0,  0);

		glEnd();
		
		if (_buttons[3+i])
		{
			glColor4f(1,0,0, 0.7);
			glBegin(GL_POLYGON);
			glVertex2d(-ww, ((i==0)? 1:-1)*hw);
			glVertex2d(ww,  ((i==0)? 1:-1)*hw);
			glVertex2d(0,  0);
			glEnd();
		}
	}

	for (int i = 0; i < 2; i++)
	{
		glColor4f(1,1,1, 1) ;
		glBegin(GL_LINE_LOOP);
		glVertex2d(((i==0)? 1:-1)*ww, hw);
		glVertex2d(((i==0)? 1:-1)*ww, -hw);
		glVertex2d(0,  0);
		glEnd();
		
		if (_buttons[5+i])
		{
			glColor4f(1,0,0, 0.7);
			glBegin(GL_POLYGON);
			glVertex2d(((i==0)? 1:-1)*ww, hw);
			glVertex2d(((i==0)? 1:-1)*ww, -hw);
			glVertex2d(0,  0);
			glEnd();
		}
	}
	glEnable(GL_DEPTH_TEST) ;
}
void VMouse::react(Observable *obs)
{
	for(int i = 0; i < AUTILS_MAX_BUTTONS; i++)
	{
		if (obs == _tk[i])
		{
			_buttons[i] = false;
			unsubscribeFrom(_tk[i]);
			postRedisplay();
		}
	}
}

bool VMouse::checkClick(float x, float y)
{
	if (x > _x-_width/2 && x < _x+_width/2 && y > _y-_width/2 && y < _y+_height/2)
	{
		return true;
	}
	return false;
}

void VMouse::move(int dx, int dy)
{
	_x = - dx + _x;
	_y = dy + _y;
	postRedisplay();
}

void VMouse::updateState(int button, bool on)
{
	button--;
	if (button < 0 || button > AUTILS_MAX_BUTTONS-1)
	{
		return;
	}
	//std::cerr << "updateState: " << button << " " << on << std::endl;
	if (on)
	{
		_buttons[button] = on;
	}
	else if (button < 3)
	{
		_buttons[button] = on;
	}
	else
	{
		if (_tk[button] == 0)
		{
			_tk[button] = TimeKeeper::create();
		}
		subscribeTo(_tk[button]);
		_tk[button]->arm(25);
	}
	postRedisplay();
}

VMouse::VMouse(
	std::string name, GLdouble x, GLdouble y, GLdouble w, GLdouble h) : sgNode(name)
{
	_x = x;
	_y = y;
	_width = w;
	_height = h;
	
	memset(&_buttons, 0, sizeof(_buttons));
	memset(&_tk, 0, sizeof(_tk));
}

VMouse::~VMouse(void)
{
	for(int i = 0; i < AUTILS_MAX_BUTTONS; i++)
	{
		if (_tk[i] != 0)
		{
			unsubscribeFrom(_tk[i]);
			delete _tk[i];
		}
	}
}

// --------------------------------------------------------------------
void
PerfMeter::display(dlPolicy policy) {
  _chrono.tick() ;

  if (_chrono.read() > 0 /*5000.0*/) {
    std::cerr << _chrono.average() 
		    << " fps (" << _chrono.read()/1000.0
		    << " seconds elapsed)" << std::endl;
    _chrono.start() ;
  }
}

