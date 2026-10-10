/*
 *
 * main/AScreen.cxx --
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

#include "AScreen.H"

#include <nucleo/utils/AppUtils.H>
#include <nucleo/gl/window/glWindow.H>
#include <nucleo/gl/glUtils.H>
#include <nucleo/image/sink/ImageSink.H>
#include <nucleo/utils/FileUtils.H>
#include <nucleo/image/encoding/Conversion.H>

#include "desktop/MetisseDesktop.H"
#include "fvwmmodule/FvwmModule.H"

#include "AUtils.H"
#include "ExtendedInputs.H"
#ifdef HAVE_SPI
#include "WidgetManager.H"
#endif
#include "Persistence.H"

#include <math.h>
#include <stdio.h>
#include <stdexcept>

#define SR_SNAP_DIST 12

#define CAPTURE_ON_TIMER 0

using namespace nucleo ;


// -------------------------------------------------------------------------
// General set up

void
AScreen::setTitle(const char *title)
{
	compositor->setTitle(title);
}

void
AScreen::setGeometry(unsigned int width, unsigned int height)
{
	_desktopWidth = width;
	_desktopHeight = height;
	if (_force_fullscreen || (width == compositor->getScreenWidth() &&
				  height == compositor->getScreenHeight()))
	{
		compositor->setFullScreen(true);
		_winwidth = compositor->getScreenWidth();
		_winheight = compositor->getScreenHeight();
	}
	else
	{
		compositor->setGeometry(width, height, 100, 100);
		_winwidth = width;
		_winheight = height;
	}
	compositor->setMinMaxSize(_winwidth, _winheight, _winwidth, _winheight);
}


void AScreen::makeCurrent(void)
{
	compositor->makeCurrent();
}

void AScreen::useSoftwareCursor()
{
	_cursor->setSoftwareCursor(true);
}

void AScreen::warpCursor(int x, int y)
{
#if 0
	char cmd[256];

	sprintf(cmd, "WindowId root 1 WarpToWindow "
		"%ip %ip", x, y);
	sendFvwmCmd(cmd, 0);
#else

	int px = x;
	int py = y;
	if (doProjectCursor())
	{
		_x11project(x, y, &px, &py);
	}
	_currX = _prevX = px;
	_currY = _prevY = py;
	#if 0
	std::cerr << "MetisseDesktop::handleCursorPosition: " << x << "," << y
		  << " --> " << px << "," << py << std::endl;
	#endif
	compositor->warpCursor(px, py);
#endif
	if (_cursor)
	{
		_cursor->updatePosition(px-_winwidth/2.0,_winheight/2.0-py);
	}
}

// ---------------------------------------------------------------------------
// Get info

void AScreen::getDesktopSize(unsigned int *width, unsigned int *height)
{
	*width = _desktopWidth;
	*height = _desktopHeight;
}

#if 0
bool AScreen::inPortrait(void)
{
	return _portrait;
}
#endif

bool AScreen::hasStencilBuffer(void)
{
	return !_noStencil;
}

// ---------------------------------------------------------------------------
// "Additional" drawing

void AScreen::addACursor(ACursor *c)
{
	_viewpoint->addDependency(c);
}

void AScreen::removeACursor(ACursor *c)
{
	_viewpoint->removeDependency(c);
}

void AScreen::stroke(bool on, bool replay)
{
	if (on)
	{
		if (_stroke)
		{
			_viewpoint->removeDependency(_stroke);
			delete _stroke;	
		}
		_stroke = new AXorDraw(
			"stroke", _currX - _winwidth/2.0,
			_winheight/2.0 - _currY);
		//std::cerr << "Strok start " << _stroke << std::endl ;
		_viewpoint->addDependency(_stroke);
		_replay_size = 256;
		_replay_x = (int*)malloc(_replay_size * sizeof(int));
		_replay_y = (int*)malloc(_replay_size * sizeof(int));
		_replay_i = 0;
		_replay_x[0] = _currX;
		_replay_y[0] = _currY;
	}
	else if (_stroke)
	{
		_viewpoint->removeDependency(_stroke);
		delete _stroke;
		_stroke = 0;
		if (replay)
		{
			// replay the stroke sequence !
			_replayMotion();
			
		}
		if (_replay_x != 0)
		{
			free(_replay_x);
			_replay_x = 0;
		}
		if (_replay_y != 0)
		{
			free(_replay_y);
			_replay_y = 0;
		}
		_replay_i = 0;
	}
}

void AScreen::_strokeAdd(int x, int y)
{
	if (_stroke == 0)
	{
		return;
	}
	_stroke->add(x-_winwidth/2.0, _winheight/2.0-y);
	_replay_i++;
	if (_replay_i >= _replay_size)
	{
		_replay_size += 256;
		_replay_x = (int*)realloc(
			(void *)_replay_x, _replay_size * sizeof(int));
		_replay_y = (int*)realloc(
			(void *)_replay_y, _replay_size * sizeof(int));
	}
	_replay_x[_replay_i] = x;
	_replay_y[_replay_i] = y;
	//fprintf(stderr,"_strokeAdd leave %i\n",_replay_i);
}

// -----------------------

void AScreen::startVideoCapture(char *uri)
{
	if (_capture) stopVideoCapture() ;

	std::cerr << "AScreen: sending images to " << uri << std::endl ;

	_capture = ImageSink::create(uri) ;
	subscribeTo(_capture) ;
	_capture->start();

#if CAPTURE_ON_TIMER
	_capture_timer = TimeKeeper::create(TimeKeeper::second/10, true) ;
	subscribeTo(_capture_timer) ;
#endif

	compositor->makeCurrent() ;  
}

void AScreen::stopVideoCapture(void)
{
  if (!_capture) return ;

  std::cerr << "AScreen: not recording anymore" << std::endl ;

  unsubscribeFrom(_capture) ;
  delete _capture ;
  _capture = 0 ;

#if CAPTURE_ON_TIMER
  unsubscribeFrom(_capture_timer) ;
  delete _capture_timer ;
  _capture_timer = 0 ;
#endif
}

// -----------------------

void AScreen::showFPS(void)
{
	_perfMeter = new PerfMeter;
	_viewpoint->addDependency(_perfMeter);  
}

void AScreen::saveFrame(void)
{
	_saveFrame = true;
}

void AScreen::showVMouse(int width, int height)
{
	_vMouseMove = 0;
	if (_vMouse == 0)
	{
		
		_vMouse = new VMouse(
			"Virtual Mouse", (_winwidth-width)/2, -_winheight/4,
			width, height);
		_viewpoint->addDependency(_vMouse);
	}
	else
	{
		_viewpoint->removeDependency(_vMouse);
		delete _vMouse;
		_vMouse = 0;
	}
}

// -----------------------------------------------------------------------------
// fvwm
void AScreen::setFvwmModule(FvwmModule *m)
{
	_fvwmModule = m;
}

void AScreen::sendFvwmCmd(const char *cmd, MetisseWindow *w)
{
	if (_fvwmModule != 0)
	{
		_fvwmModule->sendText(cmd, w);
	}
}

void AScreen::sendMetisseTransform(MetisseWindow *win, bool mark)
{
	_fvwmModule->sendMetisseTransform(win, mark);
}

// -----------------------------------------------------------------------------
// Widget Manager wrapper

bool
AScreen::initWidgetManager(void) {
  // std::cerr << "AScreen::initWidgetManager, line " << __LINE__ << std::endl ;
#ifdef HAVE_SPI
	if (_widgetManager) return true ;
	try {
		_widgetManager = new WidgetManager(this, _metisseDesktop);
	} catch (std::runtime_error e) {
		std::cerr << "AScreen::initWidgetManager: " << e.what()
			  << std::endl;
	}
	return _widgetManager!=0;
#else
	return false;
#endif
  // std::cerr << "AScreen::initWidgetManager, line " << __LINE__ << std::endl ;
  return _widgetManager!=0 ;
}

Accessible *AScreen::registerToWidgetManager(MetisseWindow *win)
{
  // std::cerr << "AScreen::registerToWidgetManager, line " << __LINE__ << std::endl ;
#ifdef HAVE_SPI
	if (!initWidgetManager())
	{
		return 0;
	}
	return _widgetManager->registerWindow(win);
#else
	return 0;
#endif
}

void AScreen::unregisterToWidgetManager(MetisseWindow *win)
{
  // std::cerr << "AScreen::unregisterToWidgetManager, line " << __LINE__ << std::endl ;
#ifdef HAVE_SPI
	if (!initWidgetManager())
	{
		return;
	}
	_widgetManager->unregisterWindow(win);
#endif
}


WinRegions::region AScreen::getWidgetExtent(MetisseWindow *win, int x, int y)
{
  // std::cerr << "AScreen::getWidgetExtent, line " << __LINE__ << std::endl ;
	WinRegions::region notfound = WinRegions::region(-1,-1,-1,-1) ;
#ifdef HAVE_SPI
	if (initWidgetManager()) return _widgetManager->getExtent(win,x,y) ;
#endif
	return notfound ;
}

void AScreen::printWidgets(MetisseWindow *win, int verb)
{
  // std::cerr << "AScreen::printWidgets, line " << __LINE__ << std::endl ;
#ifdef HAVE_SPI
	if (initWidgetManager())
	{
		_widgetManager->printWidgets(win,verb);
	}
#endif	
}
std::list<WinRegions::xyregion > AScreen::getWidgetRegionsXY(MetisseWindow *win)
{
  // std::cerr << "AScreen::getWidgetRegionsXY, line " << __LINE__ << std::endl ;
	std::list<WinRegions::region > reg;

#ifdef HAVE_SPI
	if (initWidgetManager()) _widgetManager->getWidgetRegions(win, reg);
#endif

	std::list<WinRegions::xyregion > xyreg;
	for (std::list<WinRegions::region >::iterator it = reg.begin();
		it != reg.end(); it++) {
		GLdouble ox1,oy1,oz,ox2,oy2,x1,x2,y1,y2;;
		unprojectOverWindow(
			win, (int)rint((*it).x), (int)rint((*it).y),
			&ox1, &oy1, &oz);
		unprojectOverWindow(
			win, (int)rint((*it).x+(*it).w),
			(int)rint((*it).y+(*it).h),
			&ox2, &oy2, &oz);
		x2 = (ox2 > ox1)? ox2:ox1;
		x1 = (ox1 < ox2)? ox1:ox2;
		y2 = (oy2 > oy1)? oy2:oy1;
		y1 = (oy1 < oy2)? oy1:oy2;
		xyreg.push_front(WinRegions::xyregion(x1,y1,x2,y2));
	}
	return xyreg;
}

std::list<WinRegions::region > AScreen::getWidgetRegionsX11(MetisseWindow *win)
{
  // std::cerr << "AScreen::getWidgetRegionsX11, line " << __LINE__ << std::endl ;
	std::list<WinRegions::region > reg;

#ifdef HAVE_SPI
	if (initWidgetManager()) _widgetManager->getWidgetRegions(win, reg) ;
#endif

#if 0
	std::list<WinRegions::region > unreg;
	for (std::list<WinRegions::region >::iterator it = reg.begin();
		it != reg.end(); it++) {
		GLdouble ox1,oy1,oz,ox2,oy2,x1,x2,y1,y2;
		unprojectOverWindow(
			win, (int)rint((*it).x), (int)rint((*it).y),
			&ox1, &oy1, &oz);
		unprojectOverWindow(
			win, (int)rint((*it).x+(*it).w),
			(int)rint((*it).y+(*it).h),
			&ox2, &oy2, &oz);
		x2 = (ox2 > ox1)? ox2:ox1;
		x1 = (ox1 < ox2)? ox1:ox2;
		y2 = (oy2 > oy1)? oy2:oy1;
		y1 = (oy1 < oy2)? oy1:oy2;
		unreg.push_front(WinRegions::region(x1,y1,x2-x1,y2-y1));
	}
#endif

	return reg;
}


void AScreen::actionOnWidget(MetisseWindow *win)
{
  // std::cerr << "AScreen::actionOnWidget, line " << __LINE__ << std::endl ;
#ifdef HAVE_SPI
	if (!initWidgetManager())
	{
		return;
	}

	if (_selection == 0 || win != _selection->getWindow() ||
	    win->isRootWindow())
	{
		return;
	}

	//_widgetManager->buildActionMenus(win);
	AWidget *aw =
		_widgetManager->getAWidgetAtPoint(win, _currX, _currY);

	_actionX = _currX;
	_actionY = _currY;
	_actionWin = win;
	if (aw == 0)
	{
		// check for facade
		cut fcut;
		GLdouble ox, oy, oz;
		if (unprojectOverWindow(win, _currX, _currY, &ox, &oy, &oz))
		{
			int x = (int)rint(ox);
			int y = (int)rint(oy);
			sgNode *n = win->getFacadeAtPoint(x, y, &fcut);
			if (n != 0)
			{
				MetisseWindow *w = dynamic_cast<MetisseWindow *>(n);
				sendFvwmCmd("Menu MenuMenusOnFacade", w);
			}
			_actionX = x;
			_actionY = y;
		}
		return;
	}

	AWidgetOrthoZoom *oz = dynamic_cast<AWidgetOrthoZoom *>(aw);
	if (oz != 0)
	{

		//std::cerr << "get an orthozoomer\n";
		sendFvwmCmd("Menu MenuMenusValuatorSpeedOrthoZoom", win);
		delete oz;
		return;
	}

	AWidgetValuator *sb = dynamic_cast<AWidgetValuator *>(aw);
	if (sb != 0)
	{
		sendFvwmCmd("Menu MenuMenusValuatorSpeed", win);
		delete sb;
		return;
	}

	sendFvwmCmd("Menu MenuMenusComboBox", win);
	delete aw;	
#endif
}


void AScreen::setWidgetValuator(MetisseWindow *win, double speed)
{
  // std::cerr << "AScreen::setWidgetValuator, line " << __LINE__ << std::endl ;
#ifdef HAVE_SPI
	if (!initWidgetManager())
	{
		return;
	}

	AWidgetValuator *sb =
		_widgetManager->getValuatorAtPoint(win, _actionX, _actionY);

	if (sb == 0)
	{
		return;
	}
	sb->setSpeed(speed);
	win->setWidgetValuator(sb);
#endif
}

void AScreen::setAllWidgetValuator(MetisseWindow *win, double speed)
{
  // std::cerr << "AScreen::setAllWidgetValuator, line " << __LINE__ << std::endl ;
#ifdef HAVE_SPI
	if (!initWidgetManager())
	{
		return;
	}

	std::list<AWidgetValuator *> vlist =
		_widgetManager->getAllValuators(win);
	std::list<AWidgetValuator *>::iterator iter;
	for (iter = vlist.begin(); iter != vlist.end(); iter++)
	{
		(*iter)->setSpeed(speed);
		win->setWidgetValuator((*iter));
	}
#endif
}

void AScreen::actionOnComboBox(MetisseWindow *win, char *action)
{
  // std::cerr << "AScreen::actionOnComboBox, line " << __LINE__ << std::endl ;
#ifdef HAVE_SPI
	if (!initWidgetManager())
	{
		return;
	}

	AWidget  *wa =
		_widgetManager->getAWidgetAtPoint(win, _actionX, _actionY);

	
	if (wa == 0)
	{
		return;
	}

	char cmd[256];

	if (StrEquals(action, "ToRadioButtonEditor"))
	{
		sprintf(cmd, "Exec metisse-combo2radio-edit %s %s",
			wa->getAppName(), wa->getName());
		sendFvwmCmd(cmd, 0);
	}
	else if (StrEquals(action, "CanadaProvince"))
	{
		sprintf(cmd, "Exec cd /home/olivier/src/cvs-metisse/uifacades/; ./address-caller");
		sendFvwmCmd(cmd, 0);
	}
	else if (StrEquals(action, "ToRadioButton"))
	{
		sprintf(cmd, "Exec metisse-combo2radio %s %s",
			wa->getAppName(), wa->getName());
		sendFvwmCmd(cmd, 0);
	}
	delete wa;
#endif
}

void AScreen::actionOnFacade(MetisseWindow *win, char *action)
{
  // std::cerr << "AScreen::actionOnFacade, line " << __LINE__ << std::endl ;
	cut fcut;
	sgNode *n = _actionWin->getFacadeAtPoint(_actionX, _actionY, &fcut);
	MetisseWindow *w = dynamic_cast<MetisseWindow *>(n);

	std::cerr << "actionOnFacade " << w << "\n";
	if (w == 0)
	{
		return;
	}
	if (StrEquals(action, "Remove"))
	{
		std::cerr << "actionOnFacade Remove\n";
		_actionWin->rmFacade(n, &fcut);
		_metisseDesktop->windowUpdateRequest(w);
		_metisseDesktop->windowUpdateRequest(_actionWin);
	}
	else if (StrEquals(action, "Move"))
	{
		// TODO
	}
}

void AScreen::_checkSelectedIcons(void)
{
#ifdef HAVE_SPI
	if (true || !_fvwmTraceRunning || _selection == 0 ||
	    _selection->getWindow() == 0 || !initWidgetManager())
	{
		return;
	}

	MetisseWindow *win = _selection->getWindow();
	std::list<std::string *> icons;
	std::list<std::string *> paths;
	icons.clear();

	_widgetManager->getSelectedIcons(win, &paths, &icons);

	if (icons.empty())
	{
		return;
	}

	const char *p = "";
	if (paths.begin() != paths.end())
	{
		p = (*paths.begin())->c_str();
	}
	if (win->isEwmhDesktop())
	{
		//std::cerr << "PATH is Desktop\n";
		p = "Desktop";
	}

	char *cmd = (char *)malloc(strlen(p) + 1 + 23 + 19);

	sprintf(cmd,"SendToModule FvwmTrace IconsSelectedStart %s",p);
	sendFvwmCmd(cmd,win);

	for (std::list<std::string *>::iterator it=icons.begin();
	     it!= icons.end(); it++)
	{
		free(cmd);
		cmd = (char *)malloc(23 + 14 + (*it)->length() + 1);
		sprintf(
			cmd,"SendToModule FvwmTrace IconsSelected %s",
			(*it)->c_str());
		sendFvwmCmd(cmd,win);
		//std::cerr << "SELECTED ICON:" << (*it)->c_str() << "\n";
		delete (*it);
	}

	free(cmd);
	for (std::list<std::string *>::iterator it=paths.begin();
	     it!= paths.end(); it++)
	{
		delete (*it);
	}

	sendFvwmCmd("SendToModule FvwmTrace IconsSelectedStop", win);
#endif
}

// ---------------------------------------------------------------------------
// Selection and mouse events


#define DEBUG_SELECTION 0

void AScreen::_myPointerEvent(int x, int y, unsigned long press_mask)
{
	GLdouble ox, oy, oz;

	if (_swallow_release)
	{
		return;
	}

	if (_over_selection && _over_selection->getWindow() && press_mask > 0 &&
	    unprojectOverWindow(
		    _over_selection->getWindow(), x, y, &ox, &oy, &oz))
	{
		_over_selection->pointerEvent(ox,oy, _button_mask);
		usleep(40000); // 40 ms
		_over_selection->pointerEvent(ox,oy, 0);
		usleep(40000);
	}

	WindowRenderer *sel = _selection;

	if (false && _inDragAndDrop && _managed_selection &&
	    _managed_selection->getWindow())
	{
		sel = _managed_selection;
	}
	if (sel && sel->getWindow() &&
	    unprojectOverWindow(sel->getWindow(), x, y, &ox, &oy, &oz))
	{
		#if 0
		if (_viewpoint->graphChanged())
			std::cerr << "AScreen.cxx, line " << __LINE__
				  << ": graph changed"
				  << std::endl ;
		#endif

		// to make tooltips output only
		sel->pointerEvent(ox,oy, _button_mask);

		#if 0
		// need much more work!!
		if (sel->getWindow()->isEwmhDesktop() &&
		    (_button_mask & (1<<(1-1))) &&
		    (abs(_prevX-_pressX) + abs(_prevY-_pressY) > 4)
		    && !_inTmpRestack)
		{
			tmpRestack(sel->getWindow(), true, 0);
		}
		#endif
		#if 0
		if (_viewpoint->graphChanged())
			std::cerr << "AScreen.cxx, line " << __LINE__
				  << ": graph changed" << std::endl ;
		#endif
	}
	else if (sel)
	{
		unproject(
			x, y, _viewpoint, _selectionBuffer,
			_selectionBufferSize, &ox, &oy, &oz) ;
		sel->pointerEvent(ox,oy, _button_mask);
	}
	else
	{
		int rx,ry;
		bool sendEvent;

		char cmd[256];
		
		cmd[0] = '\0';

		sendEvent = root->getPointerEvent(
			x, y, _button_mask, &rx, &ry, cmd);
		if (sendEvent)
		{
			_metisseDesktop->rootPointerEvent(rx, ry, _button_mask);
		}
		else if (_button_mask != 0)
		{
			_swallow_release = true;
		}
		if (cmd[0] != '\0')
		{
			_fvwmModule->sendText(cmd, 0);
		}
	}
}

// we need our own PickClosest for shaped window and because we use our own
// stacking (no depth test!)
int AScreen::_myPickClosest(
	int x, int y, GLuint *sbuffer, GLuint *over_sbuffer, GLuint *over_size,
	GLuint *managed_sbuffer, GLuint *managed_size, GLuint size)
{
	GLuint *tmpbuffer = new GLuint [sgViewpoint::glPickingBufferSize] ;

	int hits = _viewpoint->pickAll(
		x,y,tmpbuffer,sgViewpoint::glPickingBufferSize) ;
	if (hits <= 0)
	{
		delete [] tmpbuffer ;
		return 0 ;
	}

	 GLuint *selection=0, selectionSize=0 ;
	 GLuint *over_selection=0, over_selectionSize=0 ;
	 GLuint *managed_selection, managed_selectionSize=0 ;
	 GLdouble minimum=0, over_minimum;
	 int maxStackZ = -1;
	 int over_maxStackZ = -1;
	 int managed_maxStackZ = -1;

	 GLint i,j;
	 GLuint *hit = tmpbuffer;
	 for (i=0;  i < hits; i++)
	 {
		 GLuint numNames = *hit++;
		 GLdouble minWinZ = *hit++ / 4294967295.0 ;
		 // GLdouble maxWinZ = *(hit+2) / 4294967295.0 ;
		 hit++;

		 if (numNames == 0)
		 {
			 // is this possible? man glSelectBuffer
			 hit++;
			 continue;
		 }

		 GLuint *names;
		 names = hit;
		 hit += numNames-1;

		 WindowRenderer *sel =
			 dynamic_cast<WindowRenderer *>(
				 sgNode::lookupId(*hit++));

		 if (!sel || !sel->getWindow())
		 {
			 continue;
		 }

		 int stackZ = sel->getWindow()->getStackingPos();

		 if (i==0 || stackZ >= maxStackZ || selectionSize == 0)
		 {
			 // ckeck
		   
			 GLdouble ox, oy, oz;
			 for (GLuint n=0; n<size && n<numNames; n++)
			 {
				 sbuffer[n] = names[n];
			 }
			 unproject(
				 x, y, minWinZ, _viewpoint, sbuffer,
				 numNames, &ox, &oy, &oz);
			 if (sel->getWindow()->isAToolGlass())
			 {
				 if (stackZ >= over_maxStackZ ||
				     over_selectionSize == 0)
				 {
					 over_selection = names;
					 over_selectionSize = numNames;
					 over_minimum = minWinZ;
					 over_maxStackZ = stackZ;
				 }
				 // std::cerr << "get a Tool Glass\n";
			 }
			 else if (sel->isInsideWindow(
					  (int)rint(ox),(int)rint(oy))) 
			 {
				 selection = names ;
				 selectionSize = numNames ;
				 minimum = minWinZ ;
				 maxStackZ = stackZ;
				 if (!sel->getWindow()->isUnmanaged())
				 {
					 managed_selection = names ;
					 managed_selectionSize = numNames ;
				 }
			 }
		 }
	 }

	 for (GLuint n=0; n<size && n<selectionSize; n++)
	 {
		 sbuffer[n] = selection[n];
	 }
	 for (GLuint n=0; n<size && n<over_selectionSize; n++)
	 {
		 over_sbuffer[n] = over_selection[n];
	 }
	 for (GLuint n=0; n<size && n<managed_selectionSize; n++)
	 {
		 managed_sbuffer[n] = managed_selection[n];
	 }
	 delete [] tmpbuffer ;

	 *over_size = over_selectionSize;
	 *managed_size = managed_selectionSize;
	 return selectionSize ;    
}

void AScreen::_x11project(int x, int y, int *px, int *py)
{
	if (_selection)
	{
		GLdouble fx, fy, fz;
		float sw,sh;
		float sx, sy;
		_selection->getWindow()->getSize(&sw,&sh);
		_selection->getWindow()->getRealPosition(&sx,&sy);
		// object coordinates
		float ox, oy;
		ox = (float)x - sx - sw/2;
		oy = - (float)y + sy + sh/2;
		_viewpoint->project(
			ox, oy, 0.0, _selectionBuffer, _selectionBufferSize,
			&fx, &fy, &fz);
		// window coor to X11 coor
		*px = (int)rint(fx);
		*py = (int)rint(_winheight - fy);
	}
	else
	{
		*px = x;
		*py = y;
	}
}

void AScreen::_x11projectOverWindow(
	MetisseWindow *win, int x, int y, int *px, int *py)
{
	GLdouble fx,fy,fz;
	projectOverWindow(win, x, y, &fx, &fy, &fz);
	// window coor to X11 coor
	*px = (int)rint(fx);
	*py = (int)rint(_winheight - fy);
}

bool AScreen::_isOverWinDecor(MetisseWindow *win, int x, int y)
{
	if (win == 0 || win->isUnmanaged())
	{
		return false;
	}

	GLdouble ox, oy, oz;
	if (!unprojectOverWindow(win, x, y, &ox, &oy, &oz))
	{
		return false;
	}
	return win->isOverDecor((int)rint(ox), (int)rint(oy));
}

MetisseWindow *AScreen::getSelectedWindow(void)
{
	if (_selection)
	{
		return _selection->getWindow();
	}

	return 0;
}

MetisseWindow *AScreen::getSelectedFacade(cut *fcut)
{
	if (_selection == 0 || _selection->getWindow() == 0 ||
	    !_selection->getWindow()->hasAFacade())
	{
		return 0;
	}

	MetisseWindow *sw = _selection->getWindow();
	MetisseWindow *win = 0;

	GLdouble ox, oy, oz;
	if (unprojectOverWindow(sw, _currX, _currY, &ox, &oy, &oz))
	{
		int x = (int)rint(ox);
		int y = (int)rint(oy);
		win = dynamic_cast<MetisseWindow *>(
			sw->getFacadeAtPoint(x,y,fcut));
	}
	return win;
}

bool AScreen::doPseudoSelection(void)
{
	return _inPseudoSelectionMode;
}

void AScreen::forceDoublePointerEvent(void)
{
	_forceDoublePointerEvent = true;
}

void AScreen::_dragResetSelection(bool sendEvent)
{
	WindowRenderer *selection;
	GLuint selectionBuffer[256];
	int selectionBufferSize = 0;
	WindowRenderer *over_selection;
	GLuint over_selectionBuffer[256];
	GLuint over_selectionBufferSize;
	GLuint managed_selectionBuffer[256];
	GLuint managed_selectionBufferSize;
	WindowRenderer *old_managed_selection = _managed_selection;

	selectionBufferSize =
		_myPickClosest(
			_currX,_currY, selectionBuffer,
			over_selectionBuffer, &over_selectionBufferSize,
			managed_selectionBuffer, &managed_selectionBufferSize,
			256);

	if (_managed_selectionBufferSize)
	{
		_managed_selection = dynamic_cast<WindowRenderer *>(
			sgNode::lookupId(
				_managed_selectionBuffer[
					_managed_selectionBufferSize-1]));
	}
	else
	{
		_managed_selection = 0;
	}
	#if 0
	if (_managed_selection != old_managed_selection)
	{
		#if 1
		std::cerr << "DRAG New Managed selection: "
			  << _inDragAndDrop << " "
			  << _managed_selection << " "
			  << _selection << " "
			  << _current_dnd_win
			  << std::endl ;
		#endif
	}
	#endif
	if (selectionBufferSize)
	{
		selection = dynamic_cast<WindowRenderer *>(
			sgNode::lookupId(
				selectionBuffer[selectionBufferSize-1]));
	}
	else
	{
		if (sendEvent)
		{
			if (_forceDoublePointerEvent)
			{
				_myPointerEvent(_currX+1, _currY);
				_forceDoublePointerEvent = false;
			}
			_myPointerEvent(_currX, _currY);
		}
		return;
	}

	if (selection->getWindow()->isUnmanaged() &&
	    !selection->getWindow()->isRootWindow() &&
	    selection != _selection)
	{
		// should reset the selection
		// Do it again at this should happen not too
		// ofthen this is ok ...
		//std::cerr << "DRAG resetSelection\n";
		resetSelection(sendEvent);
		return;
	}
	#if 0
	else
	{
		std::cerr << "DRAG do not resetSelection\n";
	}
	#endif
	if (sendEvent)
	{
		if (_forceDoublePointerEvent)
		{
			_myPointerEvent(_currX+1, _currY);
			_forceDoublePointerEvent = false;
		}
		_myPointerEvent(_currX, _currY);
	}
}

void AScreen::resetSelection(bool sendEvent)
{
	WindowRenderer *oldSelection = _selection;
	WindowRenderer *old_managed_selection = _managed_selection;
	_selectionBufferSize = 0;
	_over_selectionBufferSize = 0;

#if  DEBUG_SELECTION && 0
	std::cerr << "Check Selection" << std::endl;
#endif

	if (_wmResize || _miIn)
	{
		// should (almost) never happen ...
		//std::cerr << "_wmResize Check Selection!!!" << std::endl;
		return;
	}
	_selectionBufferSize =
		_myPickClosest(
			_currX,_currY, _selectionBuffer, _over_selectionBuffer,
			&_over_selectionBufferSize, _managed_selectionBuffer,
			&_managed_selectionBufferSize, 256);

	if (_over_selectionBufferSize)
	{
		_over_selection = dynamic_cast<WindowRenderer *>(
			sgNode::lookupId(
				_over_selectionBuffer[
					_over_selectionBufferSize-1]));
	}
	else
	{
		_over_selection = 0;
	}

	if (_managed_selectionBufferSize)
	{
		// FIXME: get acrash here (killing a facade in pager mode 
		_managed_selection = dynamic_cast<WindowRenderer *>(
			sgNode::lookupId(
				_managed_selectionBuffer[
					_managed_selectionBufferSize-1]));
	}
	else
	{
		_managed_selection = 0;
	}
	#if 1
	if (_managed_selection != old_managed_selection)
	{
		#if 0
		std::cerr << "New Managed selection: "
			  << _managed_selection << " "
			  << _selection << " "
			  << _current_dnd_win
			  << std::endl ;
		#endif
		if (_inDragAndDrop)
		{
			if (_selection && _selection != _managed_selection &&
			    _selection->getWindow()->isUnmanaged())
			{
				//std::cerr << "Attach to Sel\n";
				_current_dnd_win =
					_selection->getWindow();
			}
			else if (_current_dnd_win == 0)
			{
				//std::cerr << "Attach to Un\n";
				_current_dnd_win = root->getUnmanagedWindow();
			}
			if (_current_dnd_win && _managed_selection)
			{
				#if 0
				std::cerr << "Attach "
					  << _managed_selection << " "
					  << _selection << " "
					  << _current_dnd_win
					  << "\n";
				#endif
				_current_dnd_win->setUnmanagedFor(
					_managed_selection->getWindow());
				_current_dnd_win->setTmpAlpha(0.5);
			}
		}
	}
	#endif

	if (_selectionBufferSize)
	{
		_selection = dynamic_cast<WindowRenderer *>(
			sgNode::lookupId(
				_selectionBuffer[_selectionBufferSize-1]));
	}
	else
	{
		_selection = 0;	
	}

	if (_over_selection != 0 && 
	    (_selection == 0 || _selection->getWindow()->isRootWindow() ||
	     _selection->getWindow()->isEwmhDesktop() ||
	     _isOverWinDecor(_over_selection->getWindow(), _currX, _currY)))
	{
		_selectionBufferSize = _over_selectionBufferSize;
		for (GLuint n=0; n<_over_selectionBufferSize; n++)
		{
			_selectionBuffer[n] = _over_selectionBuffer[n];
		}
		_selection = _over_selection;
		_over_selection = 0;
		_over_selectionBufferSize = 0;
	}

	bool isnewselection = false;
	if (oldSelection != _selection)
	{
#if DEBUG_SELECTION
		std::cerr << "New selection: "
			  << _selection << " "
			  << _current_dnd_win
			  << std::endl ;
#endif
		char cmd[128];
		
		sprintf(cmd, "MetisseSelectedWindow 0x%lx\n",
			(_selection != 0)?
			_selection->getWindow()->getClientID():0);
		_fvwmModule->sendText(cmd, 0);
		isnewselection = true;
		if (oldSelection)
		{
			oldSelection->pointerLeave();
		}
	}

	if (sendEvent)
	{
		if (_forceDoublePointerEvent) // || isnewselection)
		{
			// FIXME: this should be fixed in the serveur ... or not
			_myPointerEvent(_currX+1, _currY);
			_forceDoublePointerEvent = false;
		}
		_myPointerEvent(_currX, _currY);
	}
}

void AScreen::resetSelection(int x, int y)
{
	_currX = x;
	_currY = y;
	resetSelection();
	_prevX = x;
	_prevY = y;
}

void AScreen:: scheduleResetSelection(void)
{
	_resetSelectionScheduled = true;
}


// ---------------------
// Send event and unproject over a window which is not necessarily the selection
// (e.g., a window under the selection).
// (not sure that this can work with a window with some XY rotation).

void AScreen::pointerEventOverWindow(MetisseWindow *win, int x, int y)
{
	GLdouble ox, oy, oz;

	if (unprojectOverWindow(win, x, y, &ox, &oy, &oz))
	{
#if 0
		std::cerr << "mpeow: " << x << " " << y << " / "
			  << ox << " " << oy << " " << win << std::endl;
#endif
#if 0
		if (fabs(ox) > 2000 || fabs(oy) >  2000)
		{
			_metisseDesktop->rootPointerEvent(x, y, _button_mask);
			return;
		}
#endif
		win->getRenderer()->pointerEvent(ox, oy, _button_mask);
	}
	else if (unprojectOverWindow(root->getRootWindow(), x, y, &ox, &oy, &oz))
	{
		win->getRenderer()->pointerEvent(ox, oy, _button_mask);
		//std::cerr << "mpeow: fail on win" << std::endl;
	}
	else
	{
		_metisseDesktop->rootPointerEvent(x, y, _button_mask);
		//std::cerr << "mpeow: fail" << std::endl;
	}
}

bool AScreen::unprojectOverWindow(
	MetisseWindow *win, int x, int y, GLdouble *ox, GLdouble *oy,
	GLdouble *oz)
{
	// should found the z. a simpler method? not sure that this works
	// with window with some XY rotation.

	_inPseudoSelectionMode = true;
	float z = 0;
	GLuint *tmpbuffer = new GLuint [sgViewpoint::glPickingBufferSize] ;
	int hits = _viewpoint->pickAll(
		x,y,tmpbuffer,sgViewpoint::glPickingBufferSize) ;
	_inPseudoSelectionMode = false;

	if (!hits)
	{
		delete [] tmpbuffer ;
		//std::cerr << "uow: fail" << std::endl;
		return false;
	}

	bool found = false;
	for (GLuint *hit=tmpbuffer, i=0; (GLint)i<hits; i++)
	{
		 GLuint numNames = *hit ;
		 GLuint *names = hit+3 ;
		 MetisseWindow *w = dynamic_cast<MetisseWindow *>(
			 sgNode::lookupId(names[numNames-2]));
		 if (w == win)
		 {
			 z = *(hit+1) / 4294967295.0 ;
			 found = true;
			 break;
		 }
		 hit += 3+numNames ;
	}
	delete [] tmpbuffer ;

	if (!found)
	{
		//std::cerr << "uow: not found" << std::endl;
		return false;
	}

	// FIXME, this should not be hard coded
	GLuint selBuffer[3] ;
	GLuint selSize=3;

	selBuffer[2] = (win->getRenderer())->getId();
	selBuffer[1] = (win)->getId();
	selBuffer[0] = (root)->getId();

	unproject(x, y, z, _viewpoint, selBuffer, selSize, ox, oy, oz) ;
#if 0
	std::cerr << "uow: " << x << " " << y << " " << z << " / "
			  << *ox << " " << *oy << " " << *oz << std::endl;
#endif
	return true;
}

bool AScreen::projectOverWindow(
	MetisseWindow *win, int x, int y, GLdouble *px, GLdouble *py, GLdouble *pz)
{
	float sw,sh;
	float sx, sy;
	win->getSize(&sw,&sh);
	win->getRealPosition(&sx,&sy);
	// object coordinates
	float ox, oy;
	ox = (float)x - sx - sw/2;
	oy = - (float)y + sy + sh/2;

	GLuint selBuffer[3] ;
	GLuint selSize=3;
	selBuffer[2] = (win->getRenderer())->getId();
	selBuffer[1] = (win)->getId();
	selBuffer[0] = (root)->getId();

	_viewpoint->project(
			ox, oy, 0.0, selBuffer, selSize, px, py, pz);
	return true;
}


unsigned long AScreen::getButtonMask(void)
{
	return _button_mask;
}

void AScreen::getPointerPosition(int *x, int *y)
{
	*x = _currX;
	*y = _currY;
}

void AScreen::getPrevPointerPosition(int *x, int *y)
{
	*x = _prevX;
	*y = _prevY;
}



bool AScreen::doProjectCursor(void)
{
	bool r = _doProjectCursor;
	_doProjectCursor = true;

	return r;
}

void AScreen::fixCursorPos(bool start, MetisseWindow *metisseWin)
{
	static MetisseWindow *sWin = 0;
	static GLdouble sx, sy, sz;

	if (start)
	{
		if (metisseWin != 0)
		{
			// ok
		}
		else if (_managed_selection && _managed_selection->getWindow())
		{
			metisseWin = _managed_selection->getWindow();
		}
		else if (_selection && _selection->getWindow())
		{
			metisseWin = _selection->getWindow();
		}
		else
		{
			sWin = 0;
			return;
		}
		if (unprojectOverWindow(
			    metisseWin, _currX, _currY, &sx, &sy, &sz))
		{
			float w, h, x, y;

			sWin = metisseWin;
			sWin->getSize(&w,&h);
			sWin->getRealPosition(&x,&y);

			// x11 coordinate
			sx = (sx + w/2.0 + x);
			sy = (-sy + h/2.0 + y) - 1;

			//std::cerr << "start: " << sx << " " << sy << "\n";
		}
		else
		{
			sWin = 0;
		}
	}
	else if (sWin)
	{
		int px,py;
		char cmd[256];

		_x11projectOverWindow(
			sWin, (int)rint(sx), (int)rint(sy), &px, &py);
		if (px < 0)
		{
			px = 0;
		}
		else if (px >= _desktopWidth)
		{
			px = _desktopWidth-1;
		}
		if (py < 0)
		{
			py = 0;
		}
		else if (py >= _desktopHeight)
		{
			py = _desktopHeight-1;
		}
		
		//std::cerr << "stop: " << px << " " << py << "\n";
		//sprintf(cmd, "WindowId root 1 WarpToWindow %ip %ip",px, py);
		//sendFvwmCmd(cmd, 0);
		_doProjectCursor = false;
		warpCursor(px,py);
		sWin = 0;
	}
}

int AScreen::obc2x11scX(GLfloat x, GLfloat hw, GLfloat rx)
{
	return (int)(rint)((x + hw + rx));
}

int AScreen::obc2x11scY(GLfloat y, GLfloat hh, GLfloat ry)
{
	return (int)(rint)((-y + hh + ry) - 1.0);
}

// --------------------------------------------------------------------------
//
int AScreen::extInputSendEvents(int x, int y, char button_mask, int num)
{
	static int sx = _currX;
	static int sy = _currY;
	static int inExtEvents = 0;

	if((int)_button_mask != 0 ||
	   (inExtEvents != 0 && inExtEvents != num))
	{
		return 0;
	}

	if ((int)button_mask != 0)
	{
		inExtEvents = num;
		sx = _currX;
		sy = _currY;
	}
	else
	{
		inExtEvents = 0;
	}

	_currX = x;
	_currY = y;
	_button_mask = button_mask;
	resetSelection();
	_button_mask = (char)0;
	
	if ((int)button_mask == 0)
	{
		_currX = sx;
		_currY = sy;
		_button_mask = (char)0;
		resetSelection();
	}
	
	_prevX = _currX;
	_prevY = _currY;
	return 1;
}

void AScreen::configureExtInput(MetisseWindow *win, char *name, char *opt)
{
	if (_extInputs == 0)
	{
		return;
	}

	_extInputs->configure(win, name, opt);
}

// called from MetisseWindow::buildSpecialMenu
bool AScreen::generateExtInputWindowMenu(MetisseWindow *win)
{
	if (_extInputs == 0)
	{
		return false;
	}

	return _extInputs->generateWindowMenus(win);
}

// ---------------------------------------------------------------------------
// replay
void AScreen::_replayMotion(void)
{
	int i;
	unsigned long bm = _button_mask;

	//fprintf(stderr,"_replayMotion %i\n",_replay_i);
	if (_replay_i <= 0)
	{
		return;
	}

	// for now we replay a "drag"

	// re pass the press
	_button_mask = 0;
	_currX = _replay_x[0];
	_currY = _replay_y[0];
	resetSelection();
	_prevX =  _replay_x[0];
	_prevY =  _replay_y[0];
	_button_mask = bm;

	// replay the motion
	for(i = 0; i < _replay_i; i++)
	{
		usleep(4000);
		_currX = _replay_x[i];
		_currY = _replay_y[i];
		if (_button_mask &&_selection)
		{
			_dragResetSelection(true);
		}
		else
		{
			resetSelection();
		}
		_prevX =  _replay_x[i];
		_prevY =  _replay_y[i];
	}
}

// --------------------------------------------------------------------------
// Window manager operations

// this move is not used by "default"
void AScreen::wmMove(MetisseWindow *w, bool start)
{
	if (start)
	{
		w->setTmpAlpha(0.5);
		_wmMove = true;
		_opWin = w;
#if 0
		GLdouble ox, oy, oz;
		unprojectOverWindow(_opWin, _prevX, _prevY, &ox, &oy, &oz);
		float rx,ry,rh,rw;
		_opWin->getRealPosition(&rx, &ry);
		_opWin->getSize(&rw, &rh);

		float realx = ox + rw/2.0 + rx;
		float realy = -oy + rh/2.0 + ry;
		GLdouble  px, py, pz;
		unprojectOverWindow(
			root->getRootWindow(), _prevX, _prevY, &px, &py, &pz);
		float rootx = (int)rint(px + _winwidth/2.0);
		float rooty = (int)rint(-py + _winheight/2.0);
		_opsDx = (int)rint(realx - rootx);
		_opsDy = (int)rint(realy - rooty);
#endif
	}
	else
	{
		w->resetAlpha();
		_wmMove = false;
		_opWin = 0;
		resetSelection();
	}
}

void AScreen::_interactiveMove(int ex, int ey)
{
	pointerEventOverWindow(_opWin, ex,  ey);
	_prevX = ex;
	_prevY = ey;
}

bool AScreen::inInteractiveMove(void)
{
	return _wmMove;
}

void AScreen::setResizeTransparency(float alpha)
{
	_resizeTransparency = alpha;
}

void AScreen::wmResize(MetisseWindow *w, bool start)
{
	if (start)
	{
		w->setTmpAlpha(_resizeTransparency);
		_wmResize = true;
		_opWin = w;
	}
	else
	{
		w->resetAlpha();
		_wmResize = false;
		_opWin = 0;
		resetSelection();
		//std::cerr << "wmresize call "
		//	  << "persistence_window_check\n";
		persistence_window_check(w, false, this);
	}
}

void AScreen::_interactiveResize(int ex, int ey)
{
	pointerEventOverWindow(_opWin, ex,  ey);
	_prevX = ex;
	_prevY = ey;
}

bool AScreen::inInteractiveResize(void)
{
	return _wmResize;
}

// ------------------------------
void AScreen::setTmpRestackMode(restack_mode_t trm)
{
	_tmpRestackMode = trm;
}

void AScreen::tmpRestack(MetisseWindow *w, bool start, unsigned long op)
{

	if (!_inDragAndDrop && start)
	{
		if (_tmpRestackMode == TMP_RESTACK_MODE_FOLD)
		{
			root->foldOverWindows(w, true);
		}
		else if (_tmpRestackMode == TMP_RESTACK_MODE_SCALE)
		{
			root->scaleOverWindows(w, true);
		}
		else // TMP_RESTACK_MODE_ROTATE
		{
			// TODO
		}
		_inTmpRestack = true;
		_restackWin = w;
	}
	else if (_inTmpRestack)
	{
		if (_tmpRestackMode == TMP_RESTACK_MODE_FOLD)
		{
			root->foldOverWindows(_restackWin, false);
		}
		else if (_tmpRestackMode == TMP_RESTACK_MODE_SCALE)
		{
			root->scaleOverWindows(_restackWin, false);
		}
		else // TMP_RESTACK_MODE_ROTATE
		{
			// TODO
		}
		_inTmpRestack = false;
	}
}

// ------------------------------

void AScreen:: schedulePagerModeOff(void)
{
	_pagerModeOffScheduled = true;
}

void AScreen::windowDestroyed(MetisseWindow *win)
{
	if (_extInputs != 0)
	{
		_extInputs->windowDestroyed(win);
	}
	if (_widgetManager)
	{
		unregisterToWidgetManager(win);
	}
	if (_current_dnd_win && _current_dnd_win == win)
	{
		_current_dnd_win = 0;
	}
	// FIXME: should do something for the _opWin!
}

// ----------------------------------------------------------------------------
// compositor operation

// ------------------------------------
// window region selection

void AScreen::setRegionSelectionSnapDist(int val)
{
	_srSnapDist = val;
}

void AScreen::selectRegion(MetisseWindow *win, int x, int y, bool add, bool snap)
{
	GLdouble ox, oy, oz ;
	int xx, yy;
	MetisseWindow *sw = 0;

	if (_selection && _selection->getWindow())
	{
		sw = _selection->getWindow();
	}

	if (win->isAFacadeWindow() || win->getDuplicateFor() != 0 ||
	    (sw && (sw->isAFacadeWindow() || sw->getDuplicateFor() != 0)))
	{
		return;
	}

	_opWin = win;
	_x11projectOverWindow(win, x, y, &xx, &yy);
	_ssr_x = x;
	_ssr_y = y;
	_srSnap = snap;

	unprojectOverWindow(
		_opWin, xx, yy, &ox, &oy, &oz) ;
	if (!add)
	{
		_opWin->getRenderer()->rmSelectedRegions();
	}

	_opWin->getRenderer()->selectRegion(ox, oy, true);
	_selectingRegion = true;

	_srKeyCount = 0;
	_widgetRegs.clear();
	_widgetRegs = getWidgetRegionsX11(_opWin);
}

void AScreen::_inSelectRegion(int x, int y)
{
	GLdouble ox, oy, oz;

	if (!_srSnap || _widgetRegs.size() == 0)
	{
		int px, py;
		_x11projectOverWindow(_opWin, x, y, &px, &py);
		// FIXME??
		unprojectOverWindow(_opWin, px,  py, &ox, &oy, &oz);
		_opWin->getRenderer()->selectRegion(ox, oy, true);
		return;
	}

	int px, py,x1,x2,y1,y2;
	//_x11projectOverWindow(_opWin, x, y, &px, &py);
	px = x;
	py = y;
	x2 = (px > _ssr_x)? px:_ssr_x;
	y2 = (py > _ssr_y)? py:_ssr_y;
	x1 = (px < _ssr_x)? px:_ssr_x;
	y1 = (py < _ssr_y)? py:_ssr_y;
	
	int x1m = x1, x2m = x2, y1m = y1, y2m = y2;
	int dx1m = -1, dx2m = -1, dy1m = -1, dy2m = -1;
	int d1,d2;
	std::list<WinRegions::region >::iterator it;
	int i = 0;
	for (it = _widgetRegs.begin(); it != _widgetRegs.end(); it++)
	{
		// x1, y1 snap
		#if 0
		std::cerr << _srSnapDist << " " << x1 << " " << y1 << " "
			  << x2 << " " << y2 << " "
			  <<  (*it).x << " " << (*it).y  << " "
			  <<  (*it).x + (*it).w << " " << (*it).y + (*it).h 
			  << std::endl;
		#endif
		if ((y1 >= (*it).y && y1 <= (*it).y + (*it).h) ||
		    (y2 >= (*it).y && y2 <= (*it).y + (*it).h) ||
		    ((*it).y >= y1 && (*it).y <= y2) ||
		    ((*it).y+(*it).h >= y1 && (*it).y+(*it).h <= y2))
		{
			// y segments intersect do x snaping
			//
			d1 = abs((int)(*it).x - x1);
			d2 = abs((int)((*it).x+(*it).w) - x1);
			int sdx = _srSnapDist;
			if ((*it).w/2 < sdx)
			{
				sdx = (int)((*it).w/2) -1;
			}
			if (d1 < d2 && d1 <= sdx && (dx1m == -1 || d1 < dx1m))
			{
				x1m = (int)(*it).x;
				dx1m = d1;
			}
			else if (d2 <= sdx &&  (dx1m == -1 || d2 < dx1m))
			{
				dx1m = d2;
				x1m = (int)((*it).x + (*it).w);
			}
			//
			d1 = abs((int)(*it).x - x2);
			d2 = abs((int)((*it).x+(*it).w) - x2);
			sdx = _srSnapDist;
			if ((*it).w/2 < sdx)
			{
				sdx = (int)((*it).w/2) -1;
			}
			if (x1m != (int)(*it).x && d1 < d2 && d1 <= sdx &&
			    (dx2m == -1 || d2 < dx2m))
			{
				x2m = (int)(*it).x;
				dx2m = d1;
			}
			else if (x1m != (int)((*it).x + (*it).w) &&
				 d2 <= sdx &&  (dx2m == -1 || d2 < dx2m))
			{
				dx2m = d2;
				x2m = (int)((*it).x + (*it).w);
			}
		}
		if ((x1 >= (*it).x && x1 <= (*it).x + (*it).w) ||
		    (x2 >= (*it).x && x2 <= (*it).x + (*it).w) ||
		    ((*it).x >= x1 && (*it).x <= x2) ||
		    ((*it).x+(*it).w >= x1 && (*it).x+(*it).w <= x2))
		{
			// x segments intersect do y snaping
			//
			d1 = abs((int)(*it).y - y1);
			d2 = abs((int)((*it).y+(*it).h) - y1);
			int sdy = _srSnapDist;
			if ((*it).w/2 < sdy)
			{
				sdy = (int)((*it).w/2) -1;
			}
			if (d1 < d2 && d1 <= sdy && (dy1m == -1 || d1 < dy1m))
			{
				y1m = (int)(*it).y;
				dy1m = d1;
			}
			else if (d2 <= sdy &&  (dy1m == -1 || d2 < dy1m))
			{
				dy1m = d2;
				y1m = (int)((*it).y + (*it).h);
			}
			//
			d1 = abs((int)(*it).y - y2);
			d2 = abs((int)((*it).y+(*it).h) - y2);
			sdy = _srSnapDist;
			if ((*it).w/2 < sdy)
			{
				sdy = (int)((*it).w/2) -1;
			}
			if (y1m != (int)(*it).y && d1 < d2 && d1 <= sdy &&
			    (dy2m == -1 || d1 < dy2m))
			{
				y2m = (int)(*it).y;
				dy2m = d1;
			}
			else if (y1m != (int)((*it).y + (*it).h) &&
				 d2 <= sdy &&  (dy2m == -1 || d2 < dy2m))
			{
				dy2m = d2;
				y2m = (int)((*it).y + (*it).h);
			}
		}
	}
	GLdouble ox1,ox2,oy1,oy2;
	if (!unprojectOverWindow(_opWin, x1m,  y1m, &ox1, &oy1, &oz))
	{
		return;
	}
	_opWin->getRenderer()->selectRegion(ox, oy, true);
	if (!unprojectOverWindow(_opWin, x2m,  y2m, &ox2, &oy2, &oz))
	{
		return;
	}
	_opWin->getRenderer()->selectRegion(ox1, oy1, ox2, oy2);
}

// ----------------------

void AScreen::fold(MetisseWindow *win, int x, int y)
{
	GLdouble ox, oy, oz ;
	int xx, yy;
	_opWin = win;
	_x11projectOverWindow(win, x, y, &xx, &yy);
	unprojectOverWindow(
		_opWin, xx, yy, &ox, &oy, &oz) ;
	_opWin->getRenderer()->fold((int)ox,(int)oy, true);
	_folding = true;
}

void AScreen::foldByCmd(
	MetisseWindow *win, int x, int y, float alpha, float reduction)
{
	GLdouble ox, oy, oz, fx, fy, fz;
	int xx, yy;
	//_x11projectOverWindow(win, x, y, &xx, &yy);
	//unprojectOverWindow(
	//	win, xx, yy, &ox, &oy, &oz) ;
	//projectOverWindow(win, x, y, &fx, &fy, &fz);
	unprojectOverWindow(
		win, (int)x, (int)y, &ox, &oy, &oz) ;
	win->getRenderer()->fold((int)ox,(int)oy,true,false,alpha,reduction);
}

void AScreen::foldNonInteractive(
	MetisseWindow *win, int x, int y, float alpha, bool anim)
{
	GLdouble ox, oy, oz ;
	int xx, yy;
	_x11projectOverWindow(win, x, y, &xx, &yy);
	unprojectOverWindow(
		win, xx, yy, &ox, &oy, &oz) ;
	win->getRenderer()->fold(
		(int)rint(ox),(int)rint(oy), true, anim, alpha, 4.0);
}

// ----------------------
void AScreen::_dragRotate(MetisseWindow *win, int x, int y, int type)
{
	//if (!_selection || win != _selection->getWindow()) return;

	float rx = x - _prevX;
	float ry = y - _prevY;

	if (_portrait)
	{
		ry = - (x - _prevX);
		rx = + (y - _prevY);	
	}
	else
	{
		rx = x - _prevX;
		ry = y - _prevY;
	}

	switch(_rotateType)
	{
	case DRAG_ROTATE_Z:
		if ((rx > 0 && ry < 0) || (rx < 0 && ry < 0))
		{
			_opWin->getRenderer()->rotate_rel(-rx/2, 0, 0, -ry/2);
			sendMetisseTransform(_opWin, false);
		}
		else if ((rx < 0 && ry > 0) || (rx > 0 && ry > 0))
		{
			_opWin->getRenderer()->rotate_rel(-rx/2, 0, 0, ry/2);
			sendMetisseTransform(_opWin, false);
		}
		break;
	case DRAG_ROTATE_X:
		if (ry < 0)
		{
			root->rotateWindow(_opWin, ry/2, -ry/2, 0, 0);
		}
		else if (ry > 0)
		{
			root->rotateWindow(_opWin, ry/2, ry/2, 0, 0);
		}
		break;
	case DRAG_ROTATE_Y:
		if (rx > 0)
		{
			root->rotateWindow(_opWin, rx/2, 0, rx/2, 0);
		}
		else if (rx < 0)
		{
			root->rotateWindow(_opWin,rx/2, 0, -rx/2, 0);
		}
		break;
	default: 
	case DRAG_ROTATE_XY:
		if (ry < 0)
		{
			root->rotateWindow(_opWin,ry/2, -ry/2, 0, 0);
		}
		else if (ry > 0)
		{
			root->rotateWindow(_opWin,ry/2, ry/2, 0, 0);
		}
		if (rx > 0)
		{
			root->rotateWindow(_opWin, rx/2, 0, rx/2, 0);
		}
		else if (rx < 0)
		{
			root->rotateWindow(_opWin,rx/2, 0, -rx/2, 0);
		}
		break;
	}
}


void AScreen::_dragRotate(MetisseWindow *win, int x, int y)
{
	_dragRotate(win, x,y,_rotateType);
}


void AScreen::startDragRotate(MetisseWindow *win, int type, bool tmp)
{
	_rotate = true;
	_rotateType = type;
	_rotateTmp = tmp;
	_opWin = win;
	win->getRendererFullTransformation(_savedTrans);
}

void AScreen::_stopDragRotate(bool cancel)
{
	_rotate = false;
	if (!_rotateTmp && !cancel)
	{
		_fvwmModule->sendMetisseTransform(_opWin, true);
		return;
	}

	if (_rotateTmp && !cancel)
	{
		_opWin->getRenderer()->transformBack(_savedTrans);
	}
	else
	{
		_opWin->getRendererFullTransformation(_savedTrans);
	}
}

// ----------------------------

bool AScreen::waitingForAConfigureWindow(void)
{
	bool rc = false;

	if (_waitForAConfigureWindow)
	{
		rc = true;
		_waitForAConfigureWindow = false;
		if (_tmpInternalMove)
		{
			char cmd[256];
			float x,y;

			_imWindow->getRealPosition(&x, &y);
			sprintf(cmd, "AnimatedMove w%ip w%ip",
				(int)(- x + _imStartX), (int)(- y + _imStartY));
			_fvwmModule->sendText(cmd, _imWindow);
			_tmpInternalMove = false;
		}
	}
	return rc;
}

bool AScreen::inInternalMove(MetisseWindow *w)
{
	return (_inInternalMove && (w == 0 || w == _imWindow));
}

void AScreen::_snapForbidSwitch(void)
{
	if (!_inInternalMove)
	{
		return;
	}

	if (_snapForbid)
	{
		_snapForbid = false;
	}
	else
	{
		_snapForbid = true;

		_snapXOffset = _snapYOffset = 0;
		_doSGoX = _doSGoY = 0;
		_sgWarpX = _sgWarpY = 0;
		_sgInWarp = false;
		_snapForbX = _snapForbY = false;
		if (_snapFRWinX)
		{
			_snapFRWinX->unsnap();
			_snapFRWinX->unlens();
			_snapFRWinX = 0;
		}
		if (_snapFRWinY)
		{
			_snapFRWinY->unsnap();
			_snapFRWinY->unlens();
			_snapFRWinY = 0;
		}
	}
}

void AScreen::_internalMove(int x, int y, int *mod_x, int *mod_y)
{
	float oldX, oldY;
	_imWindow->getRealPosition(&oldX, &oldY);
	float xx = x, yy = y;
	float sx,sy;
	static int st_x, st_y;


	if (_pswmInWarp && _pswmWarpX == x && _pswmWarpY == y)
	{
		//fprintf(stderr, "pswm GetWarp %i %i\n",_pswmWarpX,_pswmWarpY);
		_pswmInWarp = false;
		return;
	}

	if (_sgInWarp && _sgWaitWarpX == x && _sgWaitWarpY == y)
	{
		//fprintf(stderr,"sg GetWarp %i %i\n",_sgWaitWarpX,_sgWaitWarpY);
		_sgInWarp = false;
		return;
	}
	else if (_sgInWarp)
	{
		//fprintf(stderr, "BAD EVENT in Warp\n");
	}

	 // FIXME: should (un)project over root and snapping! 
	root->getScale(&sx,&sy);
	
	int npx, npy, wx, wy, ptx, pty;
	if (root->checkMoveWindowPaneAction(
		    x, y, &npx, &npy, &wx, &wy, &ptx, &pty))
	{
		_pswmWarpX = wx;
		_pswmWarpY = wy;
		_pswmNPX = npx;
		_pswmNPY = npy;
		_pswmPTX = ptx;
		_pswmPTY = pty;
		if (_pageSwitchWinMove != 0)
		{
		}
		else
		{
			_pageSwitchWinMove = TimeKeeper::create();
			subscribeTo(_pageSwitchWinMove);
		}
		// FIXME: should be configurable
		_pageSwitchWinMove->arm(700);
	}
	else if (_pageSwitchWinMove != 0)
	{
		unsubscribeFrom(_pageSwitchWinMove);
		delete _pageSwitchWinMove;
		_pageSwitchWinMove = 0;
	}

	if (x != _prevX || y != _prevY)
	{
		MetisseWindow *win_x, *win_y;
		bool do_x,do_y;
		int snap_x = (int)oldX + (x - _prevX) + _snapXOffset;
		int snap_y = (int)oldY + (y - _prevY) + _snapYOffset;
		int p_snap_x = snap_x;
		int p_snap_y = snap_y;
		int prevOffsetX = _snapXOffset;
		int prevOffsetY = _snapYOffset;
		bool right, top;

		if ((root->getSnapAttractMode() == SNAP_ATTRAC_MODE_SG ||
		     root->getSnapAttractMode() == SNAP_ATTRAC_MODE_VISSG) &&
		    !_snapForbid)
		{
			bool rc = root->doSnapAttract(
				_imWindow, &snap_x, &snap_y, &do_x, &do_y,
				&right, &top, &win_x, &win_y);
			_snapXOffset = (p_snap_x - snap_x);
			_snapYOffset = (p_snap_y - snap_y);
			// SNAP-AND-GO
			// Not to bad should use a snap dist of 20 at least.
			// However: (a) should we warp the cursor?
			//     I would say yes and no there are various problem
			//     here
			// (b) If we are fast we may miss the start doSGoX
			//     (feature?)

			int sg_warp_x = x;
			int sg_warp_y = y;
			bool do_warp_x = false;
			bool do_warp_y = false;
			WindowRenderer *snap_frwin_x = 0;
			WindowRenderer *snap_frwin_y = 0;
#define ALWAYS_WARP 1
#define DONOT_WARP 0
#if not(ALWAYS_WARP)
			prevOffsetX = 0;
			prevOffsetY = 0;
#else
			_sgWarpX = _sgWarpY = 0;
#endif
			// x
			if (do_x && (_doSGoX == 0 || _sgWinX != win_x) &&
			    _snapXOffset != 0)
			{
				//fprintf(stderr,"STARTX %i %i\n",
				//	_snapXOffset, _doSGoX);
				if (_doSGoX != 0)
				{
					do_warp_x = true;
					sg_warp_x = x + _sgWarpX;
				}
				_doSGoX = 1;
				_sgWinX = win_x;
				if (_snapXOffset < 0)
				{
					_doSGoX = -1;
				}
				_snapXOffset = 0;
				xx = x;
			}
			else
			{
				if (do_x && _doSGoX >= 0 && _snapXOffset < 0)
				{
					// snap
					_sgWarpX =
						- (_snapXOffset - prevOffsetX);
					xx = _prevX + (snap_x - oldX)*sx;
					if (win_x)
					{
						snap_frwin_x = 
							win_x->getRenderer();
					}
					else
					{
						snap_frwin_x = 
							_imWindow->getRenderer();
					}
					//fprintf(stderr, "SnapX %i %i %i %i\n",
					//	_snapXOffset, x, right,top);
				}
				else if (do_x && _doSGoX <= 0 &&
					 _snapXOffset > 0)
				{
					// snap
					_sgWarpX =
						-  (_snapXOffset - prevOffsetX);
					xx = _prevX + (snap_x - oldX)*sx;
					if (win_x)
					{
						snap_frwin_x = 
							win_x->getRenderer();
					}
					else
					{
						snap_frwin_x = 
							_imWindow->getRenderer();
					}
					//fprintf(stderr, "SnapX %i %i %i %i\n",
					//	_snapXOffset, x, right,top);
				}
				else if (0 && do_x && win_x)
				{
					snap_frwin_x = win_x->getRenderer();
					xx = x;
					_snapXOffset = 0;
				}
				else
				{
					xx = x;
					_snapXOffset = 0;
				}
			}
			if (!do_x && _doSGoX != 0)
			{
				//fprintf(stderr,"CancelX\n");
				do_warp_x = true;
				sg_warp_x = x + _sgWarpX;
				_sgWarpX = 0;
				_doSGoX = 0;
			}

			// y
			if (do_y && (_doSGoY == 0 || _sgWinY != win_y) &&
			    _snapYOffset != 0)
			{
				//fprintf(stderr,"STARTY %i\n", _snapYOffset);
				if (_doSGoY != 0)
				{
					do_warp_y = true;
					sg_warp_y = y + _sgWarpY;
				}
				_doSGoY = 1;
				_sgWinY = win_y;
				if (_snapYOffset < 0)
				{
					_doSGoY = -1;
				}
				_snapYOffset = 0;
				yy = y;
			}
			else
			{
				if (do_y && _doSGoY >= 0 && _snapYOffset < 0)
				{
					// snap
					_sgWarpY =
						- (_snapYOffset - prevOffsetY);
					yy = _prevY + (snap_y - oldY)*sy;
					if (win_y)
					{
						snap_frwin_y = 
							win_y->getRenderer();
					}
					else
					{
						snap_frwin_y = 
							_imWindow->getRenderer();
					}
					//fprintf(stderr, "SnapY %i %i %i\n",
					//	_snapYOffset, y, _prevY);
				}
				else if (do_y && _doSGoY <= 0 &&
					 _snapYOffset > 0)
				{
					// snap
					_sgWarpY =
						- (_snapYOffset - prevOffsetY);
					yy = _prevY + (snap_y - oldY)*sy;
					if (win_y)
					{
						snap_frwin_y = 
							win_y->getRenderer();
					}
					else
					{
						snap_frwin_y = 
							_imWindow->getRenderer();
					}
					//fprintf(stderr, "SnapY %i %i %i\n",
					//	_snapYOffset, y, _prevY );
				}
				else
				{
					_snapYOffset = 0;
					yy = y;
				}
			}
			if (!do_y && _doSGoY != 0)
			{
				//fprintf(stderr,"CancelY\n");
				do_warp_y = true;
				sg_warp_y = y + _sgWarpY;
				_sgWarpY = 0;
				_doSGoY = 0;
			}

			// snap and go lens
			if (root->getSnapAttractMode() ==
			    SNAP_ATTRAC_MODE_VISSG && (do_x || do_y))
			{
				if (win_x && snap_frwin_x)
				{
					if (win_x == win_y && snap_frwin_x &&
					    snap_frwin_y)
					{
						snap_frwin_x->lensSnapAndGo(
							true, _snapXOffset,
							!right,
							(_snapXOffset >= 0)?
							true:false,
							true, _snapYOffset, !top,
							(_snapYOffset <= 0)?
							true:false); 
					}
					else
					{
						snap_frwin_x->lensSnapAndGo(
							true, _snapXOffset,
							!right,
							(_snapXOffset >= 0)?
							true:false,
							false, 0, false, false);
					}
				}
				if (win_y && snap_frwin_y &&
				    !(win_x == win_y && snap_frwin_x))
				{
					snap_frwin_y->lensSnapAndGo(
						false, 0, false, false,
						true, _snapYOffset, !top,
						(_snapYOffset <= 0)? true:false); 
				}
				if (!win_x && snap_frwin_x && !win_y &&
				     snap_frwin_y)
				{
					int sad = root->getSnapAttractDist();
					snap_frwin_x->lensSnapAndGo(
						true,
						(_snapXOffset >= 0)?
						sad -_snapXOffset:
						-sad-_snapXOffset, right,
						(_snapXOffset >= 0)? true:false,
						true,
						(_snapYOffset >= 0)?
						sad -_snapYOffset:
						-sad-_snapYOffset, top,
						(_snapYOffset <= 0)? true:false);
				}
				else if (!win_x && snap_frwin_x)
				{
					int sad = root->getSnapAttractDist();
					snap_frwin_x->lensSnapAndGo(
						true,
						(_snapXOffset >= 0)?
						sad -_snapXOffset:
						-sad-_snapXOffset, right,
						(_snapXOffset >= 0)? true:false,
						false, 0, false, false); 
				}
				else if (!win_y && snap_frwin_y)
				{
					int sad = root->getSnapAttractDist();
					snap_frwin_y->lensSnapAndGo(
						false, 0, false, false,
						true,
						(_snapYOffset >= 0)?
						sad -_snapYOffset:
						-sad-_snapYOffset, top,
						(_snapYOffset <= 0)? true:false);
				}
			}
			if (_snapFRWinX && _snapFRWinX != snap_frwin_x)
			{
				_snapFRWinX->unlens();
			}
			if (_snapFRWinY && _snapFRWinY != snap_frwin_y)
			{
				_snapFRWinY->unlens();
			}
			_snapFRWinX = snap_frwin_x;
			_snapFRWinY = snap_frwin_y;
#if ALWAYS_WARP
			if (!do_warp_x)
				sg_warp_x = x + _sgWarpX;
			if (!do_warp_y)
				sg_warp_y = y + _sgWarpY;
			do_warp_y = do_warp_x = true;
			//fprintf(stderr, "WARP0: %i %i\n", sg_warp_x-x,
			//		sg_warp_y-y);
#endif
#if not(DONOT_WARP)
			if (((do_warp_y && y != sg_warp_y) ||
			     (do_warp_x && x != sg_warp_x)) &&
			    sx == 1 && sy == 1)
			{
				// FIXME: desktop scale vs cursor warpping
				if (!do_warp_y)
					sg_warp_y = y;
				if (!do_warp_x)
					sg_warp_x = x;
				char cmd[256];
			
				if (_sgInWarp)
				{
					//fprintf(stderr, "DOUBLE  Warp\n");
				}
				#if 0
				sprintf(cmd, "WindowId root 1 WarpToWindow "
					"%ip %ip", sg_warp_x, sg_warp_y);
					//sprintf(cmd, "CursorMove %ip %ip",
					// sg_warp_x-x, sg_warp_y-y);
					//fprintf(stderr, "WARP: %i %i\n",
					// sg_warp_x-x, sg_warp_y-y);
				sendFvwmCmd(cmd, 0);
				#else
				warpCursor(sg_warp_x, sg_warp_y);
				#endif
				_sgWaitWarpX= sg_warp_x;
				_sgWaitWarpY = sg_warp_y;
				_sgInWarp = true;
			       
				if (_cursor)
				{
					_cursor->updatePosition(
						sg_warp_x-_winwidth/2.0,
						_winheight/2.0-sg_warp_y);
				}
				*mod_x = sg_warp_x;
				*mod_y = sg_warp_y;
				
				_sgWarpX = 0;
				_sgWarpY = 0;
			}
#endif // not(DONOT_WARP)
		} // SNAP_ATTRAC_MODE_SG
		else if (root->getSnapAttractMode() == SNAP_ATTRAC_MODE_CLASSICAL
			&& !_snapForbid)
		{
			bool rc = root->doSnapAttract(
				_imWindow, &snap_x, &snap_y, &do_x, &do_y,
				&right, &top, &win_x, &win_y);
			_snapXOffset = (p_snap_x - snap_x);
			_snapYOffset = (p_snap_y - snap_y);
			// classical snapping
			// FIXME: should warp the cursor when sx sy are != 1
			if (x != _prevX + (snap_x - (int)oldX))
			{
				xx = (float)_prevX + ((float)snap_x - oldX)*(sx);
			}
			else
			{
				xx = x;
			}
			if (y != _prevY + (snap_y - (int)oldY))
			{
				yy = (float)_prevY + ((float)snap_y - oldY)*(sy);
			}
			else
			{
				yy = y;
			}
		}
		else if (root->getSnapAttractMode() == SNAP_ATTRAC_MODE_METISSE
			 && !_snapForbid)
		{
			p_snap_x = snap_x = (int)oldX + (x - _prevX);
			p_snap_x = snap_y = (int)oldY + (y - _prevY);
			bool rc = root->doSnapAttract(
				_imWindow, &snap_x, &snap_y, &do_x, &do_y,
				&right, &top, &win_x, &win_y);
			_snapXOffset = snap_x - (int)oldX - (x - _prevX);
			_snapYOffset = snap_y - (int)oldY - (y - _prevY);
			#if 0
			if (do_x)
			{
				fprintf(stderr,"MSX: %i %i %i\n", _snapXOffset,
					(win_x)? 1:0, right);
			}
			if (do_y)
			{
				fprintf(stderr,"MSY: %i %i %i\n", _snapYOffset,
					(win_y)? 1:0, top);
			}
			#endif
			WindowRenderer *m_snap_rwin_x = 0;
			WindowRenderer *m_snap_rwin_y = 0;
			if ((do_x && !_snapForbX) || (do_y && !_snapForbY))
			{
				do_x = (do_x && !_snapForbX);
				do_y = (do_y && !_snapForbY);
				if (do_x && win_x && win_x->getRenderer() &&
				    do_y && win_x == win_y)
				{
					win_x->getRenderer()->snap(
						do_x, _imWindow, -_snapXOffset,
						!right,
						do_y, _imWindow, -_snapYOffset,
						!top);
					m_snap_rwin_x = win_x->getRenderer();
					m_snap_rwin_y = win_y->getRenderer();
				}
				else if (do_x && win_x && win_x->getRenderer())
				{
					win_x->getRenderer()->snap(
						do_x, _imWindow, -_snapXOffset,
						!right,
						false, 0, 0, false);
					m_snap_rwin_x = win_x->getRenderer();
				}
				if (do_y && win_y && win_y->getRenderer() &&
				    win_x != win_y)
				{
					win_y->getRenderer()->snap(
						false, 0, 0,false,
						do_y, _imWindow, -_snapYOffset,
						!top);
					m_snap_rwin_y = win_y->getRenderer();
				}
				if (do_x && win_x == 0)
				{
					if (do_y && win_y == 0)
					{
						_imWindow->getRenderer()->snap(
							do_x, 0, _snapXOffset,
							right,
							do_y, 0, _snapYOffset,
							top);
						m_snap_rwin_x =
							_imWindow->getRenderer();
						m_snap_rwin_y =
							_imWindow->getRenderer();
					}
					else
					{
						_imWindow->getRenderer()->snap(
							do_x, 0, _snapXOffset,
							right,
							false, 0, 0, false);
						m_snap_rwin_x =
							_imWindow->getRenderer();
					}
				}
				else if (do_y && win_y == 0)
				{
					_imWindow->getRenderer()->snap(
						false, 0, 0, false,
						do_y, 0, _snapYOffset,
						top);
					m_snap_rwin_y =
						_imWindow->getRenderer();
				}
			}
			if (_snapForbX && !do_x)
			{
				_snapForbX = false;
			}
			if (_snapForbY && !do_y)
			{
				_snapForbY = false;
			}
			if (_snapFRWinX && _snapFRWinX != m_snap_rwin_x)
			{
				_snapFRWinX->unsnap();
			}
			if (_snapFRWinY && _snapFRWinY != m_snap_rwin_y)
			{
				_snapFRWinY->unsnap();
			}
			_snapFRWinX = m_snap_rwin_x;
			_snapFRWinY = m_snap_rwin_y;
		} // SNAP_ATTRAC_MODE_METISSE
		else
		{
			// no snapping
			xx = x;
			yy = y;
		}

		#if 0
		if (rc || _snapXOffset != 0 ||  _snapYOffset != 0)
			fprintf(stderr,"Offset %i %i %i %i %i\n",
				rc, _snapXOffset, _snapYOffset,
				(win_x)? 1:0, (win_y)? 1:0);
		#endif

		float tx,ty;
		if (_portrait)
		{
			ty = ((float)(-_prevX + xx))*(1/sx);
			tx = -((float)(_prevY - yy))*(1/sy);
		}
		else
		{
			tx = ((float)(-_prevX + xx))*(1/sx);
			ty = ((float)(_prevY - yy))*(1/sy);
		}
		_imWindow->translate_rel(tx, ty, 1);
		_imWindow->setRealPosition(
			(float)oldX + tx, (float)oldY - ty);
		if (!root->checkMove(
			    _imWindow, _prevX - (int)rint(xx),
			    _prevY - (int)rint(yy)))
		{
			_imWindow->translate_rel(-tx, -ty, 1);
			_imWindow->setRealPosition(
				(float)oldX, (float)oldY);
		}
	}
}

void AScreen::startInternalMove(
	MetisseWindow *win, int x, int y, float alpha, bool tmp)
{
	if (_inInternalMove || _inInteractiveScale || win == 0)
	{
		return;
	}

	_tmpInternalMove = tmp;

#if 0
	std::cerr << "Start Internal Move" << std::endl;
#endif
	win->setTmpAlpha(alpha);
	win->getRealPosition(&_imStartX, &_imStartY);
	win->saveSurfaceScaleRealPosition();
	_inInternalMove = true;
	_imWindow = win;

	int xx,yy;

	_x11projectOverWindow(win, x, y, &xx, &yy);
	//_x11project(x, y, &xx, &yy);

	int savePrevX = _prevX;
	int savePrevY = _prevY;
	_prevX = xx;
	_prevY = yy;
	int mod_x = savePrevX;
	int mod_y = savePrevY;
	_snapXOffset = _snapYOffset = 0;
	_doSGoX = _doSGoY = 0;
	_sgWarpX = _sgWarpY = 0;
	_pswmInWarp =_sgInWarp = false;
	_snapFRWinX = _snapFRWinY = 0;
	_snapForbX = _snapForbY = false;
	_snapForbid = false;

#if 0
	// see if are snapping
	MetisseWindow *win_x, *win_y;
	bool do_x,do_y;
	bool right, top;
	float oldX, oldY;
	_imWindow->getRealPosition(&oldX, &oldY);

	int snap_x = (int)oldX;
	int snap_y = (int)oldY;
	bool rc = root->doSnapAttract(
		_imWindow, &snap_x, &snap_y, &do_x, &do_y,
		&right, &top, &win_x, &win_y);
	if (do_x && snap_x == (int)oldX)
	{
		_snapForbX = true;
	}
	if (do_y && snap_y == (int)oldY)
	{
		_snapForbY = true;
	}
#endif

	_internalMove(savePrevX, savePrevY, &mod_x, &mod_y);
	_prevX = _currX = mod_x;
	_prevY = _currY = mod_y;
}

void AScreen::_stopInternalMove(bool cancel)
{
	float x,y;
	float sx,sy;
	char cmd[256];
	
#if 0
	std::cerr << "Stop Internal Move" << std::endl;
#endif

	_imWindow->getRealPosition(&x, &y);
	root->getScale(&sx,&sy); // FIXME: should (un)project over root
	if (cancel)
	{
		// FIXME: add an animation if _tmpInternalMove
		_imWindow->translate_rel(_imStartX - x, - _imStartY + y, 0);
		_imWindow->setRealPosition(_imStartX, _imStartY, true);
		if (_snapFRWinX)
		{
			_snapFRWinX->unsnap();
			_snapFRWinX->unlens();
		}
		if (_snapFRWinY)
		{
			_snapFRWinY->unsnap();
			_snapFRWinY->unlens();
		}
		_inInternalMove = false;
		resetSelection();
	}
	else
	{
		float xx = x;
		float yy = y;
		if (_snapFRWinX)
		{
			if (root->getSnapAttractMode() ==
			    SNAP_ATTRAC_MODE_METISSE)
			{
				xx = x + _snapXOffset;
			}
			_snapFRWinX->unsnap();
			_snapFRWinX->unlens();
			
		}
		if (_snapFRWinY)
		{
			if (root->getSnapAttractMode() ==
			    SNAP_ATTRAC_MODE_METISSE)
			{
				yy = y + _snapYOffset;
			}
			_snapFRWinY->unsnap();
			_snapFRWinY->unlens();
		}
		if ((int)(xx - _imStartX) != 0 || (int)(yy - _imStartY) != 0)
		{
			sprintf(cmd, "Move w%ip w%ip",
				(int)(xx - _imStartX), (int)(yy - _imStartY));
			_fvwmModule->sendText(cmd, _imWindow);
			_waitForAConfigureWindow = true;
		}
		else
		{
			//fprintf(stderr,"DO NOT MOVE\n");
			_imWindow->translate_rel(
				_imStartX - x, - _imStartY + y, 0);
			_imWindow->setRealPosition(_imStartX, _imStartY, true);
		}
		_inInternalMove = false;

		// for what?
		#if 1
		int savePrevX = _prevX;
		int savePrevY = _prevY;
		_prevX = (int)rint(_prevX-1*(1/sx));
		_prevY = (int)rint(_prevY+1*(1/sy));
		//pointerEventOverWindow(
		//	root->getRootWindow(), _prevX, _prevY);
		resetSelection(true);
		_prevX = savePrevX;
		_prevY = savePrevY;
		if (!_tmpInternalMove)
		{	
			_imWindow->saveSurfaceScaleRealPosition();
		}
		#endif
		resetSelection();
	}
	_snapFRWinX = _snapFRWinY = 0;
	_imWindow->resetAlpha();
	_inInternalMove = false;
	unsubscribeFrom(_pageSwitchWinMove);
	delete _pageSwitchWinMove;
	_pageSwitchWinMove = 0;
	//_imWindow = 0;
}


void AScreen::_interactiveScale(int x, int y)
{
	float sx,sy;
	float width, height, win_x, win_y;
	GLdouble ox, oy, oz;
	float scale_x, scale_y, scale_z;
	float itx,ity;
	bool doit = false;
	int ux,uy;
	float xx, yy;

	// get
	_imWindow->getSize(&width, &height);
	_imWindow->getRealPosition(&win_x, &win_y);
	_imWindow->getInternalTranslation(&itx,&ity);
	_imWindow->getAbsoluteScale(&scale_x, &scale_y, &scale_z);

	xx = x;
	yy = y;
	#if 0
	if (!unprojectOverWindow(_imWindow, (int)x, (int)y, &ox, &oy, &oz))
	{
		return;
	}
	xx = ox + width/2.0 + win_x; // + itx; // + itx*(scale_x-1.0)/2.0;
	yy = -oy + height/2.0 + win_y - 1.0; // - 1.0 - scale_y*ity;
	
	std::cerr << "unproject x,y: " << x << " " << y <<  " "
		  << xx << " " << yy <<  " "
		  << itx << " " << ity << std::endl;
	#endif

	GLdouble nwx = win_x, nwy = win_y, nwz;
	#if 0
	projectOverWindow(_imWindow, (int)win_x, (int)win_y, &nwx, &nwy, &nwz);
	nwy = _winheight - nwy;
	std::cerr << "winx,y " << win_x << " " << win_y << " " 
		  << nwx << " " << nwy << "\n";
	#endif
	#if 0
	std::cerr << "winx,y " << win_x << " " << win_y << " x,y: " 
		  << x << " " << y << " dx,dy: " 
		  << _isDX << " " << _isDY <<"\n";
	#endif
	win_x = nwx;
	win_y = nwy;
	
#if 0
	// FIXME: should use something like that!
	GLdouble left, top, right, bottom;
	root->getWinBoundingBox(_imWindow, &left, &top, &right, &bottom, true);
#endif
	
	root->getScale(&sx,&sy); // FIXME: should (un)project over root
	//std::cerr << "    sx,sy: " << sx << " " << sy << std::endl;
	//sx = sy = 1;

	if (_interactiveGravity == EastGravity ||
	    _interactiveGravity == NorthEastGravity ||
	    _interactiveGravity == SouthEastGravity)
	{
		if (xx > win_x && xx < win_x + width - 20)

		{
			scale_x = ((width - (xx - win_x)) / width)*(1/sx);
			doit = true;
		}
		else if (xx < win_x)
		{
			scale_x = ((width + (win_x - xx)) / width)*(1/sx);
			doit = true;
		}
	}
	else if (_interactiveGravity == WestGravity ||
	    _interactiveGravity == NorthWestGravity ||
	    _interactiveGravity == SouthWestGravity)
	{
		if (xx < win_x + width && xx > win_x + 20)
		{
			scale_x = ((xx - win_x) / width)*(1/sx);
			doit = true;
		}
		else if (xx > win_x + width)
		{
			scale_x = ((width - (win_x + width - xx)) / width)
				*(1/sx);
			doit = true;
		}
	}
	if (_interactiveGravity == SouthGravity ||
	    _interactiveGravity == SouthWestGravity ||
	    _interactiveGravity == SouthEastGravity)
	{
		if (yy > win_y && yy < win_y + height - 20)
		{
			scale_y = ((height - (yy - win_y)) / height)*(1/sy);
			doit = true;
		}
		else if (yy < win_y)
		{
			scale_y = ((height + (win_y - yy)) / height)*(1/sy);
			doit = true;
		}
	}
	else if (_interactiveGravity == NorthGravity ||
		 _interactiveGravity == NorthEastGravity ||
		 _interactiveGravity == NorthWestGravity)
	{
		if (yy < win_y + height && yy > win_y + 20)
		{
			scale_y = ((yy - win_y) / height)*(1/sy);
			doit = true;
		}
		else if (yy > win_y + height)
		{
			scale_y = ((height - (win_y + height - yy)) / height)
				*(1/sy);
			doit = true;
		}
	}
	if (doit)
	{
		//std::cerr << "    " << scale_x << " " << scale_y << std::endl;
		root->scaleWindow(
			_imWindow, scale_x, scale_y, 1, _interactiveGravity,
			true);
	}
}

void AScreen::startInteractiveScale(
	MetisseWindow *win, int x, int y, float alpha, bool tmp)
{
	if (_inInternalMove || _inInteractiveScale)
	{
		return;
	}

	_tmpInteractiveScale = tmp;

	// guess the gravity: FIXME do it using the deco!
	float width, height;
	float win_x, win_y;

	win->getSize(&width, &height);
	win->getRealPosition(&win_x, &win_y);

	#if 0
	GLdouble ox,oy,oz;
	int dx,dy;
	int px,py;
	_x11projectOverWindow(win, _currX,_currY, &px, &py);
	_isDX = x - px; //obc2x11scX(ox, width/2.0, win_x);
	_isDY = y - py; //obc2x11scY(oy, height/2.0, win_y);
	#endif
	#if 0
	std::cerr << "SIS: " << x << " " << y <<  " "
		  << px  << " " << py <<  " "
		  << win_x << " " << win_y  <<  " "
		  << width << " " << height
		  << std::endl;
	#endif

	int x_gravity = 0;
	int y_gravity = 0;
	
	if (x < win_x + 10)
	{
		x_gravity = EastGravity;
	}
	else if (x > width + win_x - 10)
	{
		x_gravity = WestGravity;
	}
	if (y < win_y + 10)
	{
		y_gravity = SouthGravity;
	}
	else if (y > height + win_y - 10)
	{
		y_gravity = NorthGravity;
	}

	_interactiveGravity = x_gravity;

	switch(y_gravity)
	{
	case NorthGravity:
	{
		switch(x_gravity)
		{
		case EastGravity:
			_interactiveGravity = NorthEastGravity;
			break;
		case WestGravity:
			_interactiveGravity = NorthWestGravity;
			break;
		default:
			_interactiveGravity = y_gravity;
			break;
		}
	}
	break;
	case SouthGravity:
	{
		switch(x_gravity)
		{
		case EastGravity:
			_interactiveGravity = SouthEastGravity;
			break;
		case WestGravity:
			_interactiveGravity = SouthWestGravity;
			break;
		default:
			_interactiveGravity = y_gravity;
			break;
		}
	}
	break;
	default:
		break;
	}

	if (_interactiveGravity == 0)
	{
		return;
	}

	win->setTmpAlpha(alpha);
	_inInteractiveScale = true;
	_imWindow = win;
}

void AScreen::_stopInteractiveScale(bool cancel)
{
	if (cancel)
	{
		// FIXME: an animation if _tmpInteractiveScale
		root->scaleWindow(
			_imWindow, 1, 1, 1, _interactiveGravity, true);
		resetSelection();
	}
	else if (_tmpInteractiveScale)
	{
		_imWindow->getRenderer()->scaleBack(root, _interactiveGravity);
	}
	else
	{
		_fvwmModule->sendMetisseTransform(_imWindow, true);
	}
	resetSelection();
	_imWindow->resetAlpha();
	_inInteractiveScale = false;
	_imWindow = 0;
}

// ------------------------

void AScreen::_stopDropingRegion(int button)
{
	_viewpoint->removeDependency(_dropWindow);
	delete _dropWindow;
	_dropWindow = 0;
	// ... or true!
	resetSelection(true);
	MetisseWindow *newWin = _selection->getWindow();
	if (newWin && newWin != _opWin && _opWin)
	{
		// Add the cuts
		if (newWin->isRootWindow() || newWin->isEwmhDesktop() ||
		    newWin->isUnmanaged())
		{
			if (button==2)
			{
				_fvwmModule->cutWindow(_opWin, &_dropRegion);
			}
			else
			{
				_fvwmModule->buildFacade(
					_opWin, &_dropRegion, _currX, _currY,
					(button==3)? true:false);
			}
		}
		else
		{
			GLdouble ox, oy, oz ;
			unprojectOverWindow(
				newWin, _currX, _currY, &ox, &oy, &oz);
			_metisseDesktop->addFacade(
				&_dropRegion, _opWin, newWin, ox, oy,
				_currX, _currY);
		}
	}
	_opWin = 0;
	//std::cerr << "RE DRop\n";
}

// ------------------------------------
// clip operation

void AScreen::fvwmTraceRunning(bool run)
{
	_fvwmTraceRunning = run;
}

#define CLIP_PRIMARY  0
#define CLIP_CLIPBORD 1

#define CLIP_MOUSE_MOTION_THRESHOLD 3

// button 2 or ctrl-v interception
bool AScreen::_clipIntercept(int type, int x, int y, unsigned long keysym)
{
	// clip FIXME should check that FvwmTrace run !
	if (!_fvwmTraceRunning ||
	    _clipOpTk != 0 || 
	    !_selection || !_selection->getWindow() ||
	    _selection->getWindow()->isUnmanaged() ||
	    _selection->getWindow()->isRootWindow() ||
	    _selection->getWindow()->isEwmhDesktop())
	{
		// should exculde more: pointer should be in a
		// normal client window ... exculde desktop, wm deco,
		// the case where the pointer is not over a window
		// which accept the focus ...
		return false;
	}

	_clipIn = true;
	_clipX =x; _clipY = y; _clipKeysym = keysym;
	_clipOpTk = TimeKeeper::create();
	subscribeTo(_clipOpTk);
	_clipOpTk->arm(200);

	_clipOpType = type;

	if (_clipOpType == CLIP_CLIPBORD)
	{
		compositor->setKeyboardAutoRepeat(false);
		_clipAutoRepeatOff = true;
	}

	return true;
}

void AScreen::_clipInterceptFinish(void)
{
	if (_clipOpTk)
	{
		unsubscribeFrom(_clipOpTk);
		delete _clipOpTk;
		_clipOpTk = 0;
	}
	if (_clipOpFastTk != NULL)
	{
		unsubscribeFrom(_clipOpFastTk);
		delete _clipOpFastTk;
		_clipOpFastTk = 0;
	}
	if (_miFeed != 0)
	{
		_viewpoint->removeDependency(_miFeed);
		delete _miFeed;
		_miFeed = 0;
	}
	_clipIn = false;
}

bool AScreen::_clipGiveUpInteception(int x, int y)
{
	if (!_fvwmTraceRunning)
	{
		return false;
	}

	_clipInterceptFinish();

	if (_clipOpType == CLIP_PRIMARY)
	{
		unsigned long save_button_mask = _button_mask;
		_button_mask = _button_mask | (1<<(1));
		_myPointerEvent(x,y, _button_mask);
		_button_mask = save_button_mask;
	}
	else if (_clipOpType == CLIP_CLIPBORD)
	{
		if (!_clipOpWaitForCtrlReleased)
		{
			_clipOpFastTk = TimeKeeper::create();
			subscribeTo(_clipOpFastTk);
			_clipOpFastTk->arm(100);
			//std::cerr << "Arm Fast Tk" << std::endl;
			return true;
		}
		else
		{
			_clipOpWaitForCtrlReleased = false;
			if (_selection)
			{
				_selection->keyEvent(_clipKeysym, true);
			}
			else
			{
				_metisseDesktop->rootKeyEvent(_clipKeysym, true);
			}
			compositor->setKeyboardAutoRepeat(true);
		}
	}

	return false;
}

void AScreen::_clipInterceptTkExpire(void)
{
	if (_clipOpType == CLIP_PRIMARY)
	{
		WinRegions::region r = WinRegions::region(_currX, _currY, 16, 16);
		_miFeed = new DropWindow("curfeed", &r, _currX, _currY);
		_miFeed->setColor(1,0,0,0.5);
		_viewpoint->addDependency(_miFeed);
		_miFeed->updatePosition(
			_currX - _winwidth/2.0, _winheight/2.0 - _currY);
	}
	else // CLIP_CLIPBORD
	{
		_clipAskClipboardMenu(false);
	}
	if (_clipOpTk)
	{
		unsubscribeFrom(_clipOpTk);
		delete _clipOpTk;
		_clipOpTk = 0;
	}
}

// motion during mouse interception
bool AScreen::_clipInterceptMotion(int x, int y)
{
	if (!_fvwmTraceRunning ||  !_clipIn || _clipOpType != CLIP_PRIMARY)
	{
		return false;
	}

	if (abs(_clipX - x) > 3 || abs(_clipY - y) > 3)
	{
		// give up ... FIXME should do some stuff ? ...
		if (_clipOpTk != 0)
		{
			_clipGiveUpInteception(x, y);
			return false;
		}
		else
		{
			_clipAskClipboardMenu(false);
		}
	}

	return true;
}

bool AScreen::_clipInterceptRelease(int x, int y)
{
	if (!_fvwmTraceRunning || _clipOpType != CLIP_PRIMARY || !_clipIn)
	{
		return false;
	}

	if (_clipOpTk != 0)
	{
		_clipGiveUpInteception(x, y);
		return false;
	}
	else
	{
		_clipAskClipboardMenu(false);
	}

	return true;
}

void AScreen::_clipAskClipboardMenu(bool isvv)
{
	if (!_fvwmTraceRunning)
	{
		return;
	}

	if (_clipOpType == CLIP_PRIMARY)
	{
		sendFvwmCmd(
			"SendToModule FvwmTrace ClipboardMenu PRIMARY",
			0);
		// should we send the click to a void window ?
	}
	else
	{
		if (isvv)
		{
			sendFvwmCmd(
				"SendToModule FvwmTrace ClipboardMenu CLIPBOARD"
				" Next",
				0);
		}
		else
		{
			sendFvwmCmd(
				"SendToModule FvwmTrace ClipboardMenu CLIPBOARD",
				0);
		}
		_clipOpWaitForCtrlReleased = true;
	}
	_clipInterceptFinish();
}

void AScreen::_clipCtrlReleased(void)
{
	if (!_fvwmTraceRunning)
	{
		return;
	}

	_clipOpWaitForCtrlReleased = false;
	if (_clipAutoRepeatOff)
	{
		compositor->setKeyboardAutoRepeat(true);
		_clipAutoRepeatOff = false;
	}
}

// fast V-V stuff
void AScreen::_clipFastVVGiveUp(void)
{
	if (!_fvwmTraceRunning)
	{
		return;
	}

	if (_clipOpFastTk == NULL)
	{
		return;
	}

	if (_selection)
	{
		_selection->keyEvent(_clipKeysym, true);
		_selection->keyEvent(_clipKeysym, false);
	}
	else
	{
		_metisseDesktop->rootKeyEvent(_clipKeysym, true);
		_metisseDesktop->rootKeyEvent(_clipKeysym, false);
	}

	_clipOpWaitForCtrlReleased = false;
	if (_clipOpFastTk != NULL)
	{
		unsubscribeFrom(_clipOpFastTk);
		delete _clipOpFastTk;
		_clipOpFastTk = 0;
	}
}

void AScreen::clipPasteClipboard(void)
{
	// FIXME: should unrealese all modifiers first
	if (_selection)
	{
		_selection->keyEvent(XK_Shift_L, true);
		_selection->keyEvent(XK_Insert, true);
		_selection->keyEvent(XK_Insert, false);
		_selection->keyEvent(XK_Shift_L, false);
	}
	else
	{
		_metisseDesktop->rootKeyEvent(XK_Shift_L, true);
		_metisseDesktop->rootKeyEvent(XK_Insert, true);
		_metisseDesktop->rootKeyEvent(XK_Insert, false);
		_metisseDesktop->rootKeyEvent(XK_Shift_L, false);
	}
	
}


// ---------------------------------
// right button motion and capture ...

#define ASCREEN_LONG_PRESS   0
#define ASCREEN_PRESS_MOTION 1
#define ASCREEN_PRESS_PAUSE_MOTION 2
#define ASCREEN_RIGHT_LONG_PRESS_TO 500

void AScreen::_setupRightLongPressTimeOut(void)
{
	_right_long_press_timeout = ASCREEN_RIGHT_LONG_PRESS_TO;
}


void AScreen::setRightLongPressTimeOut(unsigned to)
{
	if (to < 250)
		to = 250;
	
	_right_long_press_timeout = to;
}

bool AScreen::_motionInterceptPress(int x, int y, int button)
{
	if (!_fvwmTraceRunning)
	{
		return false;
	}

	// FIXME: should check that no menu and co is up !
	if (_miIn)
	{
		// should cancel and capture release
		return true;
	}
	if (!_inDragAndDrop && button != 3)
	{
		return false;
	}
	if (_modifier_mask != 0)
	{
		return false;
	}
	if (_inDragAndDrop)
	{
		// for now but ...
		if ((_dnd_button_mask & (1<<(0))) && (_button_mask & (1<<(0))) &&
		    button == 3 && !_inDNDragtoFreeDnd)
		{
			// button 3 allows to switch free drag-and-drop
			_inDNDInterceptReleases = 2;
			_inDNDragtoFreeDnd = true;
			//fprintf(
			//	stderr,"_motionInterceptPress dnd: INTERCEPT\n");
			
		}
		else if (button >= 3)
		{
			// in free drag and drop button >= 3 allows cmd
			//fprintf(stderr,"_motionInterceptPress dnd: BUTTON %i\n",
			//	button);
			// FIXME: we can do a better job !
			if (button == 4)
			{
				sendFvwmCmd(
					"Next (!Sticky CurrentPage) "
					"AFuncDeiconifyFocusAndRaise", 0);
			}
			else if (button == 5)
			{
				sendFvwmCmd(
					"Prev (!Sticky CurrentPage) "
					"AFuncDeiconifyFocusAndRaise", 0);
			}
			else if (button == 3)
			{
				// do more complex things
				//sendFvwmCmd(
				//	"MyShowDesktop Nop MyShowDesktopOff", 0);
				if (_dndTrailing && !_dndTrailing->isCaptured())
				{
					//fprintf(stderr,
					//	"_motionInterceptPress dnd: B3 "
					//	"Popup\n");
					_dndTrailing->popupMenu();
				}
			}
		}
		else if (_button_mask == (1<<(0)))
		{
			// FIXME: in drag to free drag initiate by us capture
			// the press ?
			//fprintf(stderr,
			//	"_motionInterceptPress dnd: BUTTON1 %i\n",
			//	_inDNDragtoFreeDnd);
			if (_inDNDragtoFreeDnd)
			{
				_motionInterceptFinish();
				return true;
			}
			return false;
		}
		else
		{
			return false;
		}
	}
	_miStartX = x;
	_miStartY = y;
	_miIn = true;
	_miTkcount = 0;
	if (_motionInterceptTk != 0)
	{
		delete _motionInterceptTk;
		_motionInterceptTk = 0;
	}

	if (!_inDragAndDrop)
	{
		_motionInterceptTk =  TimeKeeper::create();
		subscribeTo(_motionInterceptTk);
		_motionInterceptTk->arm(_right_long_press_timeout);
	}
	
	return true;
}

void AScreen::_motionInterceptFinish(void)
{
	_miIn = false;
	_miTkcount = 0;
	_inDNDInterceptReleases = 0;
	//_inDNDragtoFreeDnd = false;
	if (_motionInterceptTk != 0)
	{
		unsubscribeFrom(_motionInterceptTk);
		delete _motionInterceptTk;
		_motionInterceptTk = 0;
	}

	if (_miFeed)
	{
		_viewpoint->removeDependency(_miFeed);
		delete _miFeed;
		_miFeed = 0;
	}		
}

bool AScreen::_motionInterceptRelease(int x, int y)
{
	if (!_miIn)
	{
		return false;
	}
	if (_inDragAndDrop)
	{
		if (_inDNDInterceptReleases)
		{
			_inDNDInterceptReleases--;
		}
		//fprintf(stderr,"_motionInterceptRelease (in) %i\n",
		//	_inDNDInterceptReleases);
		if (_inDNDInterceptReleases == 0)
		{
			// FIXME: in case where the drag has been initiate by
			// button 1 drag
			_dnd_button_mask = _button_mask;
			_button_mask = _button_mask | (1<<(0)) ;
		}
		if (_inDNDInterceptReleases <= 0)
		{
			// maybe an action on button 3
			_motionInterceptFinish();
			_inDNDInterceptReleases--;
		}
		//fprintf(stderr,"_motionInterceptRelease %i bm:%lu dbm:%lu\n",
		//	_inDNDInterceptReleases,_button_mask,_dnd_button_mask);
		return true;
		
	}
	if (_button_mask & (1<<(3-1)) )
	{
		// not button 3
		_motionInterceptCancel(x,y);
		return true;
	}

	if (_miTkcount == 0)
	{
		// a click
		_motionInterceptCancel(x, y);
		return false;
	}
	_motionInterceptAction(_currX, _currY, ASCREEN_LONG_PRESS);

	return true;
}

void AScreen::_motionInterceptCancel(int x, int y)
{
	if (!_miIn)
	{
		return;
	}

	_motionInterceptFinish();

	unsigned long save_button_mask = _button_mask;
	_button_mask = _button_mask | (1<<(2));
	_myPointerEvent(x,y, _button_mask);
	_button_mask = save_button_mask;
}

void AScreen::_motionInterceptAction(int x, int y, int what)
{
	if (what == ASCREEN_LONG_PRESS)
	{
		sendFvwmCmd(
			"SendToModule FvwmTrace ClipboardMenu DND",
			0);
	}
	else if (what == ASCREEN_PRESS_MOTION)
	{
		#if 0
		sendFvwmCmd(
			"StrokeFunc MetisseCalled FeedBack DrawMotion "
			"StrokeWidth 2",
			0);
		#endif
		_motionInterceptCancel(x, y);
	}
	else if (what == ASCREEN_PRESS_PAUSE_MOTION)
	{
		sendFvwmCmd(
			"StrokeFunc MetisseCalled 3 FeedBack DrawMotion "
			"StrokeWidth 2",
			0);
	}
	else
	{
		_motionInterceptCancel(x, y);
	}

	_motionInterceptFinish();
}

bool AScreen::_motionInterceptMotion(int x, int y)
{
	if (!_miIn)
	{
		return false;
	}
	if (_inDragAndDrop)
	{
		// do some actions ...
		//fprintf(stderr,"_motionInterceptMotion\n");
		return true;
	}
	if ((abs(_miStartX - x) > 3 || abs(_miStartY - y) > 3))
	{
		if (_miTkcount == 0)
		{
			_motionInterceptAction(x,y,ASCREEN_PRESS_MOTION);
		}
		else
		{
			_motionInterceptAction(x,y,ASCREEN_PRESS_PAUSE_MOTION);
		}
	}

	return true;
}

void AScreen::_motionInterceptTkExpire(void)
{
	// long-press
	_miTkcount = 1;
	_miStartX = _currX;
	_miStartY = _currY;

	WinRegions::region r = WinRegions::region(_currX, _currY, 16, 16);
	_miFeed = new DropWindow("curfeed", &r, _currX, _currY);
	_miFeed->setColor(1,0,0,0.5);
	_viewpoint->addDependency(_miFeed);
	_miFeed->updatePosition(
		_currX - _winwidth/2.0, _winheight/2.0 - _currY);
}

// -----------------------------------------------------------------------------
// Widget

void AScreen::popupScrollLeafingWidget(sgNode *parent)
{
	if (_scrollLeafingW != 0)
	{
		return;
	}

	WinRegions::region r = WinRegions::region(
		_currX - _winwidth/2.0,_winheight/2.0 - _currY, 45, 30);
	_scrollLeafingW = new ScrollLeafingWidget(
		this, parent, &r, true /* add desktop */);
	_viewpoint->addDependency(_scrollLeafingW);
	
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// events handling

