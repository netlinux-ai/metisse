/*
 *
 * main/LayerManaer.cxx --
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

#include "LayerManager.H"

#include <X11/X.h>

#include "main/AScreen.H"
#include "desktop/MetisseWindow.H"
#define A_PAGER_MODE_MARGIN_P 0.02

using namespace std;

// --------------------------------------------------------------------

void LayerManager::preDisplay(void)
{
	float totalDepth = 0;

	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	    o != _dependencies.end(); 
	    o++)
	{
		bool u = false;
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (!win || win->isHidden() || !win->doDraw())
		{
			continue;
		}
		win->preDisplay(
			_desktopWidth, _desktopHeight, _eyeDist, totalDepth);
	}
}

void LayerManager::display(dlPolicy policy)
{
	// top left
	float x = - (_desktopWidth/2.0) - _pagerVx + _transX;
	float y = (_desktopHeight/2.0) + _pagerVy + _transY;

	glDisable(GL_DEPTH_TEST);

#if __APPLE__
	// NICOLAS_N_AIME_PAS_LA_COULEUR
	// GLfloat bottom_color[3] = {0.3,0.3,0.3} ;
	// GLfloat top_color[3] = {0.8,0.8,0.8} ;
	GLfloat bottom_color[3] = {1,1,1} ;
	GLfloat top_color[3] = {1,1,1} ;
#else
	// OLIVIER_PREFERE_LE_DEGRADE_GRIS 
	//GLfloat top_color[3] = {0.308,0.1875,0.031} ;
	//GLfloat bottom_color[3] = {0.746, 0.46, 0.066} ;
	GLfloat bottom_color[3] = {0.3,0.3,0.3} ;
	GLfloat top_color[3] = {0.8,0.8,0.8} ;
#endif

	glBegin(GL_POLYGON) ;
	glColor3fv(top_color) ;
	glVertex2f(x, y);
	glVertex2f(x + _pagerVWidth, y);
	glColor3fv(bottom_color) ;
	glVertex2f(x + _pagerVWidth, y - _pagerVHeight);
	glVertex2f(x, y - _pagerVHeight);
	glEnd();

	int i;
	//glTranslatef(0, 0, +0.5);
	glColor3f(0,0,0);
	glBegin(GL_LINES);
	for (i = 1; i < _pagerVxPages; i++)
	{
	  glVertex2f(x + i*_desktopWidth, y);
	  glVertex2f(x + i*_desktopWidth, y - _pagerVHeight);
	}
	for (i = 1; i < _pagerVyPages; i++)
	{
	  glVertex2f(x, y - i*_desktopHeight);
	  glVertex2f(x +  _pagerVWidth, y - i*_desktopHeight);
	}
	glEnd();


	float totalDepth = 0;

	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	    o != _dependencies.end(); 
	    o++)
	{
		bool u = false;
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (!win || win->isHidden() || !win->doDraw())
		{
			continue;
		}
		if (_eyeDist == 0)
		{
			// FIXME: ortho proj
			glTranslatef(0,0,totalDepth);
		}
		(*o)->displayGraph(policy) ;
	}
}

void
LayerManager::select(void)
{
	float totalDepth = 0;

	glDisable(GL_DEPTH_TEST);

	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	    o != _dependencies.end();
	    o++)
	{
		bool u = false;
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (!win || win->isHidden())
		{
			continue;
		}
		if (_eyeDist == 0)
		{
			// FIXME: ortho proj
			glTranslatef(0,0,totalDepth);
		}

		(*o)->selectGraph() ;
	}
}

MetisseWindow *LayerManager::getUnmanagedWindow(int x, int y)
{
	MetisseWindow *rw = 0;

	for( std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o )
	{
		bool ovre = false;
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (!win || !win->isUnmanaged() || win->isHidden())
		{
			continue;
		}
		int wX,wY,wW,wH;
		win->getSize(&wW, &wH);
		win->getRealPosition(&wX, &wY);

		if (x >= wX && x <= wX + wW && y >= wY && y <= wY + wH)
		{
			rw = win;
		}
	}
	return rw;
}

MetisseWindow *LayerManager::getUnmanagedWindow(void)
{
	MetisseWindow *rw = 0;

	for( std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o )
	{
		bool ovre = false;
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (!win || !win->isUnmanaged() || win->isHidden())
		{
			continue;
		}
		return rw;
	}
	return rw;
}

void
LayerManager::setRootWin(sgNode *rootWin)
{
	_rootWin = rootWin;
}

MetisseWindow *LayerManager::getRootWindow()
{
	MetisseWindow *win = dynamic_cast<MetisseWindow *>(_rootWin);
	return win;
}

#if 0
MetisseWindow *LayerManager::getFirstNormalWindow(void)
{
	MetisseWindow *rw = 0;
	
	for( std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o )
	{
		bool ovre = false;
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (!win || win->isUnmanaged() || win->isHidden())
		{
			continue;
		}

		rw = win;
	}
	return rw;	
}
#endif

void LayerManager::restack(sgNode *node, sgNode *nodeNext, bool add)
{
	sgNode *checkNext = 0;
	std::list<sgNode *>::iterator prev;
	int pos = 0, newPos = 0;

	if (!add)
	{
		for(std::list<sgNode *>::iterator o=_dependencies.begin();
		    o != _dependencies.end(); o++)
		{
			pos++;
			if ((*o) == node)
			{
				break;
			}
		}
		_dependencies.remove(node) ;
	}

	if (nodeNext)
	{
		prev = _dependencies.begin();
		for(std::list<sgNode *>::iterator o=_dependencies.begin();
		    o != _dependencies.end(); o++)
		{
			prev = o;
			if ((*o) == nodeNext)
			{
				checkNext = nodeNext;
				break;
			}
		}
	}

	if (checkNext == 0)
	{
		if (nodeNext)
		{
		  //std::cerr << "restacking: Warnning unknown next window"
		  //	  << std::endl;
		}
		// add at the top of the stacking
		_dependencies.push_back(node) ;
	}
	else
	{
		// add it before checkNext
		_dependencies.insert(prev, node);	
	}

#if 0
	_printStacking();
#endif

	if (!add)
	{
		for(std::list<sgNode *>::iterator o=_dependencies.begin();
		    o != _dependencies.end(); 
		    ++o )
		{
			newPos++;
			if ((*o) == node)
			{
				break;
			}
		}
	}

	if (!add && pos == newPos)
	{
		//std::cerr << "Stacking: SAME POS" << std::endl;
		// ???
		return;
	}

	pos = 0;
	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	    o != _dependencies.end(); o++)
	{
		pos++;
		MetisseWindow *w = dynamic_cast<MetisseWindow *>(*o);
		if (!w)
		{
			continue;
		}
		w->setStackingPos(pos);
	}

#if 0
	_printStacking();
#endif

	_changed = true;
}

// for debuging
void
LayerManager::_printStacking(void)
{
	std::cerr << "Stacking: ";

	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	    o != _dependencies.end(); ++o )
	{
		MetisseWindow *w = dynamic_cast<MetisseWindow *>(*o);
		if (!w)
		{
			std::cerr << "??? ";
			continue;
		}
		std::cerr << w->getStackingPos() << "-" << w->getTitle() << " ";
	}

	std::cerr << std::endl;
}

// --------------------------------------------------------------------
// Inititialisation

void LayerManager::setDesktopProperties(
	float desktopWidth, float desktopHeight, double eyeDist,
	double near, double far, sgViewpoint *viewpoint, double vscale,
	double extScaleX, double extScaleY, double extZRotate)
{
	_desktopWidth = desktopWidth;
	_desktopHeight = desktopHeight;
	_pagerVWidth = _pagerVxPages * _desktopWidth;
	_pagerVHeight = _pagerVxPages * _desktopHeight;

	_vScale = vscale;
	sgNode::scale(1/_extScaleX, 1/_extScaleY, 1);
	sgNode::rotate(-_extZRotate,0,0,-extZRotate);
	_totalScaleX = _extScaleX = extScaleX;
	_totalScaleY =_extScaleY = extScaleY;
	_extZRotate = extZRotate;

	_eyeDist = eyeDist;
	_viewpoint = viewpoint;
	sgNode::scale(_extScaleX, _extScaleY, 1);
	sgNode::rotate(_extZRotate,0,0,-extZRotate);
}

LayerManager::LayerManager(AScreen *ascr, std::string name) : sgNode(name)
{
	_ascr = ascr;
	_desktopWidth = 0.0;
	_desktopHeight = 0.0;

	_vScale = 1.0;
	_totalScaleX =_extScaleX = 1.0;
	_totalScaleY =_extScaleY = 1.0;
	_extZRotate = 0;

	_pagerVxPages = 1;
	_pagerVyPages = 1;
	_pagerVx = 0;
	_pagerVy = 0;
	_pagerCurrentDesk = 0;
	_pagerVWidth = _pagerVxPages * _desktopWidth;
	_pagerVHeight = _pagerVxPages * _desktopHeight;

	_eyeDist = 0.0;

	_transX = 0;
	_transY = 0;
	_scaleX = 1;
	_scaleY = 1;

	_surfaceTop = _surfaceLeft = _surfaceBottom = _surfaceRight =
		SURFACE_DEFAULT_CUT;
	_surfaceX = _surfaceY = SURFACE_DEFAULT_CUT;
	_surface = SURFACE_DEFAULT;
	// no surface by default;
	_surfaceOn = false;

	_bg = 0;
	_rootWin = 0;

	_inExposeMode = false;
	_inPagerMode = false;
	_schedulePagerModeOff = false;
	_pagerModeTk = 0;
	_saveFrameTk = 0;

	_snapAttractionDist = DEFAULT_SNAP_ATTRACTION_DIST;
	_snapAttracMode = DEFAULT_SNAP_ATTRAC_MODE;
}

// --------------------------------------------------------------------
// reaction

#define PAGER_MODE_ANIME_STEP 10
#define PAGER_MODE_ANIME_MS   10

void LayerManager::react(Observable *obs)
{
	if (_pagerModeTk == obs)
	{
		scale((_scaleX - _pagerModeStepX)/_pagerModePrevSX, 
		      (_scaleY - _pagerModeStepY)/_pagerModePrevSY,
		      1);
		_pagerModePrevSX = _scaleX;
		_pagerModePrevSY = _scaleY;

		_pagerModeStep++;

		if (_pagerModeStep >= PAGER_MODE_ANIME_STEP)
		{
			unsubscribeFrom(_pagerModeTk);
			delete _pagerModeTk;
			_pagerModeTk = 0;
			if (_inPagerMode)
			{
				scale(_pagerModeSX, _pagerModeSY, 1);
			}
			else
			{
				resetTransformations();
			}
			_ascr->draw(false);
			// do not reset the selection as we warp the cursor!
			//_ascr->resetSelection();
			_ascr->fixCursorPos(false);
		}
		else
		{
			_pagerModeTk->arm(PAGER_MODE_ANIME_MS);
			_ascr->draw(false);
		}
	}
	else if (obs == _exposeModeTk)
	{
		float r;
		if (_inExposeMode)
			r = (float)_exposeModeStep / 10.0;
		else
			r = 1.0 - (float)_exposeModeStep / 10.0;

		std::vector<Slot>::iterator slot = _exposeModeList.begin();
		for ( ; slot != _exposeModeList.end(); slot++ )
		{
			float wH,wW;

			if (slot->win == 0 ||
			    !_ascr->getMetisseDesktop()->isWindowValide(
				    slot->win))
			{
				continue;
			}

			// FIXME: should validate window

			slot->win->getSize(&wW,&wH);

			const float sX = slot->rect.x() - (- slot->rect.width() + wW)/2;
			const float sY = slot->rect.y() - (- slot->rect.height() + wH)/2;
			const float sScale = min(
				slot->rect.height()/wH, slot->rect.width()/wW);

			const float scale = r*sScale + (1.0-r);

			slot->win->unforceCurrentTranslation();
			slot->win->getRenderer()->restoreSavedTransformations();

			slot->win->forceCurrentTranslation(
				r*sX + (1.0-r)*slot->orig.x(),
				r*sY + (1.0-r)*slot->orig.y());
			scaleWindow(slot->win, scale, scale, 1, CenterGravity);
		}

		if (_exposeModeStep >= 10)
		{
			unsubscribeFrom(_exposeModeTk);
			delete _exposeModeTk;
			_exposeModeTk = 0;

			if (_inExposeMode)
			{
				_ascr->forceDoublePointerEvent();
				_ascr->resetSelection(true);	
			}
			else
			{
				// restore desktop properties
				sgNode::restoreSavedTransformations();
				_scaleX = 1;
				_scaleY = 1;
				_transX = 0;
				_transY = 0;
				MetisseWindow::rootTransY = _transY;
				MetisseWindow::rootTransX = _transX;

				// restore window properties
				std::list<sgNode *>::iterator
					o = _dependencies.begin();
				for ( ; o != _dependencies.end(); ++o )
				{
					MetisseWindow *win = 
						dynamic_cast<MetisseWindow *>(*o);
						
					if (win == 0 ||
					    !_ascr->getMetisseDesktop()->
					    isWindowValide(win))
					{
						continue;
					}
					win->unforceCurrentTranslation();
					win->getRenderer()->
						restoreSavedTransformations();
				}
				if (!_exposeModeCanceled)
				{
					_ascr->forceDoublePointerEvent();
					_ascr->fixCursorPos(false);
				}
				else
				{
					_ascr->forceDoublePointerEvent();
					_ascr->resetSelection();
				}
			}
		}
		else
		{
			_exposeModeTk->arm(10);
		}
		_exposeModeStep++;
		_ascr->draw(false);
	}
	if (obs == _saveFrameTk)
	{
		_ascr->saveFrame();
		_ascr->draw(true);
		unsubscribeFrom(_saveFrameTk);
		delete _saveFrameTk;
		_saveFrameTk = 0;
	}
}

void LayerManager::saveFrame(unsigned int delay)
{
	if (_saveFrameTk != 0)
	{
		unsubscribeFrom(_saveFrameTk);
		delete _saveFrameTk;
	}
	_saveFrameTk = TimeKeeper::create();
	subscribeTo(_saveFrameTk);
	_saveFrameTk->arm(delay);
}

// --------------------------------------------------------------------
// "Operations"

void LayerManager::translate_rel(float x, float y, float z)
{
	float tx = _transX + x;
	float ty = _transY + y;

	if (_scaleX >= 1)
	{
		float dw = (float)_desktopWidth*_scaleX/2.0;
		float dh = (float)_desktopHeight*_scaleY/2.0;

		float adw = (float)_desktopWidth/2.0;
		float adh = (float)_desktopHeight/2.0;

		float left = -dw + tx*_scaleX;
		float top = dh + ty*_scaleY;
		float right = dw + tx*_scaleX;
		float bottom = -dh + ty*_scaleY;

		if (left > -adw)
		{
			x = (-adw + dw)/_scaleX - _transX;
		}
		if (right < adw)
		{
			x = (adw - dw)/_scaleX - _transX;
		}
		if (top < adh)
		{
			y = (adh - dh)/_scaleY - _transY;
		}
		if (bottom > -adh)
		{
			y = (-adh + dh)/_scaleY - _transY;
		}
	}
	else
	{
		int xcorr = 0,ycorr = 0;
		float sx = x;
		float sy = y;

		float dw = _desktopWidth*_scaleX/2.0;
		float dh = _desktopHeight*_scaleY/2.0;

		float adw = (float)_desktopWidth/2.0;
		float adh = (float)_desktopHeight/2.0;

		float left = - _pagerVx*_scaleX - dw + tx*_scaleX;
		float top =    _pagerVy*_scaleY + dh + ty*_scaleY;
		float right = (_pagerVWidth - _desktopWidth - _pagerVx)*_scaleX
			+ dw + tx*_scaleX;
		float bottom =
			(-_pagerVHeight + _desktopHeight + _pagerVy)*_scaleY
			- dh + ty*_scaleY;

		// margin stuff
		float maxMargeX = _desktopWidth * A_PAGER_MODE_MARGIN_P;
		float maxMargeY = _desktopHeight* A_PAGER_MODE_MARGIN_P;
		float maxScaleX =
			(_desktopWidth * (1.0 - A_PAGER_MODE_MARGIN_P))
				/ _pagerVWidth;
		float maxScaleY =
			(_desktopHeight * (1.0 - A_PAGER_MODE_MARGIN_P))
			/ _pagerVHeight;
		
		if (left > -adw)
		{
			x = (- adw - left)/_scaleX - _transX +
				maxMargeX/(2*_scaleX);
			xcorr++;
		}
		if (right < adw)
		{
			x = (adw - right)/_scaleX - _transX - 
				maxMargeX/(2*_scaleX);
			xcorr++;
		}
		if (top < adh)
		{
			y = (adh - top)/_scaleY - _transY -
				maxMargeY/(2*_scaleY);
			ycorr++;
		}
		if (bottom > -adh)
		{
			y = (-adh - bottom)/_scaleY - _transY +
				maxMargeY/(2*_scaleY);
			ycorr++;
		}
	}

	
	if (x == 0 && y == 0)
	{
		return;
	}

	_transX = _transX + x;
	_transY = _transY + y;
	for( std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o )
	{
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
		
		if (!win)
		{
			continue;
		}
		win->translate_rel(x, y, 0);
	}
	MetisseWindow::rootTransX = _transX;
	MetisseWindow::rootTransY = _transY;
}

void LayerManager::getTranslation(float *x, float *y)
{
	*x = _transX;
	*y = _transY;
}

void LayerManager::checkPaneAction(int x, int y, bool inInternalMove)
{
	float tx = 0;
	float ty = 0;

	if (_scaleX > 1)
	{
		if (x < 4) { tx = 10; }
		if (y < 4) { ty = -10; }
		if (x > _desktopWidth - 5)
		{ 
			tx = -10;
		}
		if (y > _desktopHeight - 5)
		{
			ty = 10;
		}
		if (tx != 0 || ty != 0)
		{
			translate_rel(tx,ty,0);
		}
	}
}

bool LayerManager::checkMoveWindowPaneAction(
	int x, int y, int *r_npx, int *r_npy, int *r_wx, int *r_wy,
	int *r_tx, int *r_ty)
{
	if (_scaleX != 1)
	{
		// FIXME
		return false;
	}

	char cmd[256];

	*r_npx = 0;
	*r_npy = 0;
	*r_wx = x;
	*r_wy = y;
	*r_tx = 0;
	*r_ty = 0;
	if (x == 0 && _pagerVx >= _desktopWidth)
	{
		*r_npx = -1;
		*r_wx = (int)(_desktopWidth - 1);
		*r_tx = (int)(-_desktopWidth);
	}
	if (x == _desktopWidth - 1 &&
	    _pagerVx <= _pagerVWidth - 2*_desktopWidth)
	{
		*r_npx = 1;
		*r_wx = 0;
		*r_tx = (int)(_desktopWidth);
	}
	if (y == 0 && _pagerVy >= _desktopHeight)
	{
		*r_npy = -1;
		*r_wy = (int)(_desktopHeight - 1);
		*r_ty = (int)(-_desktopHeight);
	}
	if (y == _desktopHeight - 1 &&
	    _pagerVy <= _pagerVHeight - 2*_desktopHeight)
	{
		*r_npy = 1;
		*r_wy = 0;
		*r_ty = (int)(_desktopHeight);		
	}
	if (*r_npx != 0 || *r_npy != 0)
	{
		return true;
	}

	return false;
	
}

void LayerManager::setSnapAttractDist(void)
{
	_snapAttractionDist = DEFAULT_SNAP_ATTRACTION_DIST;
}

void LayerManager::setSnapAttractDist(int dist)
{
	_snapAttractionDist = dist;
}

int LayerManager::getSnapAttractDist(void)
{
	return _snapAttractionDist;
}

void LayerManager::setSnapAttractMode(void)
{
	_snapAttracMode = DEFAULT_SNAP_ATTRAC_MODE;
}

void LayerManager::setSnapAttractMode(int mode)
{
	_snapAttracMode = mode;
}

int LayerManager::getSnapAttractMode(void)
{
	return _snapAttracMode;
}

/* This function does the SnapAttraction stuff. If takes x and y coordinates
 * (*px and *py) and returns the snapped values. */
