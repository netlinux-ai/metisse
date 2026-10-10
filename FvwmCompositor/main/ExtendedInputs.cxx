/*
 *
 * main/ExtendedInputs.cxx --
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

#include "ExtendedInputs.H"

#include <nucleo/gl/window/glWindow.H>
#include <nucleo/gl/glUtils.H>

using namespace nucleo ;

int AExtDevice::handleEvents(glWindow::event e)
{
	glWindow::event newe;

	switch(e.extType)
	{
	case glWindow::event::extStateNotify:
		//std::cerr << "extStateNotify" << std::endl;
		break;
	case glWindow::event::extProximityOut:
		//std::cerr << "extProximityOut" << std::endl;
		break;
	case glWindow::event::extProximityIn:
		//std::cerr << "extProximityIn" << std::endl;
		break;
	case glWindow::event::extFocusOut:
		std::cerr << "extFocusOut" << std::endl;
		break;
	case glWindow::event::extFocusIn:
		std::cerr << "extFocusIn" << std::endl;
		break;
	case glWindow::event::extButtonPress:
		//std::cerr << "extButtonPress " << e.button << std::endl;
		_button_mask = _button_mask | (1<<(e.button-1));
		if (_toolGlass || (_toolglassWin != 0 && e.button == 3))
		{
			break;
		}
		if (!_ascr->extInputSendEvents(
			    _prevX, _prevY, _button_mask, _num))
		{
			_swallow_release++;
		}
		break;
	case glWindow::event::extMotionNotify:
	{
		//std::cerr << "extMotionNotify" << std::endl;
		//std::cerr << "extMotionNotify" << " " << e.axis_data[0]
		//	  << " " << e.axis_data[1] << std::endl;

		unsigned int w,h;
		_ascr->compositor->getGeometry(&w, &h);
		if (_firtsExtMotionNotify)
		{
			_prevX = (int)(w/2.0);
			_prevY = (int)(h/2.0);
			_prevExtX = e.axis_data[0];
			_prevExtY = e.axis_data[1];
			_firtsExtMotionNotify = false;
			break;
		}
		int x = _prevX + (e.axis_data[0] - _prevExtX);
		int y = _prevY + (e.axis_data[1] - _prevExtY);
		int dx = (e.axis_data[0] - _prevExtX);
		int dy = (e.axis_data[1] - _prevExtY);
		if (x < 0)
		{
			x = 0;
		}
		else if (x > w-1)
		{
			x = w-1;
		}
		if (y < 0)
		{
			y = 0;
		}
		else if (y > h-1)
		{
			y = h-1;
		}

		GLdouble ox = x - w/2.0;
		GLdouble oy = h/2.0 - y;
		//std::cerr << " " << " " << x << " " << y << " " 
		//	  <<  ox << " " << oy <<std::endl;
		if (_toolGlass)
		{
			// move the window!
			char cmd[256];
			sprintf(cmd, "Move w%ip w%ip", (int)(dx), (int)(dy));
			// we may improve the performence ...
			_ascr->sendFvwmCmd(cmd, _toolglassWin);
		}
		else if (_cursor)
		{
			_cursor->updatePosition(ox, oy);
		}

		_prevExtX = e.axis_data[0];
		_prevExtY = e.axis_data[1];
		_prevX = x;
		_prevY = y;
		break;
	}
	case glWindow::event::extButtonRelease:
		//std::cerr << "extButtonRelease" << e.button << std::endl;
		_button_mask = _button_mask & (~(1<<(e.button-1)));
		if (_swallow_release > 0)
		{
			_swallow_release--;
			break;
		}
		if (e.button == 3 && _toolglassWin != 0)
		{
			_toggleToolGlass();
			break;
		}
		_ascr->extInputSendEvents(_prevX, _prevY, _button_mask, _num);
		break;
	default:
		//std::cerr << "unknown ext events" << std::endl;
		break;
	}

	return 0;
}

void AExtDevice::on(void)
{
	_firtsExtMotionNotify = true;
	_button_mask = 0;

	long eventmask = glWindow::event::extMotionNotify
		| glWindow::event::extButtonPress
		| glWindow::event::extButtonRelease
		| glWindow::event::extKeyPress
		| glWindow::event::extKeyRelease
		| glWindow::event::extStateNotify
		| glWindow::event::extProximityOut
		| glWindow::event::extProximityIn
		| glWindow::event::extFocusOut
		| glWindow::event::extFocusIn;

	_ascr->compositor->selectExtensionEvent(this, eventmask, false);
	_on = true;
	_swallow_release = 0;	
}

void AExtDevice::off(void)
{
	MetisseWindow *w = _toolglassWin;

	if (_toolglassWin && _toolGlass)
	{
		_toggleToolGlass();
	}
	if (_cursor)
	{
		_ascr->removeACursor(_cursor);
		delete _cursor;
		_cursor = 0;
	}
	if (_on)
	{
		_ascr->compositor->selectExtensionEvent(this, 0, false);
	}
	_on = false;
	_toolGlass = false;
	_toolglassWin = 0;
	if (w)
	{
		w->buildSpecialMenu();
	}
	_firtsExtMotionNotify = true;
}

void AExtDevice::cursor(void)
{
	MetisseWindow *w = _toolglassWin;

	if (!hasValuators())
	{
		return;
	}
	if (!_on)
	{
		on();
	}

	if (_cursor != 0)
	{
		if (_toolglassWin != 0 && _toolGlass)
		{
			_toggleToolGlass();
		}
		_toolGlass = false;
		_toolglassWin = 0;
	}
	else
	{
		if (_cursor == 0)
		{
			_cursor = new ACursor(_ascr->compositor, "extcursor", 16, 0,_num);
		}
		_ascr->addACursor(_cursor);
		_cursor->handleInternalCursor(7, 7, 16, 16);
		_cursor->updatePosition(0, 0);
	}
	if (w)
	{
		w->buildSpecialMenu();
	}
}


void AExtDevice::_toggleToolGlass(void)
{
	if (!_toolGlass)
	{
		_ascr->removeACursor(_cursor);
		_toolglassWin->setAlpha(0.5);
		_toolglassWin->setToolGlass(true);
		_ascr->sendFvwmCmd("Layer 0 7", _toolglassWin);
		// NoTitle?
		_ascr->sendFvwmCmd("WindowStyle NeverFocus, NoTitle",
				   _toolglassWin);
		
		_toolGlass = true;
	}
	else
	{
		_ascr->addACursor(_cursor);
		_toolglassWin->setAlpha(1);
		_toolglassWin->setToolGlass(false);
		_ascr->sendFvwmCmd("Layer 0 4", _toolglassWin);
		// Title?
		// FIXME: FOCUS
		_ascr->sendFvwmCmd("WindowStyle SloppyFocus, Title",
				   _toolglassWin);
		_ascr->sendFvwmCmd("Raise", _toolglassWin);
		float x,y,ww,wh;
		unsigned int w,h;
		_ascr->compositor->getGeometry(&w,&h);
		_toolglassWin->getCurrentRealPosition(&x, &y);
		_toolglassWin->getCurrentSize(&ww,&wh);
		_cursor->updatePosition((x+(ww/2))-w/2, h/2-(y+(wh/2)));
		_prevX = (int)(x+(ww/2));
		_prevY = (int)(y+(wh/2));
		_toolGlass = false;
		//_firstExtMotionNotify = true;
	}
}

void AExtDevice::toolGlass(MetisseWindow *win)
{
	if (!hasValuators() || win == 0)
	{
		return;
	}
	if (_toolglassWin == win)
	{
		if (!_toolGlass)
		{
			_toggleToolGlass();
		}
		return;
	}
	else if (_toolglassWin != 0)
	{
		if (_toolGlass)
		{
			_toggleToolGlass();
		}
	}

	if (!_on)
	{
		on();
	}
	if (!_cursor)
	{
		cursor();
	}
	_toolglassWin = win;
	_toolGlass = false;
	_toggleToolGlass();
	win->buildSpecialMenu();
}

bool AExtDevice::isOn(void)
{
	return _on;
}

bool AExtDevice::isCursor(void)
{
	return (_cursor != 0 && _toolglassWin == 0);
}

bool AExtDevice::isToolGlass(void)
{
	return (_toolglassWin != 0);
}

MetisseWindow *AExtDevice::getToolGlassWin(void)
{
	return _toolglassWin;
}

int AExtDevice::getNum(void)
{
	return _num;
}

void AExtDevice::windowDestroyed(MetisseWindow *w)
{
	if (w == _toolglassWin)
	{
		_toolglassWin = 0;
		cursor();
	}
}

AExtDevice::AExtDevice(
	AScreen *ascr, ExtendedInputs *inputs, glWindow::extensionDevice *ed,
	int num)
	: extensionDevice(ed->getID(),
			  // FIXME!
			  (unsigned long int)(ed->hasKeys()*HAS_KEYS |
					      ed->hasButtons()*HAS_BUTTONS |
					      ed->hasValuators()*HAS_VALUATORS |
					      ed->hasProximity()*HAS_PROXIMITY),
			  ed->getName())
{
	_ascr = ascr;
	_inputs = inputs;
	_num = num;
	_on = 0;
	_cursor = 0;
	_toolglassWin = 0;
	_toolGlass = false;

	//on();
}

//
// ----------------------------------------------------------------------
//

// called from MetisseWindow::buildSpecialMenu
bool ExtendedInputs::generateWindowMenus(MetisseWindow *win)
{
	int i = 0;
	char cmd[1024];
	bool rc = false;

	std::list<AExtDevice *>::iterator o;
	for(o = _devices.begin(); o != _devices.end(); o++)
	{
		if (!(*o)->hasValuators())
		{
			continue;
		}
		if ((*o)->getToolGlassWin() == win)
		{
			_ascr->sendFvwmCmd("+ I + \"\" Nop", 0);
			sprintf(cmd,
				"+ I + \"ToolGlass/Cursor Off\" sma ExtendedInput %s "
				"Off", (*o)->getName());
			_ascr->sendFvwmCmd(cmd, 0);
			sprintf(cmd,
				"+ I + \"Click Cursor Only\" sma ExtendedInput %s "
				"Cursor", (*o)->getName());
			_ascr->sendFvwmCmd(cmd, 0);
			return true;
		}
	}
	for(o = _devices.begin(); o != _devices.end(); o++)
	{
		if (!(*o)->hasValuators())
		{
			continue;
		}
		if (!rc)
		{
			_ascr->sendFvwmCmd("+ I + \"\" Nop", 0);
		}
		sprintf(cmd,
			"+ I + \"%s ToolGlass\" sma ExtendedInput %s "
			"ToolGlass", (*o)->getName(), (*o)->getName());
		_ascr->sendFvwmCmd(cmd, 0);
		rc = true;
	}

	return rc;
}

void ExtendedInputs::_generateCursorsMenus(void)
{
	char cmd[1024];
	_ascr->sendFvwmCmd("DestroyMenu MenuInterExtendedInputs", 0);
	_ascr->sendFvwmCmd("AddToMenu   MenuInterExtendedInputs "
			   "\"Extended Inputs\" Title", 0);

	//std::cerr << cmd << std::endl;
	int i = 0;
	std::list<AExtDevice *>::iterator o;
	for(o = _devices.begin(); o != _devices.end(); o++)
	{
		if (!(*o)->hasValuators())
		{
			// for now
			continue;
		}
		i++;
		sprintf(cmd, "AddToMenu MenuInterExtendedInputs \"%s (%i)\" "
			"Popup MenuInterExtendedInputs%i",
			(*o)->getName(), (*o)->getNum(), (*o)->getNum());
		_ascr->sendFvwmCmd(cmd, 0);
		//std::cerr << cmd << std::endl;
		sprintf(cmd,"DestroyMenu MenuInterExtendedInputs%i",
			(*o)->getNum());
		_ascr->sendFvwmCmd(cmd, 0);
		sprintf(cmd,"AddToMenu   MenuInterExtendedInputs%i",
			(*o)->getNum());
		_ascr->sendFvwmCmd(cmd, 0);
		sprintf(cmd,"+ \"Off\t%s\"    sma ExtendedInput %s Off\n",
			(!(*o)->isOn())? "o":"", (*o)->getName());
		_ascr->sendFvwmCmd(cmd, 0);
		sprintf(cmd,"+ \"Cursor\t%s\" sma ExtendedInput %s Cursor\n",
			((*o)->isCursor())? "o":"", (*o)->getName());
		_ascr->sendFvwmCmd(cmd, 0);
		sprintf(cmd,"+ \"ToolGlass\t%s\" Pick sma ExtendedInput %s "
			"ToolGlass",
			((*o)->isToolGlass())? "o":"", (*o)->getName());
		_ascr->sendFvwmCmd(cmd, 0);
	}
	if (i == 0)
	{
		_ascr->sendFvwmCmd("+ \"No Devices Detected\"", 0);
	}
	for(o = _devices.begin(); o != _devices.end(); o++)
	{
		if (!(*o)->hasValuators())
		{
			// for now
			continue;
		}
		if ((*o)->getToolGlassWin() != 0)
		{
			(*o)->getToolGlassWin()->buildSpecialMenu();
		}
	}
}

void ExtendedInputs::windowDestroyed(MetisseWindow *win)
{
	std::list<AExtDevice *>::iterator o;
	for(o = _devices.begin(); o != _devices.end(); o++)
	{
		(*o)->windowDestroyed(win);
	}
}

void ExtendedInputs::configure(MetisseWindow *win, char *name, char *opt)
{
	AExtDevice *aed = 0;

	if (name == 0 || opt == 0)
	{
		std::cerr << "[ExtendedInputs]: Bad Configuration" << std::endl;
		return;
	}

	std::list<AExtDevice *>::iterator o;
	for(o = _devices.begin(); o != _devices.end(); o++)
	{
		if ((*o)->getName() && strcmp((*o)->getName(), name) == 0)
		{
			aed = (*o);
			break;
		}
	}

	if (aed == 0)
	{
		return;
	}

	if (strcmp(opt, "Off") == 0 && aed->isOn())
	{
		aed->off();
	}
	else if (strcmp(opt, "Cursor") == 0)
	{
		aed->cursor();
	}
	else if (win != 0 && strcmp(opt, "ToolGlass") == 0)
	{
		aed->toolGlass(win);
	}
	_generateCursorsMenus();
}

int ExtendedInputs::handleEvents(glWindow::event e)
{
	std::list<AExtDevice *>::iterator o;
	for(o = _devices.begin(); o != _devices.end(); o++)
	{
		if (e.device_id == (*o)->getID() && (*o)->isOn())
		{
			return (*o)->handleEvents(e);
			break;
		}
	}

	return 0;
}

ExtendedInputs::ExtendedInputs(AScreen *ascr)
{
	_ascr = ascr;

	glWindow::extDevicesList *edl = _ascr->compositor->getExtensionDevices();
	if (edl)
	{
		int v_num = 1;
		int k_num = 1;
		for(std::list<glWindow::extensionDevice *>::iterator o =
			    edl->begin(); 
		    o != edl->end(); ++o)
		{
			fprintf(stderr,
				"Extended Input: %s (v: %i, k: %i)\n",
				(*o)->getName(),
				(*o)->hasValuators(), (*o)->hasKeys());
			if ((*o)->getName() == 0 ||
			    matchWildcards("Core*", (*o)->getName()))
			{
				std::cerr << "   Ignore this one (Core)\n";
				continue;
			}
			if ((*o)->hasKeys() || (*o)->hasValuators())
			{
				_devices.push_back(
					new AExtDevice(
						_ascr, this, (*o),
						((*o)->hasValuators())?
						v_num++:k_num++));
			}
			else
			{
				std::cerr << "   Ignore this one\n";
			}
		}
	}

	_generateCursorsMenus();
}