// modifiers
void AScreen::_setModifierMask(unsigned long keysym, bool press)
{
	unsigned long mask = 0;

	switch(keysym)
	{
	case XK_Shift_L:
	case XK_Shift_R:
		mask = ShiftMask;
		break;
	case XK_Control_L:
	case XK_Control_R:
		mask = ControlMask;
		break;
	case XK_Meta_L:
	case XK_Meta_R:
		mask = Mod1Mask;
		break;
	case XK_Alt_L:
	case XK_Alt_R:
		mask = Mod1Mask;
		break;
	case XK_Super_L:
	case XK_Super_R:
		mask = Mod4Mask;
		break;
	case XK_Hyper_L:
	case XK_Hyper_R:
		mask = Mod5Mask;
		break;
	}

	if (mask == 0)
	{
		return;
	}

	if (press)
	{
		_modifier_mask = _modifier_mask | mask;
	}
	else
	{
		_modifier_mask = _modifier_mask & !mask;
	}

	/* fprintf(
		stderr,"Modifiers Mask: %i %lu %lu\n", press, mask,
		_modifier_mask); */
}

// --------------------------
// drag and drop external event (maybe this should be handled in fvwm ...)

void AScreen::dragAndDropStart(void)
{
	//std::cerr << "dragAndDropStart" <<  std::endl ;
	_current_dnd_win = 0;
	_inDragAndDrop = true;
	tmpRestack(0,false,0);
	_dnd_managed_selection = _managed_selection;
	_dnd_button_mask = _button_mask;
	_inDNDragtoFreeDnd = false;
	_fvwmModule->sendText("FuncShowDesktopOnDragFunc",0);
	if (_dndTrailing == 0)
	{
		WinRegions::region r = WinRegions::region(_currX, _currY, 5, 5);
		_dndTrailing = new TrailWidget(this, &r);
		_dndTrailing->setColor(1,0,0,1);
		_viewpoint->addDependency(_dndTrailing);
		_dndTrailing->updatePosition(
			_currX - _winwidth/2.0,
			_winheight/2.0 - _currY);
	}
	if (_selection && _selection != _managed_selection &&
	    _selection->getWindow()->isUnmanaged() && _managed_selection)
	{
		//std::cerr << "Attach to Sel Start\n";
		_current_dnd_win = _selection->getWindow();
		if (_current_dnd_win && _managed_selection->getWindow())
		{
			_current_dnd_win->setUnmanagedFor(
				_managed_selection->getWindow());
			_current_dnd_win->setTmpAlpha(0.5);
		}
	}
	if (_current_dnd_win == 0)
	{
		//std::cerr << "Attach to Sel Start 0\n";
		_current_dnd_win = root->getUnmanagedWindow();
		if (_current_dnd_win && _managed_selection &&
		    _managed_selection->getWindow())
		{
			_current_dnd_win->setUnmanagedFor(
				_managed_selection->getWindow());
			_current_dnd_win->setTmpAlpha(0.5);
		}
	}
}