bool LayerManager::doSnapAttract(
	MetisseWindow *win, int *px, int *py, bool *do_x, bool *do_y,
	bool *right, bool *top, MetisseWindow **win_x, MetisseWindow **win_y)
{
	int nyt,nxl,dist,closestLeft,closestRight,closestBottom,closestTop;
	rectangle self, other;
	MetisseWindow *tmp;
	rectangle g;
	bool rc = false;
	
	*win_x = 0;
	*win_y = 0;
	*do_x = *do_y = false;
	*right = *top = false;

	if (!win->isManaged() || win->isIconified())
	{
		return rc;
	}

	/* resist based on window edges */
	closestTop = _snapAttractionDist;
	closestBottom = _snapAttractionDist;
	closestRight = _snapAttractionDist;
	closestLeft = _snapAttractionDist;
	nxl = -99999;
	nyt = -99999;

	win->getSize(&self.width, &self.height);
	self.x = *px;
	self.y = *py;

	/*
	 * snap attraction
	 */
	/* snap to other windows */
	if (_snapAttractionDist > 0)
	{
		for(std::list<sgNode *>::iterator o=_dependencies.begin();
		    o != _dependencies.end(); o++)
		{
			tmp = dynamic_cast<MetisseWindow *>(*o);
			if (!tmp || !tmp->isManaged() ||
			    win->getDesk() != win->getDesk() ||
			    tmp->isIconified() || win == tmp)
			{
				continue;
			}
			tmp->getSize(&other.width, &other.height);
			tmp->getRealPosition(&other.x,&other.y);
			/* prevent that window snaps off screen */
			// FIXME: scaled desktop
			if (other.x <= 0)
			{
				other.x -= _snapAttractionDist + 10000;
				other.width += _snapAttractionDist + 10000;
			}
			if (other.y <= 0)
			{
				other.y -= _snapAttractionDist + 10000;
				other.height += _snapAttractionDist + 10000;
			}
			if (other.x + other.width >= (int)_desktopWidth)
			{
				other.width += _snapAttractionDist + 10000;
			}
			if (other .y + other.height >= (int)_desktopHeight)
			{
				other.height += _snapAttractionDist + 10000;
			}

			/* snap horizontally */
			if (!((other.y + (int)other.height) < (*py) ||
			      (other.y) > (*py + (int)self.height) ))
			{
				dist = abs(other.x - (*px + (int)self.width));
				if (dist < closestRight)
				{
					*right = true;
					closestRight = dist;
					if (*px + (int)self.width >= other.x &&
					    *px + (int)self.width <
					    other.x + _snapAttractionDist)
					{
						nxl = other.x - (int)self.width;
						*do_x = rc = true;
						*win_x = tmp;
					}
					if (*px + (int)self.width >=
					    other.x - _snapAttractionDist &&
					    *px + (int)self.width < other.x)
					{
						nxl = other.x - (int)self.width;
						*do_x = rc = true;
						*win_x = tmp;
					}
				}
				dist = abs(other.x + (int)other.width - *px);
				if (dist < closestLeft)
				{
					*right = false;
					closestLeft = dist;
					if (*px <= other.x + (int)other.width &&
					    *px > other.x + (int)other.width -
					    _snapAttractionDist)
					{
						nxl = other.x +
							(int)other.width;
						*do_x = rc = true;
						*win_x = tmp;
					}
					if (*px <= other.x + (int)other.width +
					    _snapAttractionDist &&
					    *px > other.x + (int)other.width)
					{
						nxl = other.x +
							(int)other.width;
						*do_x = rc = true;
						*win_x = tmp;
					}
				}
			} /* horizontally */
			/* snap vertically */
			if (!((other.x + (int)other.width) < (*px) ||
			      (other.x) > (*px + (int)self.width)))
			{
				dist = abs(other.y - (*py + (int)self.height));
				if (dist < closestBottom)
				{
					*top = false;
					closestBottom = dist;
					if (*py + (int)self.height >= other.y &&
					    *py + (int)self.height < other.y +
					    _snapAttractionDist)
					{
						nyt = other.y -
							(int)self.height;
						*do_y = rc = true;
						*win_y = tmp;
					}
					if (*py + (int)self.height >=
					    other.y - _snapAttractionDist &&
					    *py + (int)self.height < other.y)
					{
						nyt = other.y -
							(int)self.height;
						*do_y = rc = true;
						*win_y = tmp;
					}
				}
				dist = abs(other.y + (int)other.height - *py);
				if (dist < closestTop)
				{
					*top = true;
					closestTop = dist;
					if (*py <=
					    other.y + (int)other.height &&
					    *py > other.y + (int)other.height -
					    _snapAttractionDist)
					{
						nyt = other.y +
							(int)other.height;
						*do_y = rc = true;
						*win_y = tmp;
					}
					if (*py <= other.y + (int)other.height +
					    _snapAttractionDist &&
					    *py > other.y + (int)other.height)
					{
						nyt = other.y +
							(int)other.height;
						*do_y = rc = true;
						*win_y = tmp;
					}
				}
			} /* vertically */
		} /* for */
	} /* snap to other windows */

	/* snap to screen egdes */
	// FIXME: scaled desktop: snap for all the visible page
	if (_snapAttractionDist > 0)
	{
		/* horizontally */
		if (!((int)_desktopWidth < (*px) ||
		      (*px + (int)self.width) < 0))
		{
			dist = abs((int)_desktopHeight -
				   (*py + (int)self.height));
			if (dist < closestBottom)
			{
				*top = false;
				closestBottom = dist;
				if (*py + (int)self.height >=
				    (int)_desktopHeight &&
				    *py + (int)self.height <
				    (int)_desktopHeight + _snapAttractionDist)
				{
					nyt = (int)_desktopHeight -
						(int)self.height;
					*do_y = rc = true;
					*win_y = 0;
				}
				if (*py + (int)self.height >=
				    (int)_desktopHeight - _snapAttractionDist &&
				    *py + (int)self.height <
				    (int)_desktopHeight)
				{
					nyt = (int)_desktopHeight -
						(int)self.height;
					*do_y = rc = true;
					*win_y = 0;
				}
			}
			dist = abs(*py);
			if (dist < closestTop)
			{
				*top = true;
				closestTop = dist;
				if ((*py <= 0)&&(*py > - _snapAttractionDist))
				{
					nyt = 0;
					*do_y = rc = true;
					*win_y = 0;
				}
				if ((*py <=  _snapAttractionDist)&&(*py > 0))
				{
					nyt = 0;
					*do_y = rc = true;
					*win_y = 0;
				}
			}
		} /* horizontally */
		/* vertically */
		if (!((int)_desktopHeight < (*py) ||
		      (*py + (int)self.height) < 0))
		{
			dist = abs((int)_desktopWidth - (*px + (int)self.width));
			if (dist < closestRight)
			{
				*right = true;
				closestRight = dist;
				if (*px + (int)self.width >=
				    (int)_desktopWidth &&
				    *px + (int)self.width <
				    (int)_desktopWidth + _snapAttractionDist)
				{
					nxl = (int)_desktopWidth -
						(int)self.width;
					*do_x = rc = true;
					*win_x = 0;
				}

				if (*px + (int)self.width >=
				    (int)_desktopWidth - _snapAttractionDist &&
				    *px + (int)self.width < (int)_desktopWidth)
				{
					nxl = (int)_desktopWidth -
						(int)self.width;
					*do_x = rc = true;
					*win_x = 0;
				}
			}
			dist = abs(*px);
			if (dist < closestLeft)
			{
				*right = false;
				closestLeft = dist;
				if ((*px <= 0) &&
				    (*px > - _snapAttractionDist))
				{
					nxl = 0;
					*do_x = rc = true;
					*win_x = 0;
				}
				if ((*px <= _snapAttractionDist) &&
				    (*px > 0))
				{
					nxl = 0;
					*do_x = rc = true;
					*win_x = 0;
				}
			}
		} /* vertically */
	} /* snap to screen edges */

	if (nxl != -99999)
	{
		*px = nxl;
	}
	if (nyt != -99999)
	{
		*py = nyt;
	}

#if 0

	/*
	 * Resist moving windows beyond the edge of the screen
	 */
	if (Scr.MoveResistance > 0)
	{
		/* snap to right edge */
		if (*px + Width >= (int)_desktopWidth &&
		    *px + Width < (int)_desktopWidth + Scr.MoveResistance)
		{
			*px = (int)_desktopWidth - Width;
		}
		/* snap to left edge */
		else if ((*px <= 0) && (*px > -Scr.MoveResistance))
		{
			*px = 0;
		}
		/* snap to bottom edge */
		if (*py + Height >= (int)_desktopHeight &&
		    *py + Height < (int)_desktopHeight + Scr.MoveResistance)
		{
			*py = (int)_desktopHeight - Height;
		}
		/* snap to top edge */
		else if (*py <= 0 && *py > -Scr.MoveResistance)
		{
			*py = 0;
		}
	}
	/* Resist moving windows between xineramascreens */
	if (Scr.XiMoveResistance > 0 && FScreenIsEnabled())
	{
		int scr_x0, scr_y0, scr_x1, scr_y1;
		Bool do_recalc_rectangle = False;

		FScreenGetResistanceRect(
			*px, *py, Width, Height, &scr_x0, &scr_y0, &scr_x1,
			&scr_y1);

		/* snap to right edge */
		if (scr_x1 < (int)_desktopWidth &&
		    *px + Width >= scr_x1 && *px + Width <
		    scr_x1 + Scr.XiMoveResistance)
		{
			*px = scr_x1 - Width;
			do_recalc_rectangle = True;
		}
		/* snap to left edge */
		else if (scr_x0 > 0 &&
			 *px <= scr_x0 && scr_x0 - *px < Scr.XiMoveResistance)
		{
			*px = scr_x0;
			do_recalc_rectangle = True;
		}
		if (do_recalc_rectangle)
		{
			/* Snapping in X direction can move the window off a
			 * screen.  Thus, it may no longer be necessary to snap
			 * in Y direction. */
			FScreenGetResistanceRect(
				*px, *py, Width, Height, &scr_x0, &scr_y0,
				&scr_x1, &scr_y1);
		}
		/* snap to bottom edge */
		if (scr_y1 < (int)_desktopHeight &&
		    *py + Height >= scr_y1 && *py + Height <
		    scr_y1 + Scr.XiMoveResistance)
		{
			*py = scr_y1 - Height;
		}
		/* snap to top edge */
		else if (scr_y0 > 0 &&
			 *py <= scr_y0 && scr_y0 - *py < Scr.XiMoveResistance)
		{
			*py = scr_y0;
		}
	}
#endif

	return rc;
}

