/*
 *
 * desktop/MetisseDesktop.cxx --
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

#include "MetisseDesktop.H"

#include <stdio.h>

#include <sstream>
#include <iostream>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <nucleo/gl/scenegraph/sgNode.H>
#include <nucleo/gl/scenegraph/sgClipper.H>

#include "main/AScreen.H"
#include "renderer/WindowRenderer.H"
#include "main/AUtils.H"
#include "main/Persistence.H"

#define DEBUG_LEVEL 0

//-----------------------------------------------------------------------

MetisseWindow *MetisseDesktop::findWindow(Window id)
{
	WindowMapping::iterator i = _windows.find(id);

	if (i !=_windows.end())
	{
		return (*i).second ;
	}

	return 0;
}


WindowMapping MetisseDesktop::getWindowMapping(void)
{
	return _windows;
}

bool MetisseDesktop::isWindowValide(MetisseWindow *win)
{
	WindowMapping::iterator j;
	for (j = _windows.begin(); j != _windows.end(); ++j)
	{
		MetisseWindow *dw = (*j).second;
		if (dw == win)
			return true;
	}
	
	return false;
}

void MetisseDesktop::_addWindow(
	Window id, int x, int y, unsigned int width, unsigned int height,
	bool map, int isroot)
{
	char tmp[256] ;
	sprintf(tmp,"0x%lx",id);

	MetisseWindow *window = new MetisseWindow(
		tmp, id, _metisseSource, _ascr, x, y, width, height);

	if (!window)
	{
		std::cerr << "Unable to create XwnxWindow for " << tmp
			  << std::endl;
		return;
	}
  
	_windows[id] = window;
	if (map)
	{
		_ascr->root->addDependency(window);
		window->setMapped(true);
	}
	else
	{
		window->setMapped(false);
	}

	if (isroot)
	{
		_ascr->root->setRootWin(window);
		window->setRootWindow();
	}

	WindowRenderer *renderer = new WindowRenderer(_ascr, window);
	window->setRenderer(renderer);
	subscribeTo(renderer);

#if DEBUG_PUSH_NAME
	std::cerr << "Add Window " <<  window << " " << renderer << std::endl;
#endif

	unsigned int win_w, win_h;
	_ascr->getDesktopSize(&win_w, &win_h);
	float win_width = win_w;
	float win_height = win_h;
	float px = x;
	float py = y;

	window->translate_rel(
		- (win_width-width)/2 +  px, (win_height-height)/2 - py, 0);
	window->setRealPosition(x, y);
	if (isroot)
	{
#if 0
		std::cerr << "Added the root window "
			  << x << " " << y << " " << width << "x" << height
			  << " a: " << d_x << " " << d_y << " " << win_w << "x"
			  << win_h << std::endl;
#endif
	}
#if DEBUG_LEVEL > 0
	std::cerr << "Added Window: " << tmp << " at:" << px << ","
		  << py <<std::endl;
#endif
}

void MetisseDesktop::handleConfigureWindow(
	Window id, int x, int y, int width, int height, int isroot)
{
	WindowMapping::iterator i = _windows.find(id);

	MetisseWindow *win = findWindow(id);

	if (!win)
	{
		_addWindow(id, x, y, width, height, false, isroot);
		return;
	}
	
	float oldX, oldY;
	float oldW, oldH;
	bool changed = false;
	bool sizeChanged = false;

	if (isroot)
	{
		std::cerr << "Get a config for the root window" << std::endl;
		return; // ???
	}
	win->getCurrentSize(&oldW, &oldH);
	win->getCurrentRealPosition(&oldX, &oldY);

#if 0
	std::cerr << "Get a ConfigureWindow" << std::endl;
#endif
	if ((int)oldW != width || (int)oldH != height)
	{
		win->getRenderer()->setShape(0, 0, _ascr->hasStencilBuffer());
		win->resetTexture(width, height);
		win->getRenderer()->unlens();
		sizeChanged = changed = true;
#if 0
		std::cerr << "New Size: " << width << " / " << oldW << " // "
			  << height << " / " << oldH << std::endl;
#endif
	}
	if (x != oldX || y != oldY)
	{

		if (win->inTextureReset())
		{
			win->textureResetSetRealPosition(x, y);
		}
		else if (!_ascr->inInternalMove(win))
		{
			win->translate_rel(-oldX + x, oldY - y, 0);
			win->setRealPosition(x, y);
		}
		win->setInMovePage(false);
		changed = true;
#if 0
		std::cerr << "New Positon: " << x << " / " << oldX
			  << " // " << y << " / " << oldY << std::endl;
#endif
	}

	if (_ascr->waitingForAConfigureWindow())
	{
		_ascr->draw(false);
		//std::cerr << "MD1 resetSelection\n";
		_ascr->resetSelection();
	}
	else if (changed && !_ascr->inInteractiveMove() &&
		 !_ascr->inInteractiveResize())
	{
		//std::cerr << "MD2 resetSelection\n";
		_ascr->scheduleResetSelection();
		//_ascr->draw(false);
		//_ascr->resetSelection();
	}

	if (sizeChanged)
	{
		sgNode *n;
		FacadeMapping wcm =  win->getFacade();
		if (wcm.size() > 0 && !win->isAFacadeWindow())
		{
			//_metisseSource->windowUpdateRequest(
			//		win->getFrameID());
			#if 1
			// should not do that ... must wait the new facades
			// argement: pb window shade and unmaximize!
			FacadeMapping::iterator iter;
			for (iter = wcm.begin(); iter != wcm.end(); iter++)
			{
				MetisseWindow *xmWin =
					dynamic_cast<MetisseWindow *>
					((*iter).first);
				if (xmWin == 0)
				{
					continue;
				}
				_metisseSource->windowUpdateRequest(
					xmWin->getFrameID());
			}
			#endif
		}
		else if ((n = win->getDuplicateFor()))
		{
			// FIXME: does not wrok correctly!
			MetisseWindow *xmWin =
					dynamic_cast<MetisseWindow *>(n);
			_metisseSource->windowUpdateRequest(xmWin->getFrameID());
			// std::cerr << "Duplicate Resized\n";
		}
		#if 0
		if (!_ascr->inInteractiveResize() && !win->isUnmanaged())
		{
			std::cerr << "MetisseDesktop call "
				  << "persistence_window_check\n";
			persistence_window_check(win, false, _ascr);
		}
		#endif
	}
}

void MetisseDesktop::handleCursorPosition(int x, int y)
{
	int px,py;
	if (_ascr->doProjectCursor())
	{
		_ascr->root->x11projectWarp(x, y, &px, &py);
	}
	else
	{
		px = x;
		py = y;
	}
#if 0
	std::cerr << "MetisseDesktop::handleCursorPosition: " << x << "," << y
		  << " --> " << px << "," << py << std::endl;
#endif

	_ascr->compositor->warpCursor(px, py);
	if (!_ascr->inInteractiveMove() && !_ascr->inInteractiveResize())
	{
		//std::cerr << "MD3 resetSelection\n";
		_ascr->resetSelection(px, py);
		_ascr->scheduleResetSelection();
	}
	else
	{
		// FIXME (moving/resizing with the keyboard arrows)
	}
}

void MetisseDesktop::handleImageFramebufferUpdate(
	MetisseWindow *win, bool isRoot, Image *img, int x, int y, unsigned int w,
	unsigned int h)
{

	if (!win)
	{
		std::cerr << "handleImageFramebufferUpdate for a window we do "
			  << "not know" << std::endl ;
		return;
	}

	float wH, wW;
	win->getCurrentSize(&wW, &wH);
	if (x+w > (unsigned int)wW || y+h > (unsigned int)wH)
	{
		return;
	}
	win->updateTexture(img, x, y, w, h);

	// see if we are duplicate or a cuts
	WindowMapping::iterator j;
	for (j = _windows.begin(); j != _windows.end(); ++j)
	{
		MetisseWindow *dw = (*j).second;
		if (dw->getDuplicateFor() == win)
		{
			dw->updateTexture(img, x, y, w, h, true);
		}
		else
		{
			dw->facadeUpdateTexture(
				(sgNode *)win, img, x, y, w, h);
		}
	}
}

void MetisseDesktop::handleWindowShape(MetisseWindow *win, CARD16 *buf, int nrecs)
{
	if (!win)
	{
		#if 0
		std::cerr << "handleWindowShape for a window we do "
			  << "not know" << std::endl;
		#endif
		if (buf)
		{
			delete buf;
		}
		return;
	}

	win->getRenderer()->setShape(buf, nrecs, _ascr->hasStencilBuffer());

	if (_ascr->hasStencilBuffer())
	  return;
}

void MetisseDesktop::handleWindowFacades(
	MetisseWindow *win, CARD32 *buf, CARD32 ncuts)
{
	if (!win)
	{
		std::cerr << "handleWindowFacades for a window we do "
			  << "not know" << std::endl ;
		if (buf)
		{
			delete buf;
		}
		return;
	}

	unsigned int size = ncuts*7;
	FacadeMapping facade;

	facade.clear();
	//_metisseSource->windowUpdateRequest(win->getFrameID());
	win->deleteTexture();
	_metisseSource->windowUpdateRequest(win->getFrameID());
	for (int i = 0; i+6 < size; i = i+7)
	{
		sgNode *w = findWindow(buf[i]);
		if (!w)
		{
			continue;
		}
		FacadeMapping::iterator iter = facade.find(w);
		if (iter != facade.end())
		{
			(*iter).second.push_front(
				cut(buf[i+1],buf[i+2],buf[i+3],buf[i+4],buf[i+5],
				    buf[i+6]));
		}
		else
		{
			cutlist cl;
			cl.push_front(
				cut(buf[i+1],buf[i+2],buf[i+3],buf[i+4],buf[i+5],
				    buf[i+6]));
			facade[w] = cl;
		}
	}

	bool changed = win->setFacade(facade, false);

	win->setIsAFacadeWindow();
	// request update for cuts
	if (changed)
	{
		FacadeMapping::iterator iter;
		for (iter = facade.begin(); iter != facade.end(); iter++)
		{
			MetisseWindow *xmWin =
				dynamic_cast<MetisseWindow *>((*iter).first);
			if (xmWin == 0)
			{
				continue;
			}
			_metisseSource->windowUpdateRequest(
				xmWin->getFrameID());
		}
	}
	_metisseSource->windowUpdateRequest(win->getFrameID());

	if (buf)
	{
		delete buf;
	}
}

void MetisseDesktop::handleUnmapWindow(Window id)
{
	WindowMapping::iterator i = _windows.find(id);

	if (i == _windows.end())
	{
		std::cerr << "Trying to unmap a window we don't know ("
			  << std::hex << id << std::dec << ")" << std::endl ;
		return;
	}

	MetisseWindow *win = (*i).second ;

	//std::cerr << "Pre Unmap\n";
	if (!win->isUnmanaged())
	{
		// ?
		// persistence_window_remove(win, _ascr);
	}
	if (!win->isMapped())
	{
		return;
	}
	//std::cerr << "Unmap\n";
	win->setMapped(false);
	win->getRenderer()->unlens();

	_ascr->root->removeDependency(win);
	win->unmapCorrection();
	_ascr->draw(false);
	//std::cerr << "MD4 resetSelection\n";
	_ascr->resetSelection();
	// save memory. we must get a full frame buffer update at remap
	win->deleteTexture();
}

void MetisseDesktop::handleDestroyWindow(Window id)
{
	WindowMapping::iterator i = _windows.find(id) ;
	if (i==_windows.end())
	{
		std::cerr << "Trying to remove a window we don't know ("
			  << std::hex << id << std::dec << ")" << std::endl ;
		return ;
	}

	MetisseWindow *win = (*i).second ;

	//std::cerr << "Destroy\n";
	if (!win->isUnmanaged())
	{
		persistence_window_remove(win, _ascr);
	}

	_windows.erase(id) ;
	if (win->isMapped())
	{
		_ascr->root->removeDependency(win) ;
		_ascr->draw(false);
		//std::cerr << "MD5 resetSelection\n";
		_ascr->resetSelection();
	}

#if DEBUG_LEVEL > 0
	std::cerr << std::hex << id << std::dec << " removed from the desktop"
		  << std::endl ;
#endif

	
	// see if we are a "parent" 
	WindowMapping::iterator j;
	for (j = _windows.begin(); j != _windows.end(); ++j)
	{
		MetisseWindow *w = (*j).second;
		if (w->getTransientFor() == win)
		{
			w->setTransientFor(0);
		}
		if (w->getUnmanagedFor() == win)
		{
			w->setUnmanagedFor(0);
		}
		if (w->getDuplicateFor() == win)
		{
			_closeWindow(w);
		}
		if (w->getPseudoSwallowedNode() == win)
		{
			w->pseudoSwallow(0);
			w->getRenderer()->unlens();
			w->getRenderer()->simpleReset();
		}
		w->rmFacade((sgNode *)win);
	}
	_ascr->windowDestroyed(win);
	
	delete win;
}

void MetisseDesktop::_handleWindowFlags(
	MetisseWindow *win, Window transientFor, Window unmanagedFor,
	Window grabWin, Window duplicateFor, unsigned long facadeReal,
	unsigned long flags)
{
	MetisseWindow *mTransientFor = 0;
	MetisseWindow *mUnmanagedFor = 0;
	MetisseWindow *mSelectionUnmanagedFor = 0;
	bool unmanagedByHint = false;

	if ((flags & rfbWindowFlagsOverrideRedirect) ||
	    (flags & rfbWindowFlagsUnmanaged))
	{
		win->setUnmanaged();
#if 0
		std::cerr << "Unmanaged: "
			  << std::hex << win->getFrameID() << std::dec << " "
			  << grabWin << std::endl ;
#endif
	}

	if (flags & rfbWindowFlagsEwmhDesktop)
	{
		win->setEwmhDesktop();
		return;
	}
	if (flags & rfbWindowFlagsNetChecking)
	{
		win->hide();
		return;
	}
	if (flags & rfbWindowFlagsFacadeSaved)
	{
		persitence_facade_realised(win, facadeReal);
	} 
	// reset TransientFor
	win->setTransientFor(0);
	win->setUnmanagedFor(0);
	
	if (flags & rfbWindowFlagsTransient)
	{
		WindowMapping::iterator k = _windows.find(transientFor);
		if (transientFor && k == _windows.end())
		{
			std::cerr << "Unkown parent for transient win: "
				  << std::hex << win->getFrameID() << std::dec
				  << std::endl ;
		}
		else if (0 && (*k).second->isRootWindow())
		{
		}
		else
		{
			if (win->isUnmanaged())
			{
				mUnmanagedFor = (*k).second;
				unmanagedByHint = true;
			}
			else
			{
				mTransientFor = (*k).second;
			}
#if 0
			std::cerr << "Transient For " << std::hex
				  <<  mTransientFor->getFrameID()
				  << std::dec << std::endl;
#endif
		}
#if 0
		std::cerr << "Transient For " << std::hex <<  transientFor
			  << std::dec << " (" << win->isMapped()
			  << ")" << std::endl;
#endif
	}

	if (unmanagedFor != None)
	{
		WindowMapping::iterator k = _windows.find(unmanagedFor);
		if (k == _windows.end())
		{
			std::cerr << "Unkown parent for um transient win: "
				  << std::hex << win->getFrameID()
				  << std::dec << std::endl ;
		}
		else if (0 && (*k).second->isRootWindow())
		{
		}
		else
		{
			mUnmanagedFor = (*k).second;
		}
		#if 0
		std::cerr << "Unmanaged For by Hint" << std::hex <<  unmanagedFor
			  << std::dec << std::endl;
		#endif
	}
	else if (!(flags & rfbWindowFlagsTransient) && win->isUnmanaged() &&
		 grabWin != None && !(flags & rfbWindowFlagsNetChecking))
	{
		
		WindowMapping::iterator k = _windows.find(grabWin);
		if (k == _windows.end())
		{
			std::cerr << "Unkown grab window for: "
				  << std::hex << win->getFrameID() << std::dec
				  << std::endl;
		}
		else if ((*k).second == win)
		{
			// This happen with QT and GIMP main draw win menu
			//std::cerr << "Grab window is itself" << std::endl;
			MetisseWindow *sw = _ascr->getSelectedWindow();
			if (sw && sw != (*k).second && !sw->isRootWindow())
			{
				#if 0
				std::cerr << "UnmanagedFor by Grab win "
					  << "Selection: "
					  << std::hex
					  << sw->getFrameID() << std::dec
					  << std::endl;
				#endif
				mSelectionUnmanagedFor = sw;
			}
		}
		else if (0 && (*k).second->isRootWindow())
		{
		}
		else
		{
			mUnmanagedFor = (*k).second;
			#if 0
			std::cerr << "UnmanagedFor by Grab win: " << std::hex
				  << mUnmanagedFor->getFrameID()
				  << std::dec << std::endl;
			#endif
		}
	}
	else if (win->isUnmanaged() &&
		 !(flags & rfbWindowFlagsTransient))
	{
		MetisseWindow *sw = _ascr->getSelectedWindow();
		if (sw && sw != win && !sw->isRootWindow())
		{
			#if 0
			std::cerr << "UnmanagedFor by Selection: "
				  << std::hex << sw->getFrameID()
				  << std::dec << std::endl;
			#endif
			mSelectionUnmanagedFor = sw;
		}
	}

	if (flags & rfbWindowFlagsToolTips)
	{
#if 0
		std::cerr << "ToolTips " << std::endl;
#endif
		win->setToolTips();
	}

	// should respect facade and duplicate

	// One point with fvwm menu over facade and duplicate:
	// we get mUnmanagedFor correct in this case! 
	MetisseWindow *sw = _ascr->getSelectedWindow();
	MetisseWindow *dupTransientFor = 0;
	MetisseWindow *dupUnmanagedFor = 0;
	MetisseWindow *dupSelectionUnmanagedFor = 0;
	cut *fcut = new cut(0,0,0,0,0,0);
	bool rec = false;
	if (sw && !sw->isRootWindow())
	{
		MetisseWindow *psw = 0;
		if (sw->getTransientFor() != 0)
		{
			//std::cerr << "REC TRANSIENT ON SELECTION" << std::endl;
			psw = sw;
			sw = dynamic_cast<MetisseWindow *>(sw->getTransientFor());
			rec = true;
		}

		if (mTransientFor && mTransientFor != sw &&
		    sw->getDuplicateFor() == mTransientFor)
		{
			dupTransientFor = sw;
		}
		else if (mUnmanagedFor && mUnmanagedFor != sw &&
			 sw->getDuplicateFor() == mUnmanagedFor)
		{
			dupUnmanagedFor = sw;
		}
		if (mTransientFor && mTransientFor != sw &&
		    sw->hasAFacade() &&
		    (rec || _ascr->getSelectedFacade(fcut) == mTransientFor))
		{
			dupTransientFor = sw;
			if (rec)
			{
				cut *c = psw->getCutOrig();
				if (c != 0 && fcut)
				{
					memcpy(fcut, c, sizeof(cut)); 
				}
				else
				{
					delete fcut;
					fcut = 0;
				}
			}
		}
		else if (mUnmanagedFor && mUnmanagedFor != sw &&
			 sw->hasAFacade() &&
			 (rec ||
			  _ascr->getSelectedFacade(fcut) == mUnmanagedFor))
		{
			dupUnmanagedFor = sw;
			if (rec)
			{
				cut *c = psw->getCutOrig();
				if (c != 0 && fcut)
				{
					memcpy(fcut, c, sizeof(cut)); 
				}
				else
				{
					delete fcut;
					fcut = 0;
				}
			}
		}
		else if (mSelectionUnmanagedFor && sw->hasAFacade())
		{
			// found the orig cut!
			dupSelectionUnmanagedFor =
				_ascr->getSelectedFacade(fcut);
		}
		else if (mSelectionUnmanagedFor)
		{
			// get the orig!
			dupSelectionUnmanagedFor = 
				dynamic_cast<MetisseWindow *>
				(mSelectionUnmanagedFor->getDuplicateFor());
		}
	}

	// recursivness!
	MetisseWindow *recUnTrans = 0;
	MetisseWindow *recNoTrans = 0;
	MetisseWindow *recTrans = 0;
	if (dupTransientFor && dupTransientFor->getTransientFor() != 0)
	{
		recTrans = recNoTrans =
			dynamic_cast<MetisseWindow *>(dupTransientFor);
	}
	else if (mTransientFor && mTransientFor->getTransientFor() != 0)
	{
		recTrans = recNoTrans =
			dynamic_cast<MetisseWindow *>(mTransientFor);
	}
	else if (dupUnmanagedFor && (dupUnmanagedFor->getTransientFor() != 0))
	{
		recTrans = recUnTrans =
			dynamic_cast<MetisseWindow *>(dupUnmanagedFor);
	}
	else if (mUnmanagedFor && mUnmanagedFor->getTransientFor() != 0 )
	{
		recTrans = recUnTrans =
			dynamic_cast<MetisseWindow *>(mUnmanagedFor);
	}
	else if (dupSelectionUnmanagedFor &&
		 dupSelectionUnmanagedFor->getTransientFor() != 0)
	{
		recTrans = recUnTrans =
			dynamic_cast<MetisseWindow *>(mSelectionUnmanagedFor);
	}
	else if (mSelectionUnmanagedFor &&
		 mSelectionUnmanagedFor->getTransientFor() != 0)
	{
		recTrans = recUnTrans =
			dynamic_cast<MetisseWindow *>(mSelectionUnmanagedFor);
	}


	if (recTrans != 0)
	{
		//std::cerr << "REC TRANSIENT" << std::endl;
		cut *c = recTrans->getCutOrig();
		if (c != 0 && fcut)
		{
			memcpy(fcut, c, sizeof(cut)); 
		}
		else if (fcut != 0)
		{
			delete fcut;
			fcut = 0;
		}
		if (recNoTrans)
		{
			win->setTransientFor(
				recTrans->getTransientFor(),
				recTrans->getTransientForOrig(),
				fcut);
		}
		else
		{
			win->setUnmanagedFor(
				recTrans->getTransientFor(),
				recTrans->getTransientForOrig(),
				fcut);
		}
	}
	else if (dupTransientFor)
	{
		win->setTransientFor(dupTransientFor, mTransientFor, fcut);
		//std::cerr << "SHOULD TRANSLAT 1" << std::endl;
		// done here: LayerManager::attachWindow
	}
	else if (mTransientFor)
	{
		win->setTransientFor(mTransientFor);
		if (fcut)
		{
			delete fcut;
		}
	}
	else if (dupUnmanagedFor)
	{
		win->setUnmanagedFor(dupUnmanagedFor, mUnmanagedFor, fcut);
		//std::cerr << "SHOULD TRANSLAT 2" << std::endl;
		// done here: LayerManager::attachWindow
	}
	else if (mUnmanagedFor)
	{
		win->setUnmanagedFor(mUnmanagedFor);
		if (fcut)
		{
			delete fcut;
		}
	}
	else if (dupSelectionUnmanagedFor)
	{
		// correct! as the selection gives the window we want and
		// so dup is the original!
		win->setUnmanagedFor(
			mSelectionUnmanagedFor, dupSelectionUnmanagedFor,
			fcut);
		//std::cerr << "SHOULD TRANSLAT 3" << std::endl;
		// done here: LayerManager::attachWindow
	}
	else if (mSelectionUnmanagedFor)
	{
		win->setUnmanagedFor(mSelectionUnmanagedFor);
		if (fcut)
		{
			delete fcut;
		}
	}
	else if (fcut)
	{
		delete fcut;
	}
}

void MetisseDesktop::handleRestackWindow(
	Window id, Window nextId, Window transientFor, 
	Window unmanagedFor, Window grabWin, Window duplicateFor,
	unsigned long facadeReal, unsigned long flags)
{
	WindowMapping::iterator i = _windows.find(id);
	WindowMapping::iterator j = _windows.find(nextId);

	if (i == _windows.end())
	{
		std::cerr << "Trying to restack a window we don't know ("
			  << std::hex << id << std::dec << ")" << std::endl ;
		return;
	}

	MetisseWindow *win = (*i).second ;

	if (!win->isMapped())
	{
		_handleWindowFlags(
			win, transientFor, unmanagedFor, grabWin, duplicateFor,
			facadeReal, flags);
	}

	MetisseWindow *nextWin = 0;
	if (j != _windows.end())
	{
		nextWin = (*j).second ;
	}


	if (flags & rfbWindowFlagsDuplicate)
	{
		WindowMapping::iterator k = _windows.find(duplicateFor);
		if (k == _windows.end())
		{
			std::cerr << "Unkown duplicate for: "
			  << std::hex << id << std::dec << std::endl ;
		}
		else
		{
#if 0
			std::cerr << "Duplicate " << std::hex <<  duplicateFor
				  << " " << id << std::dec << std::endl;
#endif
			win->setDuplicateFor((*k).second);
			_metisseSource->windowUpdateRequest(
				(*k).second->getFrameID());
		}
	}
	else if ((flags & rfbWindowFlagsFacade))
	{
#if 0
		std::cerr << "Facade " << std::hex <<  duplicateFor
			  << " " << id << std::dec << std::endl;
#endif
		win->setIsAFacadeWindow();
	}

	if (flags & rfbWindowFlagsInputOnly)
	{
		std::cerr << "Input Only window " << std::hex << id << std::dec
			  << std::endl;
	}

	if (win->isMapped())
	{
		_ascr->root->restack(win, nextWin, false);
		if (!_ascr->inInteractiveResize())
		{
			// _ascr->scheduleResetSelection(); <- more is needed!
			_ascr->draw(false);
			// std::cerr << "MD6 resetSelection\n";
			_ascr->resetSelection();
		}
	}
	else
	{
		_ascr->root->restack(win, nextWin, true);
		if ((!win->isUnmanaged() && win->neverMapped()))
		{
			_ascr->root->translateForWindow(win);
		}
		win->mapCorrection();
		win->setMapped(true);

		// should do that ...
		_ascr->draw(false);
		//std::cerr << "MD7 resetSelection\n";
		_ascr->resetSelection();
		//_ascr->scheduleResetSelection();
	}
}

// ----------------------------------------------------------------------------

void MetisseDesktop::windowUpdateRequest(MetisseWindow *w)
{
	_metisseSource->windowUpdateRequest(w->getFrameID());
}

// ----------------------------------------------------------------------------

void 
MetisseDesktop::setCursor(ACursor *cursor)
{
	_cursor = cursor;
}

// ----------------------------------------------------------------------------

void 
MetisseDesktop::rootPointerEvent(int x, int y, unsigned long button_mask)
{
	_metisseSource->pointerEvent(_xroot, x, y, button_mask);
}

void 
MetisseDesktop::rootKeyEvent(unsigned long key, bool down_flag)
{
	_metisseSource->keyEvent(key, down_flag);
}

void 
MetisseDesktop::setModifiers(glWindow::event e, bool down_flag)
{
#if 0
	if (IsModifierKey(e.keysym))
	{
		KeyCode kc = XKeysymToKeycode(_xdisplay, e.keysym);
		if ((int)kc < 256) _modifierPressed[(int)kc] = down_flag;
	}
#endif
}

void 
MetisseDesktop::releaseModifiers(void)
{
#if 0
	int i;
	KeySym ks;

	for (i = 0; i < 256; i++)
	{
		if (_modifierPressed[i])
		{
			ks = XKeycodeToKeysym(_xdisplay, i, 0);
			_metisseSource->keyEvent(ks, false);
			_modifierPressed[i] = false;
		}
	}
#endif
}

void MetisseDesktop::getSize(unsigned int *width, unsigned int *height)
{
	_metisseSource->getSize(width, height);
}

// -------------------------------------------------------------------------

char *MetisseDesktop::getMetisseServerDisplay(void)
{
	return _msDisplayName;
}

void MetisseDesktop::setAScreen(AScreen *ascr)
{
	_ascr = ascr;
}

void MetisseDesktop::disconnect(void)
{
	_metisseSource->stop();
}

MetisseDesktop::MetisseDesktop(
	std::string rfburi, std::string style, bool use_shm, const char *encoding)
{
	_style = style ;
	_xdisplay = 0 ;
	_cursor = NULL;

	URI uri(rfburi) ;

	std::stringstream tmp ;
	tmp << uri.host << ":" << uri.port ;
	_xdisplay = XOpenDisplay(tmp.str().c_str());
	_msDisplayName = (char *)malloc(strlen(tmp.str().c_str())+1);
	_msDisplayName[0] = 0;
	strcat(_msDisplayName, tmp.str().c_str());

	if (!_xdisplay)
	{
		std::cerr << "[MetisseDesktop] Can't open Xmetisse display"
			  << std::endl ;
		ReactiveEngine::stop() ;
		// exit(1);
		return ;
	}

	_xroot = XDefaultRootWindow(_xdisplay);

	_metisseSource = new MetisseSource(this, encoding, uri, use_shm);
	if (!_metisseSource)
	{
		std::cerr << "[MetisseDesktop] Can't create metisseSource"
			  << std::endl ;
		ReactiveEngine::stop() ;
		// exit(1);
		return ;
	}
	subscribeTo(_metisseSource) ;

	bool ret = _metisseSource->start();

	if (!ret)
	{
		std::cerr << "[MetisseDesktop] Can't start metisseSource" << std::endl;
		ReactiveEngine::stop() ;
		// exit(1);
		return ;
	}

	for (int i = 0; i < 256; i++)
	{
		_modifierPressed[i] = false;
	}
}

MetisseDesktop::MetisseDesktop(std::string rfburi)
	: MetisseDesktop(rfburi, "foldable", false, (const char *)0)
{
}

// --------------------------------------------------------------------------
// react ...

void
MetisseDesktop::react(Observable *obs) {
  // std::cerr << "MetisseDesktop::react this=" << this << " obs=" << obs << std::endl ;
  notifyObservers() ;
}

// --------------------------------------------------------------------------

Bool
MetisseDesktop::HandleXCursor(
	int xhot, int yhot, int width, int height, char *buf,
	const rfbXCursorColors &colors)
{
	if (_cursor)
	{
		_cursor->handleXCursor(
			xhot, yhot, width, height, buf, colors);
		return true;
	}
	return false;
}

Bool
MetisseDesktop::HandleARGBCursor(
	int xhot, int yhot, int width, int height, char *buf)
{

	if (_cursor)
	{
		_cursor->handleARGBCursor(xhot, yhot, width, height, buf);
		return true;
	}
	return false;
}

// ------------------------------------------------------------
void MetisseDesktop::_closeWindow(MetisseWindow *win)
{
	_ascr->sendFvwmCmd("Close", win);
}

// ------------------------------------------------------------

// FIXME FIXME
void MetisseDesktop::_setSubFacadeProperty(Window win, FacadeMapping facade)
{
	char cmd[256];
	FacadeMapping::iterator iter;
	for (iter = facade.begin(); iter != facade.end(); iter++)
	{
		XClientMessageEvent ev;
		MetisseWindow *dw =
			dynamic_cast<MetisseWindow *>((*iter).first);

		sprintf(cmd, "Exec metisse-xlib SubFacadeProperty %lu",
			dw->getClientID());
		_ascr->sendFvwmCmd(cmd,0);	
	}
}


void MetisseDesktop::addOverFacade(
	WinRegions::region *r, MetisseWindow *src_win, MetisseWindow *dest_win,
	int dest_x, int dest_y)
{
	//std::cerr << "addOverFacade\n";
	if (dest_win->isAFacadeWindow())
	{
		return;
	}

	FacadeMapping facade = dest_win->getFacade();
	FacadeMapping::iterator iter = facade.find(src_win);

	//std::cerr << "Add an over Facade\n";
	float src_width,src_height, dest_width, dest_height;
	dest_win->getSize(&dest_width,&dest_height);
	src_win->getSize(&src_width,&src_height);

	if (iter != facade.end())
	{
		(*iter).second.push_front(
			cut((int)(r->x),
			    (int)(r->y),
			    (int)r->w,
			    (int)r->h,
			    (int)(dest_x),
			    (int)(dest_y)));
	}
	else
	{
		cutlist cl;
		cl.push_front(
			cut((int)(r->x),
			    (int)(r->y),
			    (int)r->w,
			    (int)r->h,
			    (int)(dest_x),
			    (int)(dest_y)));   
		facade[src_win] = cl;
	}
	dest_win->setFacade(facade, true);
	_metisseSource->windowUpdateRequest(src_win->getFrameID());
	src_win->getRenderer()->rmMatchingSelectedRegions(r);
	if (dest_win->getFrameID() != 0)
	{
		_setSubFacadeProperty(dest_win->getClientID(), facade);
	}
	return;
}

void MetisseDesktop::addFacade(
	WinRegions::region *r, MetisseWindow *src_win, MetisseWindow *dest_win,
	GLdouble dest_x, GLdouble dest_y, int pointer_x, int pointer_y)
{
	// std::cerr << "addFacade\n";
	if (!dest_win->isAFacadeWindow())
	{
		FacadeMapping facade = dest_win->getFacade();
		FacadeMapping::iterator iter = facade.find(src_win);

		//std::cerr << "Add an over Facade\n";
		float src_width,src_height, dest_width, dest_height;
		dest_win->getSize(&dest_width,&dest_height);
		src_win->getSize(&src_width,&src_height);

		if (iter != facade.end())
		{
			(*iter).second.push_front(
				cut((int)(r->x + src_width/2),
				    (int)(-r->y + src_height/2),
				    (int)r->w, (int)r->h,
				    (int)(dest_x + dest_width/2 - r->w/2),
				    (int)(-dest_y + dest_height/2 - r->h/2)));
		}
		else
		{
			cutlist cl;
			cl.push_front(
				cut((int)(r->x + src_width/2),
				    (int)(-r->y + src_height/2),
				    (int)r->w, (int)r->h,
				    (int)(dest_x + dest_width/2 - r->w/2),
				    (int)(-dest_y + dest_height/2- r->h/2)));   
			facade[src_win] = cl;
		}
		dest_win->setFacade(facade, true);
		_metisseSource->windowUpdateRequest(src_win->getFrameID());
		src_win->getRenderer()->rmMatchingSelectedRegions(r);
		if (dest_win->getFrameID() != 0)
		{
			_setSubFacadeProperty(dest_win->getClientID(), facade);
		}
		return;
	}

	char cmd[1024];
	float width,height;
	src_win->getSize(&width,&height);

	sprintf(cmd, "Exec metisse-xlib AddFacade %lu %lu %lu %i %i %i %i %i %i",
		dest_win->getFrameID(),
		dest_win->getClientID(),
		src_win->getFrameID(),
		(long)pointer_x, (long)pointer_y,
		(long)(r->x + width/2), (long)(- r->y + height/2),
		(long)(r->w), (long)(r->h));
	_ascr->sendFvwmCmd(cmd,0);

	src_win->getRenderer()->rmMatchingSelectedRegions(r);
}


void MetisseDesktop::rmFacade(
	cut c, MetisseWindow *src_win, MetisseWindow *dest_win)
{
	XClientMessageEvent ev;

	float width,height;
	src_win->getSize(&width,&height);

	char cmd[256];
	sprintf(cmd, "Exec metisse-xlib RmFacade %lu %lu %lu %i %i %i %i",
		dest_win->getFrameID(),
		dest_win->getClientID(),
		src_win->getFrameID(),
		c.sx, c.sy, c.sw, c.sh);
	_ascr->sendFvwmCmd(cmd,0);
}

void MetisseDesktop::setFacadeName(MetisseWindow *w, char *name)
{
	char cmd[256];
	sprintf(cmd, "Exec metisse-xlib SetName %lu \"%s\"",
		w->getClientID(), name);
	_ascr->sendFvwmCmd(cmd,0);
}