void AScreen::dragAndDropStop(void)
{
	if (_inDragAndDrop)
	{
		_fvwmModule->sendText(
			"Schedule 1000 FuncShowDesktopOnDropFunc",0);
	}
	if (_scrollLeafingW != 0)
	{
		_viewpoint->removeDependency(_scrollLeafingW);
		delete _scrollLeafingW;
		_scrollLeafingW = 0;
	}
	if (_dndTrailing != 0)
	{
		_viewpoint->removeDependency(_dndTrailing);
		delete _dndTrailing;
		_dndTrailing = 0;
	}
	_current_dnd_win = 0;
	_inDragAndDrop = false;
	//std::cerr << "dragAndDropStop" <<   " " << _button_mask << std::endl ;
	if (_miIn)
	{
		//std::cerr << "dragAndDropStop_miIn " <<  _inDNDragtoFreeDnd
		//	  << " " << _button_mask << std::endl ;
		if (_inDNDragtoFreeDnd)
		{
			_button_mask = _button_mask & ~(1<<(0)) ;
		}
		// hum ...
		_motionInterceptFinish();
	}
	_inDNDragtoFreeDnd = false;
}

void AScreen::_dragAndDropMotion(int x, int y)
{
	if (!_inDragAndDrop)
	{
		return;
	}

	if (_pswmInWarp && _pswmWarpX == x && _pswmWarpY == y)
	{
		_pswmInWarp = false;
		return;
	}

	int npx, npy, wx, wy, ptx, pty;
	if (false && root->checkMoveWindowPaneAction(
		    x, y, &npx, &npy, &wx, &wy, &ptx, &pty))
	{
		char cmd[256];
		_pswmWarpX = wx + npx*20;
		_pswmWarpY = wy + npy*20;
		_pswmNPX = npx;
		_pswmNPY = npy;
		_pswmPTX = ptx;
		_pswmPTY = pty;
		if (_pageSwitchWinMove != 0)
		{
		}
		else
		{
			_pageSwitchWinMove = TimeKeeper::create();
			subscribeTo(_pageSwitchWinMove);
		}
		// FIXME: should be configurable
		_pageSwitchWinMove->arm(500);
	}
	else if (_dnd_managed_selection != _managed_selection &&
		 _managed_selection != 0)
	{
		if (_dndRaiseTk == 0)
		{
			_dndRaiseTk = TimeKeeper::create();
			subscribeTo(_dndRaiseTk);
		}
		_dndRaiseTk->arm(400);
	}
	else if (0 && _managed_selection &&
		 _isOverWinDecor(_managed_selection->getWindow(),x,y))
	{
		/* stupide */
		//std::cerr << "_dragAndDropMotion LOWER" <<  std::endl ;
		_dnd_managed_selection = _managed_selection;
		_fvwmModule->sendText(
				"Silent Lower",
				_managed_selection->getWindow());
	}
	    
	
	// should pick over managed and raise with a delay ...
}