bool LayerManager::checkMove(
	MetisseWindow *win, int diff_x, int diff_y)
{
	if (_surface == SURFACE_SCALE)
	{
		double dw = _desktopWidth/2;
		double dh = _desktopHeight/2;
		float w,h, tx, ty;

		win->getSize(&w, &h);
		win->getCurrentTranslation(&tx, &ty);

		if (diff_x > 0 && tx - w/2 < -dw)
		{
			// left
			if (dw + tx > 30)
			{
				//*scale = (2/w)*(dw + *tx);
				// ok
			}
			else
			{
				//*scale = 24/w;
				// No
				return false;
			}
		}
		else if (diff_x < 0  && tx + w/2 > dw)
		{
			if (dw - tx > 30)
			{
				//*scale = (2/w)*(dw - *tx);
				// ok
			}
			else
			{
				//*scale = 24/w;
				// no
				return false;
			}
		}
	}

	return true;
}
  
void LayerManager::zoomedout_translate_rel(float x, float y, float z)
{
	if (_scaleX <= 1)
	{
		return;
	}
	translate_rel(x, y, z);
}

void LayerManager::x11project(int x, int y, int *rx, int *ry)
{
	GLdouble ox, oy, oz;

	GLuint selBuffer[1] ;
	GLuint selSize=1;
	selBuffer[0] = (this)->getId();

	unproject(
		  (int)x, (int)y, 0, _viewpoint, selBuffer,
		  selSize, &ox, &oy,&oz);
	
	*rx = (int)(_vScale*ox + _desktopWidth/2.0 - _transX);
	*ry = (int)(-_vScale*oy + _desktopHeight/2.0 + _transY);
}

// X11 geometries!
void LayerManager::x11projectWarp(int x, int y, int *px, int *py, bool warp)
{
	// project with the hands in 3D
	float adw = (float)_desktopWidth/2.0;
	float adh = (float)_desktopHeight/2.0;
	float gx = (((float)x - adw) + _transX)*_scaleX*_extScaleX;
	float gy = ((-(float)y + adh) + _transY)*_scaleY*_extScaleY;

	if (_extZRotate == 0 && warp)
	{
		// FIXME in Portrait!
		float tx = 0, ty = 0;
		if (gx < -adw)
		{
			tx = (-adw/_scaleX) + adw - _transX - x + 4;
		}
		else if (gx > adw)
		{
			tx = (adw/_scaleX) + adw - _transX - x - 4;
		}
		if (gy > adh)
		{
			ty = (adh/_scaleY) - adh + y - _transY - 4;
		}
		else if (gy < -adh)
		{
			ty = (-adh/_scaleY) - adh + y - _transY + 4;
		}

		if (tx != 0 || ty != 0)
		{
			translate_rel(tx, ty, 0);

			gx = (((float)x - adw) + _transX)*_scaleX;
			gy = ((-(float)y + adh) + _transY)*_scaleY;
		}
	}

	// X11 geometries
	if (_extZRotate)
	{
		*py = (int)(gx + adw);
		*px = (int)(_desktopHeight - (-gy + adh));
	}
	else
	{
		*px = (int)(gx + adw);
		*py = (int)(-gy + adh);
	}
}

bool LayerManager::getWinBoundingBox(
	MetisseWindow *win, GLdouble *ret_left, GLdouble *ret_top,
	GLdouble *ret_right, GLdouble *ret_bottom, bool x11)
{
	float x,y,w,h;
	win->getRealPosition(&x,&y);
	win->getSize(&w,&h);

	// FIXME should not be hard coded
	GLuint selBuffer[3] ;
	GLuint selSize=3;

	selBuffer[2] = (win->getRenderer())->getId();
	selBuffer[1] = (win)->getId();
	selBuffer[0] = (this)->getId();

	GLdouble left, top, right, bottom, px, py, pz = 0;
	bool ret;
	ret =_viewpoint->project(
		-w/2, h/2,  0.0, selBuffer, selSize, &left, &top, &pz);
	ret =_viewpoint->project(
		-w/2, -h/2,  0.0, selBuffer, selSize, &px, &bottom, &pz);

	if (px < left)
		left = px;

	ret =_viewpoint->project(
		w/2, -h/2,  0.0, selBuffer, selSize, &right, &py, &pz);
	if (py < bottom)
		bottom = py;
	ret =_viewpoint->project(
		w/2, h/2,  0.0, selBuffer, selSize, &px, &py, &pz);
	if (py > top)
		top = py;
	if (px > right)
		right = px;

	// upside down? ... FIXME?
	if (top < bottom)
	{
		py = top;
		top = bottom;
		bottom = py;
	}
	if (right < left)
	{
		px = right;
		right = left;
		left = px;
	}

#if 0
	std::cerr << "Translate: " << left << " " << top << " " << right << " "
		  << bottom <<  std::endl;
#endif

	if (x11)
	{
		*ret_left = left;
		*ret_top = - top + _desktopHeight;
		*ret_right = right;
		*ret_bottom = - bottom + _desktopHeight;	
	}
	else
	{
		*ret_left = left;
		*ret_top = top;
		*ret_right = right;
		*ret_bottom = bottom;
	}

	return ret;
}

// FIXME: not perfect if the window has some XY rotation
void LayerManager::translateForWindow(MetisseWindow *win)
{
	bool found = false;

	if (_scaleX <= 1 && _scaleY <= 1)
	{
		return;
	}

	for( std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o )
	{
		MetisseWindow *w = dynamic_cast<MetisseWindow *>(*o);
		if (w == win) found = true; 
	}

	if (!found)
		return;

	GLdouble left, top, right, bottom;
	getWinBoundingBox(win, &left, &top, &right, &bottom);

	float tsX = _scaleX*_extScaleX;
	float tsY = _scaleY*_extScaleY;

	float tx = 0, ty = 0;
	if (left < 0)
	{
		tx = - left / tsX;
	}
	else if (right > _desktopWidth)
	{
		tx = (_desktopWidth - right)/tsX;
		left = left + tx*tsX;
		if (left < 0)
		{
			tx += - left / tsX;
		}
	}
	if (top > _desktopHeight)
	{
		ty = (_desktopHeight - top) / tsY;
	}
	else if (bottom < 0)
	{
		ty = - bottom / tsY;
		top = top + ty*tsY;
		if (top > _desktopHeight)
		{
			ty += (_desktopHeight - top)/ tsY;
		}
	}

	translate_rel(tx, ty, 0);
}

// simplistic
unsigned int LayerManager::_guessGravity(MetisseWindow *win)
{
	unsigned int gravity = NorthWestGravity; // default
	unsigned int NSGravity = NorthGravity; // default
	unsigned int WEGravity = WestGravity; // default

	float x,y,width,height;
	win->getRealPosition(&x,&y);
	win->getSize(&width,&height);
	
	float factorW = _desktopWidth/10;
	float leftLimit = _desktopWidth/4;
	float leftStrongLimit = factorW;
	float rightLimit = _desktopWidth - leftLimit;
	float rightStrongLimit = _desktopWidth - leftStrongLimit;

	float factorH = _desktopHeight/10;
	float topLimit = _desktopHeight/4;
	float topStrongLimit = factorH;
	float bottomLimit = _desktopHeight - topLimit;
	float bottomStrongLimit = _desktopHeight - topStrongLimit;

	if (x >= rightLimit)
	{
		WEGravity = EastGravity;
	}
	else if (x >= leftLimit)
	{
		if (x + width >= rightStrongLimit)
			WEGravity = EastGravity;
		else
			WEGravity = CenterGravity;
	}
	else if (x > leftStrongLimit)
	{
		if (x + width > rightStrongLimit)
			WEGravity = EastGravity;
		else if (x + width > leftLimit)
			WEGravity = CenterGravity;
	}
	//  x <= leftStrongLimit?
	else if (x + width > rightStrongLimit && x > leftStrongLimit)
	{
		if (_desktopWidth - (x + width) < x)
		    WEGravity = EastGravity;
		else if (_desktopWidth - (x + width) == x)
		    WEGravity = CenterGravity;
	}

	if (y >= bottomLimit)
	{
		NSGravity = SouthGravity;
	}
	else if (y >= topLimit)
	{
		if (y + height >= bottomStrongLimit)
			NSGravity = SouthGravity;
		else
			NSGravity = CenterGravity;
	}
	else if (y > topStrongLimit)
	{
		if (y + height > bottomStrongLimit)
			NSGravity = SouthGravity;
		else if (y + height > bottomLimit)
			NSGravity = CenterGravity;
	}
	// y <= topStrongLimit?
	else if (y + height >= bottomStrongLimit && y > topStrongLimit)
	{
	  //fprintf(stderr, "STRANGE %i %i\n", y, y+height);
		if (_desktopHeight - (y + height) < y)
		    NSGravity = SouthGravity;
		else if (_desktopHeight - (x + width) == x)
		    NSGravity = CenterGravity;
	}

	switch(NSGravity)
	{
	case NorthGravity:
	{
		switch(WEGravity)
		{
		case EastGravity:
			gravity = NorthEastGravity;
			break;
		case CenterGravity:
			gravity = NorthGravity;
			break;
		case WestGravity:
			gravity = NorthWestGravity;
			break;
		default:
			break;
		}
	}
	break;
	case CenterGravity:
	{
		switch(WEGravity)
		{
		case EastGravity:
			gravity = EastGravity;
			break;
		case CenterGravity:
			gravity = CenterGravity;
			break;
		case WestGravity:
			gravity = WestGravity;
			break;
		default:
			break;
		}
	}
	break;
	case SouthGravity:
	{
		switch(WEGravity)
		{
		case EastGravity:
			gravity = SouthEastGravity;
			break;
		case CenterGravity:
			gravity = SouthGravity;
			break;
		case WestGravity:
			gravity = SouthWestGravity;
			break;
		default:
			break;
		}
	}
	break;
	default:
		break;
	}

	return gravity;
}

void LayerManager::scaleWindow(
	MetisseWindow *win, GLfloat fx, GLfloat fy, GLfloat fz,
	unsigned int gravity, bool absolute, bool animate, bool fixCursor)
{
	float x,y,w,h;
	win->getRealPosition(&x,&y);
	win->getSize(&w,&h);

	// FIXME should not be hard coded
	GLuint selBuffer[3] ;
	GLuint selSize=3;

	selBuffer[2] = (win->getRenderer())->getId();
	selBuffer[1] = (win)->getId();
	selBuffer[0] = (this)->getId();

	float tsX = _scaleX;
	float tsY = _scaleY;

	if (gravity == 0)
	{
		gravity = _guessGravity(win);
	}

	GLdouble left, top, gx, gy, qx, qy, rx, ry, px, py, pz = 0;
	bool ret;
	switch (gravity)
	{
	case SouthEastGravity:
		gx = w/2; gy = -h/2;
		break;
	case SouthGravity:
		gx = 0; gy = -h/2;
		break;
	case SouthWestGravity:
		gx = -w/2; gy = -h/2;
		break;
	case EastGravity:
		gx = w/2; gy = 0;
		break;
	case CenterGravity:
		gx = 0; gy = 0;
		break;
	case WestGravity:
		gx = -w/2; gy = 0;
		break;
	case NorthEastGravity:
		gx = w/2; gy = h/2;
		break;
	case NorthGravity:
		gx = 0; gy = h/2;
		break;
	case NorthWestGravity:
	default:
		gx = -w/2; gy = h/2;
		break;
	}

	ret =_viewpoint->project(
		-w/2, h/2,  0.0, selBuffer, selSize, &left, &top, &pz);

	if (gravity == NorthWestGravity)
	{
		qx = left;
		qy = top;
	}
	else
	{
		ret =_viewpoint->project(
			gx, gy,  0.0, selBuffer, selSize, &qx, &qy, &pz);
	}

	GLfloat curTrans[18];
	GLfloat scaleTrans[18];
	if (animate)
	{
		win->getRendererFullTransformation(curTrans);
	}
	if (fixCursor)
	{
		_ascr->fixCursorPos(true, win);
	}

	if (absolute)
	{
		win->absoluteScale(fx, fy, fz);
	}
	else
	{
		win->getRenderer()->scale(fx, fy, fz);
	}

	ret =_viewpoint->project(
		-w/2, h/2,  0.0, selBuffer, selSize, &px, &py, &pz);
	if (gravity == NorthWestGravity)
	{
		rx = px;
		ry = py;
	}
	else
	{
		ret =_viewpoint->project(
			gx, gy,  0.0, selBuffer, selSize, &rx, &ry, &pz);
	}

	if (_extZRotate == 0)
	{
		win->internalTranslation(
			(-rx + qx)/(_scaleX*_extScaleX),
			(- ry + qy)/(_scaleY*_extScaleY), 0);
	}
	else
	{
		win->internalTranslation(
			- (- ry + qy)/(_scaleY*_extScaleY),
			(-rx + qx)/(_scaleX*_extScaleX), 0);
	}

	if (animate)
	{
		win->getRendererFullTransformation(scaleTrans);
		win->setRendererFullTransformation(curTrans);
		win->getRenderer()->transformBack(scaleTrans);
	}
	if (fixCursor)
	{
		_ascr->fixCursorPos(false);
	}
	_ascr->sendMetisseTransform(win, false);
	// FIXME: keep on screen (top,left)?
}