// --------------------------
// --------------------------

void AScreen::_exposeModeHandleEvents(void)
{
	static bool first = true;
	static bool hasmoved = false;
	static MetisseWindow *myFirstSelectedWindow ;
	bool recover = false;
	bool cancel = false;
	glWindow::event e ;
	unsigned long smm;

	if (first)
	{
		resetSelection();
		first = false;
		hasmoved = false;
		if (_dndTrailing)
		  _dndTrailing->hide();
		myFirstSelectedWindow  = 0;
		if (_managed_selection && _managed_selection->getWindow())
		{
			// FIXME: broken by the animation
			myFirstSelectedWindow  = _managed_selection->getWindow();
		}
	}
	while (compositor->getNextEvent(&e) && !recover)
	{
		switch (e.type)
		{
		case glWindow::event::configure:
			break;
		case glWindow::event::expose:
			_viewpoint->postRedisplay();
			break ;
		case glWindow::event::buttonPress:
			_button_mask = _button_mask | (1<<(e.button-1)) ;
			_currX = _prevX = e.x;
			_currY = _prevY = e.y;
			if (_perfMeter) _perfMeter->restart();
			break;
		case glWindow::event::pointerMotion:
			if (_cursor)
			{
				_cursor->updatePosition(
					e.x-_winwidth/2.0, _winheight/2.0-e.y) ;
			}
			if (_dndTrailing)
			{
				_dndTrailing->updatePosition(
					e.x - _winwidth/2.0,
					_winheight/2.0 - e.y);
			}
			hasmoved = true;
			_currX = e.x ;
			_currY = e.y ;
			if (false && _button_mask == 0)
			{
				resetSelection();
			}
			else if (false && _inDragAndDrop)
			{
				resetSelection();
			}
			_prevX = e.x ;
			_prevY = e.y ;	
			break;
		case glWindow::event::buttonRelease:
			_button_mask = _button_mask & (~(1<<(e.button-1))) ;
			recover = true;
			break;
		case glWindow::event::keyPress:
			smm = _modifier_mask;
			_setModifierMask(e.keysym, true);
			//fprintf(stderr,"keyPress _modifier_mask %i\n",
			//	_modifier_mask);
			if (smm != _modifier_mask)
			{
				if (_selection)
				{
					_selection->keyEvent(e.keysym, true);
				}
				else
				{
					_metisseDesktop->rootKeyEvent(
						e.keysym, true);
				}

			}
			if (e.keysym == XK_e)
			{
				recover = true;
				if (!hasmoved)
					cancel = true;
			}
			else if (e.keysym == XK_d || e.keysym == XK_D)
			{
				recover = true;
				sendFvwmCmd(
					"MyShowDesktop Nop Nop",
					0);
			}
			break;
		case glWindow::event::keyRelease:
			smm = _modifier_mask;
			_setModifierMask(e.keysym, false);
			//fprintf(stderr,"keyRelease _modifier_mask %i\n",
			//	_modifier_mask);
			if (e.keysym == XK_Escape)
			{
				recover = true;
				cancel = true;
			}
			// FIXME: e.keysym == XK_e
			if (smm != _modifier_mask || e.keysym == XK_e)
			{
				if (_selection)
				{
					_selection->keyEvent(e.keysym, false);
				}
				else
				{
					_metisseDesktop->rootKeyEvent(
						e.keysym, false);
				}
			}
			break;
		case glWindow::event::leave:
			break;
		default:
			break ;	
		}
	}

	if (recover)
	{
		resetSelection();
		root->exposeMode(cancel);
		//draw(false);
		if (!cancel && _managed_selection)
		{
			_fvwmModule->sendText(
				"Silent AFuncDeiconifyFocusAndRaise",
				_managed_selection->getWindow());
		}
		else if (!cancel && _selection &&
		    !_selection->getWindow()->isRootWindow() &&
		    !_selection->getWindow()->isEwmhDesktop())
		{
			_fvwmModule->sendText(
				"Silent AFuncDeiconifyFocusAndRaise",
				_selection->getWindow());
		}
		else if (cancel && myFirstSelectedWindow)
		{
			_fvwmModule->sendText(
				"Silent AFuncDeiconifyFocusAndRaise",
				myFirstSelectedWindow);
		}
		if (_dndTrailing)
		  _dndTrailing->unHide();
		//resetSelection();
		first = true;
	}
}