void LayerManager::rotateWindow(
	MetisseWindow *win, GLfloat angle, GLfloat fx, GLfloat fy, GLfloat fz,
	unsigned int gravity, bool absolute)
{
	float x,y,w,h;
	win->getRealPosition(&x,&y);
	win->getSize(&w,&h);

	// FIXME should not be hard coded
	GLuint selBuffer[3] ;
	GLuint selSize=3;

	selBuffer[2] = (win->getRenderer())->getId();
	selBuffer[1] = (win)->getId();
	selBuffer[0] = (this)->getId();

	float tsX = _scaleX;
	float tsY = _scaleY;

	if (gravity == 0)
	{
		gravity = _guessGravity(win);
	}

	GLdouble left, top, gx, gy, qx, qy, rx, ry, px, py, pz = 0;
	bool ret;
	switch (gravity)
	{
	case SouthEastGravity:
		gx = w/2; gy = -h/2;
		break;
	case SouthGravity:
		gx = 0; gy = -h/2;
		break;
	case SouthWestGravity:
		gx = -w/2; gy = -h/2;
		break;
	case EastGravity:
		gx = w/2; gy = 0;
		break;
	case CenterGravity:
		gx = 0; gy = 0;
		break;
	case WestGravity:
		gx = -w/2; gy = 0;
		break;
	case NorthEastGravity:
		gx = w/2; gy = h/2;
		break;
	case NorthGravity:
		gx = 0; gy = h/2;
		break;
	case NorthWestGravity:
	default:
		gx = -(w/2); gy = (h/2);
		//gx = - (_desktopWidth - w)/2 + x;
		//gy = (_desktopHeight - h)/2 - y;
		break;
	}
	GLfloat curTrans[18];
	float itx,ity;
	win->getRendererFullTransformation(curTrans);
	win->getInternalTranslation(&itx, &ity);
	win->getRenderer()->reset();
	if (gravity == CenterGravity)
	{
		// FIXME: ...
		win->internalTranslation(itx, ity, 0);
	}
#if 0
	ret =_viewpoint->project(
		-w/2, h/2,  0.0, selBuffer, selSize, &left, &top, &pz);

	if (gravity == NorthWestGravity)
	{
		qx = left;
		qy = top;
	}
	else
#endif
	{
		ret =_viewpoint->project(
			gx, gy,  0.0, selBuffer, selSize, &qx, &qy, &pz);
	}

	//fprintf(stderr, "project 0: %f %f %f\n", qx, qy, pz);
	if (gravity == CenterGravity)
	{
		win->internalTranslation(-itx, -ity, 0);
	}
	win->setRendererFullTransformation(curTrans);
	if (absolute)
	{
		win->absoluteRotate(angle, fx, fy, fz);
	}
	else
	{
		win->getRenderer()->rotate_rel(angle, fx, fy, fz);
	}

#if 0
	ret =_viewpoint->project(
		-w/2, h/2,  0.0, selBuffer, selSize, &px, &py, &pz);
	if (gravity == NorthWestGravity)
	{
		rx = px;
		ry = py;
	}
	else
#endif
	{
		ret =_viewpoint->project(
			gx, gy,  0.0, selBuffer, selSize, &rx, &ry, &pz);
	}

	//fprintf(stderr, "project 1: %f %f %f\n", rx, ry, pz);	
	if (_extZRotate == 0)
	{
		win->internalTranslation(
			(-rx + qx)/(_scaleX*_extScaleX),
			(- ry + qy)/(_scaleY*_extScaleY), 0);
	}
	else
	{
		win->internalTranslation(
			- (- ry + qy)/(_scaleY*_extScaleY),
			(-rx + qx)/(_scaleX*_extScaleX), 0);
	}

	_ascr->sendMetisseTransform(win, false);
	// FIXME: keep on screen (top,left)?
}

void LayerManager::attachWindow(MetisseWindow *main, MetisseWindow *sub)
{
	float mx,my,mw,mh;
	float sx,sy,sw,sh;
	float ox,oy,ow,oh;
	float tx,ty;
	GLdouble mleft, mtop, sleft, stop, pz = 0;
	bool ret;
	MetisseWindow *orig;

	main->getRealPosition(&mx,&my);
	main->getSize(&mw,&mh);
	sub->getRealPosition(&sx,&sy);
	sub->getSize(&sw,&sh);

	tx = ty = 0;
	orig = dynamic_cast<MetisseWindow *>(sub->getTransientForOrig());
	if (orig)
	{
		orig->getRealPosition(&ox,&oy);
		orig->getSize(&ow,&oh);
		cut *c = sub->getCutOrig();
		float sx = 0, sy = 0, sw = 0, sh = 0, dx = 0, dy = 0;
		if (main->hasAFacade() && c != 0)
		{
			int left = 0,top = 0, right = 0, bottom = 0;
			if (main->isAFacadeWindow())
			{
				main->getDecorStrut(
					&left, &top, &right, &bottom);
			}
			sx = c->sx; sy = c->sy;
			sw = c->sw; sh = c->sh;
			dx = c->dx + left; dy = c->dy + top;
			#if 0
			std::cerr << "LM Orig Cut: " 
				  << left << " " << top << " " << sx <<  " "
				  << sy << " " << sw <<  " " << sh << " " << dx
				  <<  " " << dy << std::endl;
			#endif
		}
		// should check visibility
		tx = ox - dx - mx + sx;
		ty = oy - dy - my + sy;
	}

	// FIXME should not be hard coded
	GLuint selBuffer[3] ;
	GLuint selSize=3;

	selBuffer[2] = (main->getRenderer())->getId();
	selBuffer[1] = (main)->getId();
	selBuffer[0] = (this)->getId();

	ret =_viewpoint->project(
		-mw/2 + (sx-mx) -tx, mh/2 + (my-sy) +ty,  0.0,
		selBuffer, selSize, &mleft, &mtop, &pz);

	selBuffer[2] = (sub->getRenderer())->getId();
	selBuffer[1] = (sub)->getId();
	selBuffer[0] = (this)->getId();

	ret =_viewpoint->project(
		-sw/2, sh/2,  0.0, selBuffer, selSize, &sleft, &stop, &pz);

#if 0
	std::cerr << "A: " << mleft << " " << mtop << " "
		  << sleft << " " << mtop << std::endl;
#endif

	if (_extZRotate == 0)
	{
		// -tx , +ty
		sub->internalTranslation(
			(-sleft + mleft)/(_scaleX*_extScaleX),
			(- stop + mtop)/(_scaleY*_extScaleY), 0);
	}
	else
	{
		sub->internalTranslation(
			- (- stop + mtop)/(_scaleY*_extScaleY),
			(-sleft + mleft)/(_scaleX*_extScaleX), 0);
	}

	_ascr->sendMetisseTransform(sub, false); 
}

void LayerManager::scale(float x, float y, float z)
{
	float sx = _scaleX*x;
	float sy = _scaleY*y;
	bool restTrans = false;

	if (sx < 1)
	{
		if (_scaleX > 1)
		{
			restTrans = true;
		}
		else
		{
		}
	}
	else if (sx > 1)
	{
		if (_scaleX < 1)
		{
			restTrans = true;
		}
	}
	if (sy < 1)
	{
		if (_scaleY > 1)
		{
			restTrans = true;
		}
		else
		{
		}
	}
	else if (sy > 1)
	{
		if (_scaleY < 1)
		{
			restTrans = true;
		}
	}

	if (_scaleX != _scaleX*x && _scaleX*x == 1)
	{
		restTrans = true;
	}

	if (restTrans)
	{
		resetTransformations();
		return;
	}

	_scaleX = _scaleX*x;
	_scaleY = _scaleY*y;

	
	if (_scaleX >= 1)
	{
		sgNode::scale(x, y, 1);
		translate_rel(0,0,0);
	}
	else
	{
		if (_pagerVWidth*_scaleX <= _desktopWidth)
		{
			_scaleX =
				(_desktopWidth * (1.0 - A_PAGER_MODE_MARGIN_P))
				/ _pagerVWidth;
		}
		if (_pagerVHeight*_scaleY <= _desktopHeight)
		{
			_scaleY =
				(_desktopHeight * (1.0 - A_PAGER_MODE_MARGIN_P))
				/ _pagerVHeight;
		}

		// normalize
		if (_pagerVxPages > _pagerVyPages)
		{
			_scaleX = _scaleY*_pagerVyPages/_pagerVxPages;
		}
		else if (_pagerVxPages < _pagerVyPages)
		{
			_scaleY = _scaleX*_pagerVxPages/_pagerVyPages;
		}
		else
		{
			_scaleY = _scaleX;
		}

		float asx,asy;
		asx = _scaleX;
		asy = _scaleY;
		resetTransformations();
		_scaleX = asx;
		_scaleY = asy;
		//translate_rel(0,0,0);
		sgNode::scale(asx, asy, 1);
		translate_rel(0,0,0);
	}
}

void LayerManager::getScale(float *x, float *y)
{
	*x = _scaleX*_extScaleX;
	*y = _scaleY*_extScaleY;
}

void LayerManager::resetTransformations(void)
{
	sgNode::resetTransformations();
	sgNode::scale(_extScaleX, _extScaleY, 1);
	sgNode::rotate(_extZRotate,0,0,-_extZRotate);

	for( std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o )
	{
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
		
		if (!win)
		{
			continue;
		}
		win->translate_rel(-_transX, -_transY, 0);
	}
	_transX = 0;
	_transY = 0;
	_scaleX = 1;
	_scaleY = 1;
	MetisseWindow::rootTransY = 0;
	MetisseWindow::rootTransX = 0;
}

// --------------------------------------------------------------------
// Pager

bool LayerManager::inMovePage(void)
{
	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	    o != _dependencies.end(); o++)
	{
		MetisseWindow *win =
			dynamic_cast<MetisseWindow *>(*o);
		if (!win || win->isUnmanaged() || win->isRootWindow() ||
		    win->isSticky())
		{
			continue;
		}
		if (win->inMovePage())
		{
			return true;
		}
	}
	return false;
}


void LayerManager::_setInMovePage(void)
{
	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	    o != _dependencies.end(); o++)
	{
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
		if (!win || win->isUnmanaged() || win->isRootWindow() ||
		    win->isSticky())
		{
			continue;
		}
		win->setInMovePage(true);
	}
}

void LayerManager::setVirtualDesktopPara(
	int pagerVx, int pagerVy, long currentDesk, int pagerVxPages,
	int pagerVyPages)
{
	bool rescale = false;
	bool changed = false;
	float sx = 1,sy = 1;

	if ((int)_pagerVxPages != (int)pagerVxPages ||
	    (int)_pagerVyPages != (int)pagerVyPages)
	{
		rescale = true;
	}
	if (rescale || _pagerCurrentDesk != currentDesk ||
	    (int)_pagerVx != (int)pagerVx ||
	    (int)_pagerVy != (int)pagerVy)
	{
		changed = true;
	}

	if ((int)_pagerVx != (int)pagerVx || (int)_pagerVy != (int)pagerVy)
	{
		_setInMovePage();
	}
	_pagerVx = (float)pagerVx;
	_pagerVy = (float)pagerVy;
	_pagerCurrentDesk = currentDesk;
	_pagerVxPages = (float)pagerVxPages;
	_pagerVyPages = (float)pagerVyPages;
	_pagerVWidth =  (float)pagerVxPages * _desktopWidth;
	_pagerVHeight = (float)pagerVyPages * _desktopHeight;

	if (_scaleX < 1 && changed)
	{
		scale(1, 1, 1);
		_ascr->scheduleResetSelection();
	}
	else if (changed)
	{
		//_ascr->scheduleResetSelection();
	}
	if (_schedulePagerModeOff)
	{
		_schedulePagerModeOff = false;
		_ascr->schedulePagerModeOff();
	}
}

bool LayerManager::getPointerEvent(
	int x, int y, unsigned char button_mask, int *rx, int *ry, char *cmd)
{
	GLdouble ox, oy, oz;

	float z = 0;

	GLuint selBuffer[1] ;
	GLuint selSize=1;
	selBuffer[0] = (this)->getId();

	unproject(
		  (int)x, (int)y, 0, _viewpoint, selBuffer,
		  selSize, &ox, &oy,&oz);
	
	*rx = (int)(_vScale*ox + _desktopWidth/2.0 - _transX);
	*ry = (int)(-_vScale*oy + _desktopHeight/2.0 + _transY);
	
	// compute the relative page
	int npx = 0;
	int npy = 0;

	if ((int)button_mask != 0)
	{
		npx =  (int)((float)*rx/_desktopWidth) + ((*rx<0)? -1:0);
		npy =  (int)((float)*ry/_desktopHeight) + ((*ry<0)? -1:0);
		#if 0
		std::cerr << x << " " << y << " " << ox << " " << oy << " " 
			  << (5.0/2.0)*ox << " " << (5.0/2.0)*oy << " " 
			  << *rx << " " << *ry
			  << " Page: " << npx << " " << npy << std::endl;
		#endif
	}
	if (npx != 0 || npy != 0)
	{
		if ((button_mask & (1<<0)) || (button_mask & (1<<1)) )
		{
			sprintf(cmd,"GotoPage Animate %ip %ip", npx, npy);
			return false; // swallow release
		}
		else if ((button_mask & (1<<(3-1))) ||
			 (button_mask & (1<<(4-1))) )
		{
			_schedulePagerModeOff = true;
			sprintf(cmd,"GotoPage %ip %ip", npx, npy);
			pagerMode(false); 
			return false;  // swallow release
		}
	}
	else if (button_mask & (1<<(8-1)))
	{
		// FIXME: we are here only if we are in the void!
		pagerMode(false); 
		return false;  // swallow release
	}
	return true;
}

void LayerManager::getPagerModeScale(float *x, float *y)
{
	*x = _scaleX;
	*y = _scaleY;
}

void LayerManager::pagerMode(bool on)
{

	// FIXME: Expose !
	if (inExposeMode() || _exposeModeTk != 0)
	{
		return;
	}

	if (_pagerModeTk != 0)
	{
		return;
	}

	if (_inPagerMode && on)
	{
		return;
	}
	else if (!_inPagerMode && !on)
	{
		return;
	}

	if (on)
	{
		 _pagerModeSX = (_desktopWidth * (1.0 - A_PAGER_MODE_MARGIN_P)) /
			_pagerVWidth;
		 _pagerModeSY = (_desktopHeight *(1.0 - A_PAGER_MODE_MARGIN_P)) /
			_pagerVHeight;
	}
	else
	{
		 _pagerModeSX =  _pagerModeSY = 1;
	}

	_pagerModeStepX = (_scaleX - _pagerModeSX) / PAGER_MODE_ANIME_STEP;
	_pagerModeStepY = (_scaleY - _pagerModeSY) / PAGER_MODE_ANIME_STEP;

	_pagerModePrevSX = _scaleX;
	_pagerModePrevSY = _scaleY;

	_pagerModeStep = 0;

	if (_pagerModeTk != 0)
	{
		unsubscribeFrom(_pagerModeTk);
		delete _pagerModeTk;
	}
	_pagerModeTk = TimeKeeper::create();
	subscribeTo(_pagerModeTk);
	_pagerModeTk->arm(1);

	_ascr->fixCursorPos(true);
	if (on)
	{
		_inPagerMode = true;
	}
	else
	{
		_inPagerMode = false;
	}

}

void LayerManager::togglePagerMode(void)
{
	if (_inPagerMode)
	{
		pagerMode(false);
	}
	else
	{
		pagerMode(true);	
	}
}

// --------------------------------------------------------------------
// for click motion

bool LayerManager::_overlap(
	MetisseWindow *win1,
	GLdouble bbleft, GLdouble bbtop, GLdouble bbright, GLdouble bbbottom,
	MetisseWindow *win2,
	GLdouble *rleft, GLdouble *rtop, GLdouble *rright, GLdouble *rbottom,
	GLdouble *rx1, GLdouble *ry1, GLdouble *rx2, GLdouble *ry2,
	GLdouble *rx3, GLdouble *ry3, GLdouble *rx4, GLdouble *ry4,
	float *oltop, float *olright, float *olbottom, float *olleft)
{
	GLdouble left, top, right, bottom;
	getWinBoundingBox(win2, &left, &top, &right, &bottom, true);
	*rleft = left;
	*rtop = top;
	*rright = right;
	*rbottom = bottom;

	if (bbright <= left)
	{
		return false;
	}
	if (right <= bbleft)
	{
		return false;
	}
	if (bbbottom <= top)
	{
		return false;
	}
	if (bottom <= bbtop)
	{
		return false;
	}

	*rx1 = fmax(bbleft, left);
	*ry1 = fmax(bbtop, top);
	*rx3 = fmin(bbright, right);
	*ry3 = fmin(bbbottom, bottom);

	*rx2 = *rx3;
	*ry2 = *ry1;
	*rx4 = *rx1;
	*ry4 = *ry3;

	*oltop = *olright = *olbottom = *olleft = 0;
	if ((int)*ry1 == (int)top && right - left > 0)
	{
		// intersection intersect the top border of win2
		*oltop = (*rx2 - *rx1)/(right -left);
	}
	if ((int)*rx2 == (int)(right) && bottom - top > 0)
	{
		// intersection intersect the right border of win2
		*olright = (*ry3 - *ry2)/(bottom - top);
	}
	if ((int)*ry3 == (int)(bottom) && right - left > 0)
	{
		// intersection intersect the bottom border of win2
		*olbottom = (*rx3 - *rx4)/(right -left);
	} 
	if ((int)*rx1 == (int)(left) && bottom - top > 0)
	{
		// intersection intersect the left border of win2
		*olleft = (*ry4 - *ry1)/(bottom - top);
	}
	return true;
	
}

void LayerManager::foldOverWindows(
	MetisseWindow *startWindow, bool set, float alpha, bool anim)
{
	bool start = false; 
	GLdouble rx1, ry1, rx2, ry2;
	GLdouble rx3, ry3, rx4, ry4;
	float oltop, olright, olbottom, olleft;

	float swx, swy, swh, sww;
	startWindow->getRealPosition(&swx,&swy);
	startWindow->getSize(&sww,&swh);

	
	GLdouble swleft = 0, swtop = 0, swright = 0, swbottom = 0;
	if (set)
	{
		getWinBoundingBox(
			startWindow, &swleft, &swtop, &swright, &swbottom, true);
	}
	//std::cerr << (int)swleft << " " << (int)swright << " " <<
	//		(int)swbottom << " "  << (int)swtop << "\n";
	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o)
	{
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (!win || win->isUnmanaged() || !win->getRenderer())
		{
			continue;
		}
		if (win == startWindow)
		{
			start = true;
			continue;
		}
		if (!start)
		{
			continue;
		}
		if (!set)
		{
			win->getRenderer()->fold(0,0, false);
			continue;
		}
		if (!anim)
		{
			win->getRenderer()->foldOff();
		}
		GLdouble tleft, ttop, tright, tbottom;
		if (!_overlap(
			    startWindow, swleft, swtop, swright, swbottom,
			    win, &tleft, &ttop, &tright, &tbottom,
			    &rx1, &ry1, &rx2, &ry2, &rx3, &ry3, &rx4, &ry4,
			    &oltop, &olright, &olbottom, &olleft))
		{
			continue;
		}
		float ox, oy, oh, ow, tor,cotor;
		win->getRealPosition(&ox,&oy);
		win->getSize(&ow,&oh);
		int fx, fy; // want to fold here in x11 coordinate

		// hum ...
		ox = tleft;
		oy = ttop;
		oh = tbottom - ttop;
		ow = tright - tleft;
		swx = swleft;
		swy = swtop;
		swh = swbottom - swtop;
		sww = swright - swleft;
		//std::cerr << olleft << " " << olright << " " << olbottom << " "
		//	  << oltop << "\n";
		
		tor = 40;
		if (olleft >= olright && olleft >= olbottom && olleft >= oltop)
		{
			if (olleft >= 1 && olright >= 1 && oltop >=1 &&
			    olbottom >=1)
			{
				// startWin is included in win, fold on the
				// right 
				cotor = (tor*oh)/ow;
				fx = (int)(
					swx + sww + ow + fabs(ox+ow-swx-sww)/2
					+ cotor);
				fy = (int)(oy + tor);

			}
			else
			{
				double w = (rx3-rx1);
				cotor = 2*(w*tor)/oh;
				oy = ry4;
				fx = (int)(rx1 + 2*w);
				fy = (int)(oy + cotor);
			}
		}
		else if (olright >= olleft && olright >= olbottom &&
			 olright >= oltop)
		{
			double w = fabs(rx2-rx1);
			cotor = 2*(w*tor)/oh;
			ox = ox + ow;
			oy = ry4;
			fx = (int)(rx2 - 2*w);
			fy = (int)(oy + cotor); //(int)(ry1);
		}
		else if (olbottom >= oltop && olbottom >= olright &&
			 olbottom >= olleft)
		{
			oy = oy+oh;
			double h = fabs(ry4-ry1);
			cotor = 2*(h*tor)/ow;
			ox = rx2;
			fx = (int)(ox + cotor);
			fy = (int)(ry4 - 2*h);
		}
		else if (oltop >= olbottom && oltop >= olright &&
			 oltop >= olleft)
		{
			double h = fabs(ry3-ry1);
			cotor = 2*(h*tor)/ow;
			ox = rx2;
			fx = (int)(ox + cotor);
			fy = (int)(ry1 + 2*h);
		}
		else
		{
			// impossible
			std::cerr << "foldOverWindows: ERROR!\n"; 
		}
		_ascr->foldNonInteractive(win,(int)ox,(int)(oy),alpha, anim);
		_ascr->foldNonInteractive(win, fx,fy,alpha, anim);
	}
}