// --------------------------

void AScreen::_handleOneEvent(glWindow::event &e)
{
  // std::cerr << "AScreen::_handleOneEvent: " ; e.debug(std::cerr) ; std::cerr << std::endl ;

	static unsigned long _prevWheelTime = 0;

	_motion = 0;
	switch (e.type)
	{
	case glWindow::event::extensionEvent:
	{
		_extInputs->handleEvents(e);
		break;
	}
	case glWindow::event::configure:
	{
#if 0
		std::cerr << "Configure Window: " << _winwidth << " c: "
			  << e.width << " " << _winheight <<  " c: "
			  << e.height << " " << ready << std::endl;
#endif
		if (_winwidth != e.width || _winheight != e.height)
		{
			break;
		}
		double far;
		double near;
		double eyeDist;
		double s;
		glViewport(0,0,e.width,e.height) ;
		if (_orthoProj)
		{
			GLfloat w2=e.width/2.0, h2=e.height/2.0 ;
			((sgOrthoViewpoint *)_viewpoint)->resize(
				-w2,w2,-h2,h2,-5000,5000) ;
			eyeDist = 0;
			near = -5000;
			far = 5000;
			s = 1;
		}
		else
		{
			double fovy = 60.0 ;
			double a = (fovy*M_PI/180.0)/2.0 ;
			double h = (double)e.height / (2.0*sin(a)) ;
			double dist = sqrt(
				h*h - (double)(e.height*e.height)/4.0) ;
			double r = (GLfloat)e.height/(GLfloat)e.width;
			double u = 1.0;
			// FIXME: should be configurable with an option
			double v = 4.0; // 4
			s = (u+v)/2.0;
			near = dist*u;//dist;
			far = dist*v;//*10;
			eyeDist = s*dist;
			((sgPerspectiveViewpoint *)_viewpoint)->
				setNearLimit(near) ;
			((sgPerspectiveViewpoint *)_viewpoint)->
				setFarLimit(far + 10) ;
			((sgPerspectiveViewpoint *)_viewpoint)->
				setEyePoint(0.0, 0.0, eyeDist) ;
			((sgPerspectiveViewpoint *)_viewpoint)->
				resetTransformations() ;
			((sgPerspectiveViewpoint *)_viewpoint)->scale(
				s*r, s*1.0, 1.0) ;
		}
		root->setDesktopProperties(
			(float)_desktopWidth, (float)_desktopHeight,
			eyeDist, near, far, _viewpoint, s,
			(_portrait)?
			(float)_winheight/(float)_desktopWidth:
			(float)_winwidth/(float)_desktopWidth,
			(_portrait)?
			(float)_winwidth/(float)_desktopHeight:
			(float)_winheight/(float)_desktopHeight,
			(_portrait)? 90:0);

		if (_ext_inputs)
		{
			_extInputs = new ExtendedInputs(this);
			_ext_inputs = false;
		}
		else if (_extInputs)
		{
			// "OK"
		}
		if (_perfMeter)
		{
			_perfMeter->resetTransformations() ;
			_perfMeter->translate(
				-_winwidth/2+5, -_winheight/2+5, 0);
		}
		//root->scale(0.5, 0.5, 1);
	}
	break ;
	case glWindow::event::expose:
		_viewpoint->postRedisplay();
		break ;
	case glWindow::event::buttonPress:
	{
		_button_mask = _button_mask | (1<<(e.button-1)) ;
		if (e.x < 0) e.x = 0;
		if (e.x > _winwidth -1) e.x = _winwidth -1;
		if (e.y < 0) e.y = 0;
		if (e.y > _winheight -1) e.y = _winheight -1;

		int baccel = 0;
		bool send_pointer_event = true;

		_currX = e.x;
		_currY = e.y;
		// apple mouse ...
		if (0 && (e.button == 6 || e.button == 7))
		{
			_button_mask = _button_mask & (~(1<<(e.button-1)));
			if (e.button == 6)
			{
				e.button = 4;
			}
			else if (e.button == 7)
			{
				e.button = 5;
			}
			baccel = 3;
			_button_mask = _button_mask | (1<<(e.button-1)) ;
		}
#if 0
		std::cerr << "buttonPress: " << e.button
			  << " (status=" << _button_mask << ")"
			  << std::endl ;
#endif
		if (_vMouse)
		{
			_vMouse->updateState(e.button, true);
		}

		if (_wmMove)
		{
			_interactiveMove(e.x, e.y);
			send_pointer_event = false;
		}
		else if (_wmResize)
		{
			_interactiveResize(e.x, e.y);
			send_pointer_event = false;
		}
		else if (_inInternalMove)
		{
			_stopInternalMove();
			send_pointer_event = false;
		}
		else if (_inInteractiveScale)
		{
			_stopInteractiveScale();
			send_pointer_event = false;
		}
		else if (_vMouse &&_vMouse->checkClick(
				    e.x-_winwidth/2.0,_winheight/2.0-e.y))
		{
			_vMouseMove = true;
			send_pointer_event = false;
		}
		if (send_pointer_event &&_selection &&
		    _selection->hasSelectedRegions())
		{
			GLdouble ox, oy, oz ;
			unprojectOverWindow(
				_selection->getWindow(), e.x, e.y,
				&ox, &oy, &oz);
			WinRegions::region *tmpR = 
				_selection->getSelectRegionFromXY(ox,oy);
			if (tmpR)
			{
				if (e.button == 1 || e.button == 2)
				{
					_dropRegion = WinRegions::region(
						tmpR->x, tmpR->y,
						tmpR->w, tmpR->h);
					_opWin = _selection->getWindow();
					_dropWindow = new DropWindow(
						"dw", &_dropRegion, ox, oy);
					_viewpoint->addDependency(_dropWindow);
					_dropWindow->updatePosition(
						e.x-_winwidth/2.0,
						_winheight/2.0-e.y);
					//std::cerr << "Drop Win" << std::endl;
				}
				else if (e.button == 3)
				{
					_fvwmModule->sendText(
						"Menu MenuMenusFacadeOps",
						_selection->getWindow());
					_swallow_release = true;
				}
				send_pointer_event = false;
			}
		}
		if (send_pointer_event && e.button == 2 && 
		    _clipIntercept(CLIP_PRIMARY, e.x,e.y,0))
		{

			send_pointer_event = false;
		}
		if (send_pointer_event && _motionInterceptPress(e.x,e.y,e.button))
		{
			// ok
			send_pointer_event = false;
		}
		
		if (send_pointer_event)
		{
			_myPointerEvent(e.x,e.y, _button_mask);

			#if 0
			if (e.button == 4 || e.button == 5)
			{
				//std::cerr << "WT: " << e.time-_prevWheelTime
				//<< "\n";
				float d = e.time - _prevWheelTime;
				float acc = (20.0 - 20.0*(d/50.0));
				baccel = (int)acc;
				_prevWheelTime = e.time;
			}
			#endif

			if (baccel)
			{
				//std::cerr << "acc: " << mult << "\n";
				for (int i = 1; i <= baccel; i++)
				{
					_button_mask =
						_button_mask &
						(~(1<<(e.button-1))) ;
					usleep(500); //
					_myPointerEvent(e.x,e.y, _button_mask);
					_button_mask =
						_button_mask | (1<<(e.button-1));
					usleep(500); //
					_myPointerEvent(e.x,e.y, _button_mask);
				}
			}
		}

		_prevX = _pressX = e.x ;
		_prevY = _pressY = e.y ;
		if (e.button == 1)
		{
			dragAndDropStop();
		}
		if (_perfMeter) _perfMeter->restart() ;	
		break;
	}
#if __APPLE__
	case glWindow::event::wheelMotion:
	{
		int fakebutton = (e.delta>0) ? 4 : 5 ;
		if (_vMouse)
		{
			_vMouse->updateState(fakebutton, true);
		}
		_button_mask = _button_mask | (1<<(fakebutton-1)) ;
		_myPointerEvent(_prevX, _prevY) ;
		_button_mask = _button_mask & (~(1<<(fakebutton-1))) ;
		if (_vMouse)
		{
			_vMouse->updateState(fakebutton, false);
		}
		_myPointerEvent(_prevX, _prevY) ;
	} break ;
#endif
	case glWindow::event::pointerMotion:
	{
		if (_swallow_release)
		{
			break;
		}
#if 0
		std::cerr << "Motion: "
			  << e.x << " " << e.y << std::endl ;
		std::cerr << "prev: "
			  << _prevX << " " << _prevY << std::endl ;
#endif
		float tx = 0;
		float ty = 0;
		if (e.x < 0) e.x = 0;
		if (e.x > _winwidth -1) e.x = _winwidth -1;
		if (e.y < 0) e.y = 0;
		if (e.y > _winheight -1) e.y = _winheight -1;

		_currX = e.x;
		_currY = e.y;
		root->checkPaneAction(e.x,e.y, _inInternalMove);
		_motion = 1;
		if (_cursor)
		{
			_cursor->updatePosition(
				e.x-_winwidth/2.0, _winheight/2.0-e.y) ;
		}

		if (_scrollLeafingW != 0)
		{
			int ta = _scrollLeafingW->getAction();
			if (ta == STACKW_ACTION_UNMAP)
			{
				_viewpoint->removeDependency(_scrollLeafingW);
				delete _scrollLeafingW;
				_scrollLeafingW = 0;
			}
		}  
		if (_dndTrailing != 0)
		{
			_dndTrailing->updatePosition(
				e.x - _winwidth/2.0,
				_winheight/2.0 - e.y);
			int ta = _dndTrailing->getAction();
			if (ta == TRAILING_ACTION_TL)
			{
				sendFvwmCmd(
					"MyShowDesktop Nop MyShowDesktopOff",
					0);
				_dndTrailing->unCapture();
			}
			else if (ta == TRAILING_ACTION_BR)
			{
				std::cerr << "TRAILING_ACTION_BR\n";
				_dndTrailing->unCapture();
					
			}
			else if (ta == TRAILING_ACTION_TR)
			{
				std::cerr << "TRAILING_ACTION_TR\n";
				if (_scrollLeafingW == 0)
				{
					#if 0
					popupScrollLeafingWidget(_dndTrailing);
					_dndTrailing->unCapture();
					_dndTrailing->setChild(_scrollLeafingW);
					_viewpoint->removeDependency(
						_dndTrailing);
					_viewpoint->addDependency(_dndTrailing);
					#endif
					if (_popScrollLeafingTk == 0)
					{
						_popScrollLeafingTk =
							TimeKeeper::create();
						subscribeTo(_popScrollLeafingTk);
						_popScrollLeafingTk->arm(50);
						_xpopSL = e.x;
						_ypopSL = e.y;
					}
				}
			}
			else if (ta == TRAILING_ACTION_BL)
			{
				std::cerr << "TRAILING_ACTION_BL\n";
				_dndTrailing->unCapture();
					
			}
		}
		if (_scrollLeafingW != 0)
		{
			_scrollLeafingW->updatePosition(e.x - _winwidth/2.0,
				_winheight/2.0 - e.y);
		}
		if (_stroke)
		{
			_motion = 0;
			_strokeAdd(e.x,e.y);
		}
		if (_clipInterceptMotion(e.x, e.y))
		{
			// FIXME: should we set curX and co ? A priori no
			break;
		}

		if (_clipInterceptMotion(e.x, e.y))
		{
			// FIXME: should we set curX and co ? A priori yes
			_currX = e.x;
			_currY = e.y;
			break;
		}
		if (_motionInterceptMotion(e.x, e.y))
		{
			// FIXME: should we set curX and co ? A priori yes
			_currX = e.x;
			_currY = e.y;
			break;
		}

		if (_wmMove)
		{
			_motion = 0;
			_interactiveMove(e.x, e.y);
		}
		else if (_wmResize)
		{
			_motion = 0;
			_interactiveResize(e.x, e.y);
		}
		else if (_inInternalMove)
		{
			int mod_x,mod_y;
			_motion = 0;
			mod_x = e.x;
			mod_y = e.y;
			_internalMove(e.x, e.y, &mod_x, &mod_y);
			_currX = e.x = mod_x;
			_currY = e.y = mod_y;
		}
		else if (_inInteractiveScale)
		{
			_motion = 0;
			_interactiveScale(e.x, e.y);
		}
		else if (_folding && _opWin) {
			_motion = 0;
			GLdouble ox, oy, oz ;
			unprojectOverWindow(
				_opWin,e.x, e.y, &ox, &oy, &oz);
			_opWin->getRenderer()->fold((int)ox,(int)oy, true);
		} 
		else if (_selectingRegion && _opWin) {
			_motion = 0;
			_inSelectRegion(e.x, e.y);
		} 
		else if (_rotate && _opWin)
		{
			_motion = 0;
			_dragRotate(_opWin, e.x,e.y);
		}
		else if (_vMouse && _vMouseMove)
		{
			_vMouse->move(_prevX - e.x, _prevY - e.y);
		}
		else if (_dropWindow)
		{
			_motion = 0;
			_dropWindow->updatePosition(
				e.x-_winwidth/2.0, _winheight/2.0-e.y) ;
		}
		else if (!_inDragAndDrop && _button_mask &&_selection)
		{
			// At least one button is pressed we should reset
			// the selection under certain condition
			_dragAndDropMotion(e.x, e.y);
			_dragResetSelection(true);
		}
		else
		{
			_dragAndDropMotion(e.x, e.y);
			resetSelection(e.x, e.y);
		}
		_prevX = e.x ;
		_prevY = e.y ;
	}
	break ;
	case glWindow::event::buttonRelease:
	{
		//apple silken mouse ...
		if (0 && (e.button == 6 || e.button == 7))
		{
			if (e.button == 6)
			{
				e.button = 4;
			}
			else if (e.button == 7)
			{
				e.button = 5;
			}
		}
		
		_button_mask = _button_mask & (~(1<<(e.button-1))) ;
		if (_swallow_release)
		{
			_swallow_release = false;
			// FIXME: ??
			//break;
		}
		if (e.x < 0) e.x = 0;
		if (e.x > _winwidth -1) e.x = _winwidth -1;
		if (e.y < 0) e.y = 0;
		if (e.y > _winheight -1) e.y = _winheight -1;
#if 0
		std::cerr << "buttonRelease: " << e.button
			  << " (status=" << (int)_button_mask << ")"
			  << std::endl ;
#endif

		_currX = e.x;
		_currY = e.y;

		if (_vMouse)
		{
			_vMouse->updateState(e.button, false);
		}
		bool reselect = false;
		if (_rotate)
		{
			reselect = true;
			_stopDragRotate(false);
			_opWin = 0;
		}
		if (_folding)
		{
			if (_opWin)
			{
				GLdouble ox, oy, oz ;
				unprojectOverWindow(
					_opWin,e.x, e.y, &ox, &oy, &oz) ;
				_opWin->getRenderer()->fold(
					(int)rint(ox),(int)rint(oy), false);
			}
			reselect = true;
			_folding = false;
			_opWin = 0;
		}
		if (_selectingRegion)
		{
			if (_opWin)
			{
				GLdouble ox, oy, oz ;
				unprojectOverWindow(
					_opWin,e.x, e.y, &ox, &oy, &oz);
				_opWin->getRenderer()->selectRegion(
					ox, oy, false);
			}
			reselect = true;
			_selectingRegion = false;
			_opWin = 0;
		}
		if (_clipInterceptRelease(e.x, e.y))
		{
			break;
		}
		if (_motionInterceptRelease(e.x, e.y))
		{
			break;
		}
		if (_inTmpRestack)
		{
			tmpRestack(_restackWin, false, 0);
		}

		if (_wmMove)
		{
			_interactiveMove(e.x, e.y);
		}
		else if (_wmResize)
		{
			_interactiveResize(e.x, e.y);
		}
		else if (_inInternalMove)
		{
			_stopInternalMove();
		}
		else if (_inInteractiveScale)
		{
			_stopInteractiveScale();
		}
		else if (_vMouse && _vMouseMove)
		{
			_vMouseMove = false;
		}
		else if (_dropWindow != 0)
		{
			_stopDropingRegion(e.button==3);
		}
		else
		{
			if (e.button == 1)
			{
				_checkSelectedIcons();
			}
			//std::cerr << "RE\n";
			// was (true || reselect)
			if (reselect)
			{
				resetSelection(e.x, e.y);
			}
			else
			{
				_myPointerEvent(e.x,e.y);
			}
		}
		if (e.button == 1)
		{
			dragAndDropStop();
		}
		_prevX = e.x ;
		_prevY = e.y ;
	}
	break ;
	case glWindow::event::keyPress:
	{
		bool stopOp = false;
		bool cancelOp = false;

		_setModifierMask(e.keysym, true);
		
		if (_inInternalMove || _inInteractiveScale|| _folding ||
		    _rotate)
		{
			switch(e.keysym)
			{
			case XK_Return:
			case XK_KP_Enter:
			case XK_space:
				stopOp = true;
				cancelOp = false;
				break;
			case XK_Escape:
				stopOp = true;
				cancelOp = true;
				break;
			default:
				_snapForbidSwitch();
				break;
			}
			// FIXME: handle keyboard move with the usual
			// keys, see fvwm/libs/Target.c
		}
		if (_clipOpFastTk != 0 && !_clipOpWaitForCtrlReleased &&
		    e.keysym == _clipKeysym)
		{
			_clipAskClipboardMenu(true);
			break;
		}
		else if (_clipOpFastTk != 0)
		{
			_clipFastVVGiveUp();
		}
		else if (_clipOpTk == 0 && _modifier_mask == ControlMask &&
			 !_clipOpWaitForCtrlReleased &&
			 (e.keysym == XK_v || e.keysym == XK_V) &&
			 _clipIntercept(CLIP_CLIPBORD, _currX, _currY, e.keysym))
		{
			//std::cerr << " Clip Op: Start Key " << std::endl;
			break;
		}
		else if (_inDragAndDrop)
		{
			if (e.keysym == XK_e || e.keysym == XK_E)
			{
				root->exposeMode();
			}
			else if (e.keysym == XK_d || e.keysym == XK_D)
			{
				sendFvwmCmd(
					"MyShowDesktop Nop Nop",
					0);
			}
		}
		if (stopOp)
		{
			if (_inInternalMove)
			{
				_stopInternalMove(cancelOp);
			}
			else if (_inInteractiveScale)
			{
				_stopInteractiveScale(cancelOp);
			}
			else if (_folding)
			{
				if (_opWin)
				{
					GLdouble ox, oy, oz ;
					unprojectOverWindow(
						_opWin,e.x, e.y, &ox, &oy, &oz) ;
					_opWin->getRenderer()->fold(
						(int)rint(ox),(int)rint(oy),
						false);
				}
				_folding = false;
				_opWin = 0;
			}
			else if (_rotate)
			{
				_stopDragRotate(cancelOp);
			}
			_opWin = 0;
		}
		if (_selectingRegion)
		{
			_srKeyCount++;
			if (_srKeyCount > 1)
			{
				_srSnap = !_srSnap;
				_srKeyCount = 0;
			}
		}
		if (_selection)
		{
			_selection->keyEvent(e.keysym, true) ;
		}
		else
		{
			_metisseDesktop->rootKeyEvent(e.keysym, true);
		}
		_metisseDesktop->setModifiers(e, true);
	}
	break ;
	case glWindow::event::keyRelease:
	{
		_setModifierMask(e.keysym, false);

		if (_selectingRegion)
		{
			_srKeyCount++;
			if (_srKeyCount > 1)
			{
				_srSnap = !_srSnap;
				_srKeyCount = 0;
			}
			
		}
		if (e.keysym == XK_Control_L || e.keysym == XK_Control_R)
		{
			//std::cerr << "Controle Released" << std::endl;
			_clipCtrlReleased();
			_clipFastVVGiveUp();
		}
		if (_clipOpTk && _clipOpType == CLIP_CLIPBORD && 
		    e.keysym == _clipKeysym)
		{
			if (_clipGiveUpInteception(0,0))
			{
				// still intercept for fast V-V
				break;
			}
		}
		if (_selection)
		{
			_selection->keyEvent(e.keysym, false);
		}
		else
		{
			_metisseDesktop->rootKeyEvent(e.keysym, false);
		}
		_metisseDesktop->setModifiers(e, false);
		//dragAndDropStop();
		break;
	}
	case glWindow::event::leave:
	case glWindow::event::focusOut:
#if 0
		std::cerr << "leave event "  << std::endl ;
#endif
		_metisseDesktop->releaseModifiers();
		break;
	case glWindow::event::enter:
#if 0
		std::cerr << "enter event "  << std::endl ;
#endif
	default:
		break ;
	}
	
}