void LayerManager::scaleOverWindows(MetisseWindow *startWindow, bool set)
{
	bool start = false; 
	GLdouble rx1, ry1, rx2, ry2;
	GLdouble rx3, ry3, rx4, ry4;
	float oltop, olright, olbottom, olleft;
	bool over;
	int gravity;
	float scale_x, scale_y;

	GLdouble swleft = 0, swtop = 0, swright = 0, swbottom = 0;
	if (set)
	{
		getWinBoundingBox(
			startWindow, &swleft, &swtop, &swright, &swbottom);
	}

	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o)
	{
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (!win || win->isUnmanaged() || !win->getRenderer())
		{
			continue;
		}
		if (win == startWindow)
		{
			start = true;
			continue;
		}
		if (!start)
		{
			continue;
		}
		GLdouble tleft, ttop, tright, tbottom;
		over = _overlap(
			startWindow,  swleft, swtop, swright, swbottom,
			win, &tleft, &ttop, &tright, &tbottom,
			&rx1, &ry1, &rx2, &ry2, &rx3, &ry3, &rx4, &ry4,
			&oltop, &olright, &olbottom, &olleft);
		if (!over)
		{
			continue;
		}
		float sx, sy, sh, sw;
		win->getRealPosition(&sx,&sy);
		win->getSize(&sw,&sh);
		if (olleft >= olright && olleft >= olbottom && olleft >= oltop)
		{
			gravity =  EastGravity;
			scale_x = (sw-(rx2-rx1))/sw;
			scale_y = 1;
		}
		else if (olright >= olleft && olright >= olbottom &&
			 olright >= oltop)
		{
			gravity =  WestGravity;
			scale_x = (sw-(rx2-rx1))/sw;
			scale_y = 1;
		}
		else if (olbottom >= oltop && olbottom >= olright &&
			 olbottom >= olleft)
		{
			gravity =  NorthGravity;
			scale_x =  1;
			scale_y = (sh-(ry4-ry1))/sh;
		}
		else if (oltop >= olbottom && oltop >= olright &&
			 oltop >= olleft)
		{
			gravity =  SouthGravity;
			scale_x =  1;
			scale_y = (sh-(ry4-ry1))/sh;
		}
		else
		{
			// impossible
			std::cerr << "scaleOverWindows: ERROR!\n"; 
		}
		if (scale_x <= 0)
		{
			scale_x = 0.01;
		}
		if (scale_y <= 0)
		{
			scale_y = 0.01;
		}
		if (!set)
		{
			scale_x = 1/scale_x;
			scale_y = 1/scale_y;
		}
		//
		scaleWindow(
			win, scale_x, scale_y, 1, gravity, true, true);
	}
}

// not good
void LayerManager::setTmpAlphaAbove(
	MetisseWindow *startWindow, bool set, float alpha)
{
	bool start = false; 

	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o)
	{
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (!win || win->isUnmanaged())
		{
			continue;
		}
		if (win == startWindow)
		{
			start = true;
			continue;
		}
		if (!start)
		{
			continue;
		}
		if (!set)
		{
			win->resetAlpha();
		}
		else
		{
			win->setTmpAlpha(alpha);
		}
		
	}
}

// --------------------------------------------------------------------
// Surface  FIXME: obsolete code

void LayerManager::setSurfaceParameters(
	float surfaceLeft, float surfaceTop,
	float surfaceRight, float surfaceBottom)
{
	_surfaceTop = surfaceTop;
	_surfaceLeft = surfaceLeft;
	_surfaceBottom = surfaceBottom;
	_surfaceRight = surfaceRight;

	_changed = true;
}

void LayerManager::setSurfaceParameters(float cutX, float cutY)
{
	setSurfaceParameters(cutX, cutY, cutX, cutY);
}

void LayerManager::setSurfaceParameters(float cut)
{
	setSurfaceParameters(cut, cut, cut, cut);
}

// surface by name
void LayerManager::setSurface(int surface, float cutX, float cutY)
{
	_surfaceX = cutX;
	_surfaceY = cutY;
	_surface = surface;
	switch(surface)
	{
	case SURFACE_TBCYLINDER:
		_surfaceTop = cutY;
		_surfaceLeft = 0;
		_surfaceBottom = cutY; 
		_surfaceRight = 0;
		break;
	case SURFACE_LRCYLINDER:
		_surfaceTop = 0;
		_surfaceLeft = cutX;
		_surfaceBottom = 0; 
		_surfaceRight = cutX;
		break;
	case SURFACE_SPHERE_CYLINDER:
		_surfaceTop = cutY;
		_surfaceLeft = cutX;
		_surfaceBottom = 0; 
		_surfaceRight = cutX;
		break;
	case SURFACE_CYLINDER_SPHERE:
		_surfaceTop = 0;
		_surfaceLeft = cutX;
		_surfaceBottom = cutY; 
		_surfaceRight = cutX;
		break;
	case SURFACE_TABLETOP:
		break;
	case SURFACE_SCALE:
		break;	
	default:
	case SURFACE_SPHERE:
		_surfaceTop = cutY;
		_surfaceLeft = cutX;
		_surfaceBottom = cutY; 
		_surfaceRight = cutX;
		break;
	}

	_changed = true;
}

void LayerManager::setSurface(int surface, float cut)
{
	setSurface(surface, cut, cut);
}

void LayerManager::setSurface(int surface)
{
	setSurface(surface, _surfaceX, _surfaceY);
}

void LayerManager::setSurface(float cutX, float cutY)
{
	setSurface(_surface, cutX, cutY);
}

void LayerManager::setSurface(float cut)
{
	setSurface(_surface, cut, cut);
}

void LayerManager::enableSurface(int toggle)
{
	switch(toggle)
	{
	case 0:
		_surfaceOn = false;
		break;
	case 1:
		_surfaceOn = true;
		break;
	case -1:
	default:
		_surfaceOn = !_surfaceOn;
		break;
	}
	_changed = true;
}

void LayerManager::getSurfaceRotation(
	float x, float y, float w, float h, double prevScale,
	double *rx, double *ry, double *rz, double *scale)
{
	*rx = 0.0;
	*ry = 0.0;
	*rz = 0.0;
	*scale = 1.0;

	if (!_surfaceOn)
	{
		return;
	}

	double dw = _desktopWidth/2;
	double dh = _desktopHeight/2;

	if (_surface == SURFACE_TABLETOP)
	{
		if (y + h/2 <= 0)
		{
			// totally on the left screen
			*rz = 0;
		}
		else if (y - h/2 > 0)
		{
			// totally on the right screen
			if (x >= 0)
				*rz = 180.0;
			else
				*rz = -180.0;
		}
		else
		{
			*rz = 180.0*(y + h/2)/h;
			if (x < 0)
				*rz = - 180.0*(y + h/2)/h;
		}
		return;
	}

	if (_surface == SURFACE_SCALE)
	{
		if (x - w/2 < -dw)
		{
			if (dw + x > 0)
				*scale = (2/w)*(dw + x);
			else
				*scale = 24/w;	
		}
		else if (x + w/2 > dw)
		{
			if (dw - x > 0)
				*scale = (2/w)*(dw - x);
			else
				*scale = 24/w;
		}
			
		if (*scale <= 0)
		{
			*scale = 1;
		}
		else if (*scale * w < 24)
		{
			*scale = 24/w;
		}
		if (*scale > 1)
		{
			*scale = 1;
		}
		return;
	}

	if (x > dw)
	{
		*rx = _surfaceRight * 90;
	}
	else if (x < - dh)
	{
		*rx = - _surfaceLeft * 90;
	}
	else if (x >= 0)
	{
		*rx = _surfaceRight * (x/dw) * 90;	
	}
	else
	{
		*rx = _surfaceLeft * (x/dh) * 90;
	}

	if (y > dh)
	{
		*ry = _surfaceTop * 90;
	}
	else if (y < - dh)
	{
		*ry = - _surfaceBottom * 90;
	}
	else if (y >= 0)
	{
		*ry = _surfaceTop  * (y/dh) * 90;	
	}
	else
	{
		*ry = _surfaceBottom * (y/dh) * 90;
	}
}

// --------------------------------------------------------------------
// Conditionnal code
bool LayerManager::_isOnCurrentPage(MetisseWindow *win)
{
	float wx, wy, wh, ww;

	if (win == 0)
	{
		return false;
	}
	if (win->getDesk() != _pagerCurrentDesk)
	{
		return false;
	}
	if (win->isIconified())
	{
		return false;
	}
	win->getRealPosition(&wx,&wy);
	win->getSize(&ww,&wh);

	#if 0
	fprintf(stderr,"x: %i, y: %i, w: %i, h: %i (%i %i %i)\n",
		(int)wx,(int)wy,(int)(wx + ww),(int)(wy + wh),
		(int)_desktopWidth, (int)_desktopHeight,
		(!win->isRootWindow() && !win->isEwmhDesktop() &&
		 !win->isUnmanaged() && !win->isSticky()));
	#endif

	// FIXME: 1 - check the desk
	//        2 - check for "visibility" or principal pages
	//            (MUST look fvwm code ...)
	//        3 - what to do with iconified window ?
	if (wx >= 0 && wx + ww <= _desktopWidth && 
	    wy >= 0 && wy + wh <= _desktopHeight)
	{
		// fully on page
		return true;
	}

	return false;
}

// more simple that _oberlap
bool LayerManager::_doOverlap(MetisseWindow *win1, MetisseWindow *win2)
{
	GLdouble left1, top1, right1, bottom1;
	getWinBoundingBox(win1, &left1, &top1, &right1, &bottom1, true);
	GLdouble left2, top2, right2, bottom2;
	getWinBoundingBox(win2, &left2, &top2, &right2, &bottom2, true);
	
	if (right1 <= left2 ||  right2 <= left1)
		return false;
	if (top1 >= bottom2 ||  top2 >= bottom1)
		return false;

	return true;
}

// --------------------------------------------------------------------
//
std::list<MetisseWindow *> LayerManager::getWindowOnCurrentPage(void)
{
	std::list<MetisseWindow *> list;
	list.clear();

	for( std::list<sgNode *>::iterator o=_dependencies.begin();
	     o != _dependencies.end(); 
	     ++o )
	{
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (win && !win->isUnmanaged() && !win->isSticky() &&
		    !win->isRootWindow() && _isOnCurrentPage(win))
		{
		  list.push_front(win);
		}
	}

	return list;
}

// FIXME: the algo can be improved !
std::list<std::list<MetisseWindow *> *>
LayerManager::getWindowOnCurrentPageByStack(bool add_desktop)
{
	std::list<std::list<MetisseWindow *> *> list;
	list.clear();

	std::list<MetisseWindow *> *wl;
	wl = new std::list<MetisseWindow *>;
	wl->clear();
	std::list<MetisseWindow * >::iterator win_it;
	MetisseWindow *twin = 0;
	list.push_back(wl);

	for(std::list<sgNode *>::iterator o=_dependencies.begin();
	    o != _dependencies.end(); 
	    ++o )
	{
		MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
    
		if (win && !win->isUnmanaged() &&
		    (!win->isSticky() || (add_desktop && win->isEwmhDesktop())) &&
		    !win->isRootWindow() && _isOnCurrentPage(win))
		{
			for (win_it = wl->begin(); win_it != wl->end(); win_it++)
			{
				twin = (*win_it);
				if (_doOverlap(win,twin))
				{
					wl = new std::list<MetisseWindow *>;
					wl->clear();
					list.push_back(wl);
					break;
				}
			}
			wl->push_front(win);
		}
	}

	return list;
	
}

// --------------------------------------------------------------------
// Expose

bool LayerManager::inExposeMode(void)
{
	return _inExposeMode;
}

void LayerManager::exposeMode(bool cancel)
{
	static float stransX = 0, stransY = 0, sscaleX = 1, sscaleY = 1;
	_exposeModeStep = 0;

	// FIXME: pager mode !
	if (_inPagerMode ||  _exposeModeTk != 0)
	{
		return;
	}

	if (_inExposeMode)
	{
		_inExposeMode = false;
		if (!cancel)
		{
			_exposeModeCanceled = false;
			_ascr->fixCursorPos(true);
		}
		else
		{
			_exposeModeCanceled = true;
		}
	}
	else
	{
		_inExposeMode = true;
		int count = 0;

		sgNode::saveTransformations();
		stransX = _transX;
		stransY = _transY;
		sscaleX = _scaleX;
		sscaleY = _scaleY;
		for( std::list<sgNode *>::iterator o=_dependencies.begin();
		     o != _dependencies.end(); 
		     ++o )
		{
			MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
			if (win)
				win->getRenderer()->saveTransformations();
		}
		resetTransformations();

		// Count the number of windows
		for( std::list<sgNode *>::iterator o=_dependencies.begin();
		     o != _dependencies.end(); 
		     ++o )
		{
			MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);

			if (win && !win->isRootWindow() && !win->isEwmhDesktop()
			    && !win->isUnmanaged() && !win->isSticky() &&
			    _isOnCurrentPage(win))
			{
				//win->getRenderer()->resetTransformations();
				count++;
			}
		}

		const int nWindows = count;
		const int lines = (int)sqrt(nWindows + 1);
		const int spacing = 10; // add option here

		_exposeModeList.clear();
		_exposeModeList.reserve(nWindows);

		int nSlots  = 0;
		int y = spacing;
		int height =
		  (int)(_desktopHeight - (lines + 1) * spacing) / lines;

		// Create a list of window slots
		std::list<sgNode*>::iterator o = _dependencies.begin();
		for ( ; o != _dependencies.end(); ++o )
		{
			MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);
			if (win == 0)
			{
				continue;
			}
			else if (win->isUnmanaged() || win->isSticky() ||
				 win->isEwmhDesktop())
			{
				// move the window behind the camera
				win->getRenderer()->translate_rel(
					0, 0, -5000000.0);
				if (win->isUnmanaged())
				{
					fprintf(stderr, "Unmanaged\n");
				}
				continue;
			}
			else if (win->isRootWindow() || !_isOnCurrentPage(win))
			{
				continue;
			}

			int x, y, w, h;
			win->getSize(&w,&h);
			win->getRealPosition(&x, &y);
			Slot s(x, y, w, h, win);
			s.orig = Point(x, y);

			_exposeModeList.push_back(s);
		}

		for (int i = 0; i < 20; i++)
		{
			// loop over the windows to move the overlapping ones
			std::vector<Slot>::iterator slot1 = _exposeModeList.begin();
			std::vector<Slot>::iterator slot2 = _exposeModeList.begin();
			for ( ; slot1 != _exposeModeList.end(); slot1++)
			{
				//fprintf(stderr, "%p - ", slot1->win);
				slot2 = slot1;
				slot2++;
				for ( ; slot2 != _exposeModeList.end(); slot2++)
				{
					Rect r1(slot1->rect);
					Rect r2(slot2->rect);
					Rect r = r1.intersection(r2);

					if (!r.isNull())
					{
						Point v12 = r2.center() - r1.center();

						if (r.width() < r.height())
							v12 *= std::abs(r.width() / v12.x());
						else
							v12 *= std::abs(r.height() / v12.y());

						v12 /= 5; // pouet

						slot1->rect.rx() -= v12.x();
						slot1->rect.ry() -= v12.y();
						slot2->rect.rx() += v12.x();
						slot2->rect.ry() += v12.y();
						//fprintf(stderr, "[%p rx=%f ry=%f v12=%f,%f] ", slot2->win, r.width(), r.height(), v12.x(), v12.y());
					}
					else
					{}//fprintf(stderr, "[%p rx=%f ry=%f] ", slot2->win, r.width(), r.height());
				}

				//fprintf(stderr, "\n");
			}
		}

		const int b = 10;
		const Rect desktop(b, b, _desktopWidth-2*b, _desktopHeight-2*b);

		Rect global = _exposeModeList.begin()->rect;
		std::vector<Slot>::iterator s;
		for (s = _exposeModeList.begin(); s != _exposeModeList.end(); s++)
		{
			global = global.bounding(s->rect);
		}

		// maybe I need a Size class like QSize for stuff like this?
		Rect desktopSize(0, 0, desktop.width(), desktop.height());
		Rect globalSize(0, 0, global.width(), global.height());

		// let's find out if the non-overlapping windows go offscreen
		// *but* if they can be translated into the screen without any
		// rescaling...
		if (global.bounding(desktop) != desktop && 
		    desktopSize.bounding(globalSize) == desktopSize)
		{
			// recenter, it's ugly but it'll do for now
			const Point delta(global.center() - desktop.center());
			for (s = _exposeModeList.begin(); s != _exposeModeList.end(); s++)
			{
				s->rect.moveCenter( s->rect.center() + delta );
			}
		}
		// ... or we will have to rescale the whole desktop
		else if (global.bounding(desktop) != desktop && 
		         desktopSize.bounding(globalSize) != desktopSize)
		{
			float f = std::max(global.width()/desktop.width(),
					   global.height()/desktop.height());

			for (s = _exposeModeList.begin(); s != _exposeModeList.end(); s++)
			{
				s->rect.rx() = (s->rect.x() - global.x()) / f + desktop.x();
				s->rect.ry() = (s->rect.y() - global.y()) / f + desktop.y();
				s->rect.rwidth() /= f;
				s->rect.rheight() /= f;
			}

			// find the 4 {left,top,right,bottom}-most windows
			Slot slotL = _exposeModeList.front();
			Slot slotR = _exposeModeList.front();
			Slot slotT = _exposeModeList.front();
			Slot slotB = _exposeModeList.front();
			for (s = _exposeModeList.begin(); s != _exposeModeList.end(); s++)
			{
				if (s->rect.left() < slotL.rect.left())
					slotL = *s;
				if (s->rect.right() > slotR.rect.right())
					slotR = *s;
				if (s->rect.top() < slotT.rect.top())
					slotT = *s;
				if (s->rect.bottom() > slotB.rect.bottom())
					slotB = *s;
			}

			Rect boundingBefore(slotL.rect.center(), slotT.rect.center(),
					    slotR.rect.center(), slotB.rect.center());

			// position those 4 windows correctly
			Slot slotL2 = slotL, slotR2 = slotR;
			Slot slotT2 = slotT, slotB2 = slotB;

			slotL2.rect.rx() = desktop.left();
			slotR2.rect.rx() = desktop.right() - slotR2.rect.width();
			slotT2.rect.ry() = desktop.top();
			slotB2.rect.ry() = desktop.bottom() - slotB2.rect.height();

			Rect boundingAfter(slotL2.rect.center(), slotT2.rect.center(),
					    slotR2.rect.center(), slotB2.rect.center());

			// find the correct transform and apply to each window
			Transform2D M = boundingBefore.findTransformTo(boundingAfter);
			for (s = _exposeModeList.begin(); s != _exposeModeList.end(); s++)
			{
				Eigen::Vector3f v1;
				v1.x() = s->rect.center().x();
				v1.y() = s->rect.center().y();
				v1.z() = 1;

				Eigen::Vector3f v2 = M * v1;
				s->rect.moveCenter( Point(v2.x(), v2.y()) );
			}
		}

#if 0
		for ( ; o != _dependencies.end(); ++o )
		{
			MetisseWindow *win = dynamic_cast<MetisseWindow *>(*o);

			if (win == 0)
			{
				continue;
			}
			else if (win->isUnmanaged() || win->isSticky() ||
				 win->isEwmhDesktop())
			{
				// move the window behind the camera
				win->getRenderer()->translate_rel(
					0, 0, -5000000.0);
				if (win->isUnmanaged())
				{
					fprintf(stderr, "Unmanaged\n");
				}
				continue;
			}
			else if (win->isRootWindow() ||
				 !_isOnCurrentPage(win))
			{
				continue;
			}

			float win_w, win_h;
			int win_x, win_y;
			win->getSize(&win_w,&win_h);
			win->getRealPosition(&win_x, &win_y);

			float win_cx, win_cy;
			win_cx = win_x + win_w/2.0;
			win_cy = win_y + win_h/2.0;

			long closest_distance;
			bool set = false;
			std::vector<Slot>::iterator closest_slot;

			closest_slot = _exposeModeList.end();

			std::vector<Slot>::iterator slot =
				_exposeModeList.begin();
			for ( ; slot != _exposeModeList.end(); slot++)
			{
				if (slot->win)
					continue;

				long distance = (int)(
					pow(win_cx - slot->cx, 2) +
					pow(win_cy - slot->cy, 2));
				if (!set || distance < closest_distance)
				{
					closest_distance = distance;
					closest_slot = slot;
					set = true;
				}
			}
			if (closest_slot != _exposeModeList.end())
			{
				win->getRealPosition(
					&closest_slot->orig_x,
					&closest_slot->orig_y);
				closest_slot->win = win;
			}
			else
			{
				fprintf(stderr,"Expose: SHOULD NOT HAPPEN\n");
			}
		}
#endif
	}
	
	_exposeModeTk = TimeKeeper::create();
	subscribeTo(_exposeModeTk);
	_exposeModeTk->arm(1);
}