void AScreen::react(Observable *obs)
{
	// std::cerr << "AScreen::react" << std::endl ;
	compositor->makeCurrent() ;

	if (_perfMeter) _perfMeter->postRedisplay() ;

	if (obs == _pageSwitchWinMove &&
	    ((_inInternalMove && _imWindow) || _inDragAndDrop))
	{
		char cmd[256];

		//fprintf(stderr, "Page Switchning!\n");
		sprintf(cmd,"GotoPage %ip %ip", _pswmNPX, _pswmNPY);
		sendFvwmCmd(cmd, 0);
		sprintf(cmd, "WindowId root 1 WarpToWindow %ip %ip",
			_pswmWarpX, _pswmWarpY);
		sendFvwmCmd(cmd, 0);

		if (_inInternalMove && _imWindow && !_inDragAndDrop)
		{
			float sx,sy;
			float tx,ty;
			float oldX, oldY;

			_imWindow->getRealPosition(&oldX, &oldY);
			root->getScale(&sx,&sy);
			// FIXME: portait!
			tx = ((float)(-_pswmPTX))*(1/sx);
			ty = ((float)(_pswmPTY))*(1/sy);
			_imWindow->translate_rel(tx, ty, 1);
			_imWindow->setRealPosition(
				(float)oldX + tx, (float)oldY - ty);
			_imStartX = _imStartX - _pswmPTX;
			_imStartY = _imStartY - _pswmPTY;
		}
		if (_inDragAndDrop)
		{
			// DND (for now)
			// FIXME: should send a event ?
			_myPointerEvent(
				_pswmWarpX+_pswmNPX, _pswmWarpY+_pswmNPY,
				_button_mask);
		}
		_pswmInWarp = true;
		unsubscribeFrom(_pageSwitchWinMove);
		delete _pageSwitchWinMove;
		_pageSwitchWinMove = 0;
	}

	if (obs == _dndRaiseTk)
	{
		 if (_dnd_managed_selection != _managed_selection &&
		     _managed_selection != 0)
		 {
			 _dnd_managed_selection = _managed_selection;
			 _fvwmModule->sendText(
				 "Silent Raise",
				 _managed_selection->getWindow());
		 }
	}

	if (obs == _clipOpFastTk)
	{
		_clipFastVVGiveUp();
	}

	if (obs == _clipOpTk)
	{
		//_clipAskClipboardMenu(false);
		_clipInterceptTkExpire();
	}

	if (obs == _motionInterceptTk)
	{
		_motionInterceptTkExpire();
	}

	if (obs == _popScrollLeafingTk)
	{
		if (abs(_xpopSL - _currX) <= 1 && abs(_ypopSL - _currY) <= 1)
		{
			if (_dndTrailing != 0)
			{
				popupScrollLeafingWidget(_dndTrailing);
				_dndTrailing->unCapture();
				_dndTrailing->setChild(_scrollLeafingW);
				_viewpoint->removeDependency(
					_dndTrailing);
				_viewpoint->addDependency(_dndTrailing);
			}
			else
			{
				popupScrollLeafingWidget(0);
			}
			unsubscribeFrom(_popScrollLeafingTk);
			delete _popScrollLeafingTk;
			_popScrollLeafingTk = 0;
		}
		else
		{
			_popScrollLeafingTk->arm(50);
		}
		_xpopSL = _currX;
		_ypopSL = _currY;
		
	}

	if (root->inExposeMode())
	{
		_exposeModeHandleEvents();
	}
	else
	{
		glWindow::event e ;
		while (compositor->getNextEvent(&e))
			_handleOneEvent(e);
	}

	draw(false) ;
}

void AScreen::draw(bool force)
{ 
	if (force || (_viewpoint->graphChanged() && !root->inMovePage())
#if CAPTURE_ON_TIMER
	    || (_capture && _capture_timer->getState()&TimeKeeper::TRIGGERED)
#endif
	    ) {
		root->preDisplay();
		_viewpoint->displayGraph(_breakDisplayLists ? sgNode::NODL : sgNode::CREATE) ;

		if (_capture) {
		  // std::cerr << "AScreen::draw: grab!" << std::endl ;
#if __APPLE__
		  glScreenCapture(&_capture_image, Image::ARGB, false) ;
#else
		  glScreenCapture(&_capture_image, Image::RGBA, false) ;
#endif
		  _capture->handle(&_capture_image) ;
		  compositor->makeCurrent() ;
		} else if (_saveFrame) {
		  Image jpeg ;
		  glScreenCapture(&jpeg, Image::JPEG, true) ;
		  jpeg.saveAs("capture.jpg") ;
		  std::cerr << "Image saved as capture.jpg" << std::endl ;
		  _saveFrame = false ;
		}

		compositor->swapBuffers() ;

		if (_pagerModeOffScheduled)
		{
			_pagerModeOffScheduled = false;
			root->pagerMode(false);
		}
		if (_resetSelectionScheduled) {
			//std::cerr << "_resetSelectionScheduled" << std::endl ;
			_resetSelectionScheduled = false;
			resetSelection((_miIn)? false:true);
		}
	}
}

// -----------------------------------------------------------------------------

AScreen::AScreen(MetisseDesktop *metisseDesktop, bool ortho,
			  bool noStencil, bool portrait, bool force_fullscreen)
{

	long options = glWindow::DOUBLE_BUFFER | glWindow::DEPTH;

	_noStencil = noStencil;
	if (!noStencil) options = options | glWindow::STENCIL;

	long eventmask = glWindow::event::configure
		| glWindow::event::expose
		| glWindow::event::pointerMotion
		| glWindow::event::buttonPress
#if __APPLE__
		| glWindow::event::wheelMotion
#endif
		| glWindow::event::buttonRelease
		| glWindow::event::keyPress
		| glWindow::event::keyRelease
#if 1
		| glWindow::event::leave
		| glWindow::event::focusOut
#endif
	  ;

	compositor = glWindow::create(options, eventmask) ;
	// compositor->debugEvents = true ;
	subscribeTo(compositor) ;

	compositor->syncToVbl() ;

	_metisseDesktop = metisseDesktop;
	subscribeTo(_metisseDesktop) ;
	_portrait = portrait;
	_force_fullscreen = force_fullscreen;
	_winShaders = new WindowShaders(_metisseDesktop);

	_orthoProj = ortho;
	if (_orthoProj)
	{
		_viewpoint = new sgOrthoViewpoint("orthoproj");
	}
	else
	{
		_viewpoint = new sgPerspectiveViewpoint(
			"persproj",
			0.0, 0.0, 1.25,   // ex, ey, ez
			0.001, 1500.0) ;  // near, far
	}
  
	_settings = new DefaultSettings() ;
	_viewpoint->addDependency(_settings) ;
	glClearDepth(100.2) ;

	root = new LayerManager(this, "layermanager");
	_viewpoint->addDependency(root) ;

	_ext_inputs = true;
	_extInputs = 0;

	_selection = 0;
	_selectionBufferSize = 0;
	_over_selection = 0;
	_over_selectionBufferSize = 0;
	_managed_selection = NULL;
	_dnd_managed_selection = NULL;
	_inMenuDrag = false;

	_resetSelectionScheduled = false;
	_pagerModeOffScheduled = false;
	_forceDoublePointerEvent = false;

	_doProjectCursor = true;

	_capture = 0;
	_capture_timer = 0;
	_perfMeter = 0;
	_cursor = 0;
	_stroke = 0;
	_dropWindow = 0;
	_vMouse = 0;
	_winwidth = 0;
	_winheight = 0;
	_button_mask = 0;
	_modifier_mask = 0;
	_swallow_release = false;
	_motion = 0;
	_wmMove = false;
	_wmResize = false;
	_resizeTransparency = 0.7;
	_folding = false;
	_selectingRegion = false;
	_srSnapDist = SR_SNAP_DIST;
	_rotate = false;
	_rotateTmp = false;
	_inInternalMove = false;
	_tmpInternalMove = false;
	_pageSwitchWinMove = 0;
	_pswmInWarp = false;
	_fvwmTraceRunning = false;
	_clipIn = false;
	_clipOpTk = 0;
	_clipOpFastTk = 0;
	_clipOpWaitForCtrlReleased = false;
	_clipAutoRepeatOff = false;
	_miIn = false;
	_motionInterceptTk = 0;
	_miTkcount = 0;
	_miFeed = 0;
	_setupRightLongPressTimeOut();
	_dndRaiseTk = NULL;
	_dndTrailing = 0;
	_inDragAndDrop = false;
	_current_dnd_win = NULL;
	_scrollLeafingW = 0;
	_popScrollLeafingTk = 0;
	_replay_x = 0;
	_replay_y = 0;
	_replay_size = 1;
	_replay_i = 0;
	_inInteractiveScale = false;
	_waitForAConfigureWindow = false;
	_currX = _prevX=0;
	_currY = _prevY=0;
	_breakDisplayLists = true;
	_saveFrame = false;
	_inPseudoSelectionMode = false;
	_opWin = 0;
	_imWindow = 0;
	_tmpRestackMode = TMP_RESTACK_MODE_FOLD;
	_inTmpRestack = false;
	_widgetManager = 0;
	_snapFRWinX = _snapFRWinY = 0;
	_fvwmModule = 0 ;


	_cursor = new ACursor(compositor, "cursor", 5, (_portrait)? 90:0);
	_cursor->setSoftwareCursor(false);
	_viewpoint->addDependency(_cursor);
	_metisseDesktop->setCursor(_cursor);
}

AScreen::~AScreen(void)
{
  std::cerr << "AScreen::~AScreen" << std::endl ;
	stopVideoCapture() ;
	compositor->restoreCorePointer();
#ifdef HAVE_SPI
	delete _widgetManager;
#endif
	_metisseDesktop->disconnect();
	// delete a lot of other stuff ... but we do not care
	// as we exit ... only useful for valgrind
}

