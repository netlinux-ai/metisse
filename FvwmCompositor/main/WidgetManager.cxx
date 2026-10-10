/*
 *
 * main/WidgetManager.cxx --
 *
 * Copyright (C) Dusty Phillips
 * Copyright (C) Olivier Chapuis
 * Copyright (C) Nicolas Roussel
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config-not-xserver.h"
#endif

#include "WidgetManager.H"
#include "AScreen.H"

#include <nucleo/core/ReactiveEngine.H>

#include <iostream>
#include <vector>
//#include <sstream>
#include <stdexcept>

#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-application.h>
#include <libbonobo.h>
#include <cspi/spi.h>
#include <libspi/Accessibility.h>
#include <orbit/poa/poa.h>

// ----------------------------------------------------------------------------
//


bool AWidget::getExtents(long *x, long *y, long *w, long *h)
{
	if (_comp == 0)
	{
		return false;;
	}
	AccessibleComponent_getExtents(_comp, x, y, w, h, SPI_COORD_TYPE_SCREEN);
	return true;
}

bool AWidget::getValues(double *cur, double *min, double *max)
{
	if (_value == 0)
	{
		return false;
	}

	*cur = AccessibleValue_getCurrentValue(_value);
	*max = AccessibleValue_getMaximumValue(_value);
	*min = AccessibleValue_getMinimumValue(_value);

	return true;
}

bool AWidget::setValue(double val)
{
	if (_value == 0)
	{
		false;
	}

	return  AccessibleValue_setCurrentValue(_value, val);
}

char *AWidget::getName(void)
{
	// leak?
	if (_name != 0)
	{
		return _name;
	}
	_name =  Accessible_getName(_accessible);
	return _name;
}

char *AWidget::getAppName(void)
{
	if (_appName)
	{
		return _appName;
	}

	Accessible *parent;
	Accessible *child = _accessible;

	parent = child;
	while(parent && !Accessible_isApplication(parent))
	{
		if (child != _accessible)
		{
			Accessible_unref(child);
		}
		child = parent;
		parent = Accessible_getParent(child);
	}
	if (parent == 0)
	{
		return 0;
	}

	// leak ?
	_appName = Accessible_getName(parent);
	if (parent != _accessible)
	{
		Accessible_unref(parent);	
	}

	return _appName;
}

AWidget::AWidget(MetisseWindow *win, Accessible *accessible)
{
	_accessible = accessible;
	Accessible_ref(_accessible); // ? 
	_win = win;

	_comp = Accessible_getComponent(_accessible);
	_value = Accessible_getValue(_accessible);
	
	_appName = 0;
	_name = 0;
}

AWidget::~AWidget(void)
{
	if (_accessible)
	{
		Accessible_unref(_accessible);
	}
	if (_comp)
	{
		AccessibleValue_unref(_comp);
	}
	if (_value)
	{
		AccessibleValue_unref(_value);
	}
	if (_name)
	{
		SPI_freeString(_name);
	}
	if (_appName)
	{
		SPI_freeString(_appName);
	}
}

// -------------------------

void AWidgetValuator::setSpeed(double speed)
{
	_speed = speed;
}

double AWidgetValuator::getSpeed(void)
{
	return _speed;
}

void AWidgetValuator::setInaction(bool b)
{
	_inaction = b;
}

bool AWidgetValuator::getInaction(void)
{
	return _inaction;
}

AWidgetValuator::AWidgetValuator(MetisseWindow *win, Accessible *accessible)
	: AWidget(win, accessible)
{
	_speed = -1;
	_inaction = false;
}

// -----------------------

void AWidgetOrthoZoom::zoomIn(int count)
{
	//std::cerr << "zoomIn done" << "\n";
	bool fail = false;
	for (int i = 0; i < count; i++)
	{
		int k = 0;
		bool fail = false;
		while (!AccessibleAction_doAction(_zin, _inindex))
		{
			k++;
			//std::cerr << "Action Fail " << k << "\n";
			if (k > 10)
			{
				fail = true;
				break;
			}
			usleep(1000);
		}
		usleep(10);
		if (!fail)
		{
			_whatDone--;
		}
	}
	//std::cerr << "zoomIn done" << _whatDone << "\n";
}

void AWidgetOrthoZoom::zoomOut(int count)
{
	//std::cerr << "zoomOut\n";
	for (int i = 0; i < count; i++)
	{
		bool fail = false;
		int k = 0;
		while (!AccessibleAction_doAction(_zout, _outindex))
		{
			k++;
			//std::cerr << "Action Fail " << k << "\n";
			if (k > 10)
			{
				fail = true;
				break;
			}
			usleep(1000);
		}
		usleep(10);
		if (!fail)
		{
			_whatDone++;
		}
	}
	//std::cerr << "zoomOut done " << _whatDone << "\n";
}

void AWidgetOrthoZoom::start(void)
{
	//std::cerr << "start\n";
	_whatDone = 0;
}

bool AWidgetOrthoZoom::doaction(int dist)
{
	// FIXME: assume a right scrollbar
	dist = abs(dist/15);

	if (dist >= 15)
	{
		dist = 15;
	}

	//std::cerr << "doaction " << dist << "\n";
	if (dist > _whatDone)
	{
		zoomOut(dist - _whatDone);
	}
	else if (dist < _whatDone)
	{
		zoomIn(_whatDone - dist);
	}
	else
	{
		return false;
	}

	return true;
}

void AWidgetOrthoZoom::end(void)
{
	//std::cerr << "end " <<  _whatDone<< "\n";
	if (_whatDone > 0)
	{
		zoomIn(_whatDone);
	}
	else if (_whatDone < 0)
	{
		zoomIn(-_whatDone);
	}
}


AWidgetOrthoZoom::AWidgetOrthoZoom(
	MetisseWindow *win, Accessible *accessible,
	AccessibleAction *zin, int inindex, 
	AccessibleAction *zout, int outindex)
	: AWidgetValuator(win, accessible)
{
	_zin = zin;
	_inindex = inindex;
	_zout = zout;
	_outindex = outindex;
	_whatDone = 0;
}

AWidgetOrthoZoom::~AWidgetOrthoZoom(void)
{
	AccessibleAction_unref(_zin);
	AccessibleAction_unref(_zout);
}

// -----------------------------------------------------------------------------
//

// WARNING: parental relation does not imply a geometrical relation!?!
// An other problems is that when a menu as been pop-up it is still in
// the list of visible/showing widget!
Accessible *WidgetManager::_findInnerChild(Accessible *child, int x, int y)
{
	std::vector<Accessible*> containing_children;
	AccessibleComponent *child_as_component;
	Accessible *contained_child;
	Accessible *new_child;
	
	int numchildren = Accessible_getChildCount(child);
	if (numchildren == -1)
	{
		return 0;
	}
	if (Accessible_getRole(child) == SPI_ROLE_MENU)
	{
		// FIXME!!
		numchildren = 0; 
	}
	for (int i = 0; i < numchildren; i++)
	{
		new_child = Accessible_getChildAtIndex(child, i);
		if (new_child == 0)
		{
			continue;
		}
		contained_child = _findInnerChild(new_child, x, y);
		if (contained_child != 0)
		{
			containing_children.push_back(contained_child);
		}
		if (contained_child != new_child)
		{
			Accessible_unref(new_child);
		}
	}
	if (containing_children.size() == 0)
	{
		child_as_component = Accessible_getComponent(child);
		if (child_as_component != 0 &&
		    AccessibleComponent_contains(
			    child_as_component, x, y, SPI_COORD_TYPE_SCREEN))
		{
			AccessibleComponent_unref(child_as_component);
			return child;
		}
		if (child_as_component)
		{
			AccessibleComponent_unref(child_as_component);
		}
		return 0;
	}
	else if (containing_children.size() == 1)
	{
		return containing_children[0];
	}
	else
	{
		#if 0
		std::cout << "WidgetManager: "
			  << "I don't think you wanted to see this line"
			  << std::endl;
		#endif
		for (int i = 1; i < containing_children.size(); i++)
		{
			Accessible_unref(containing_children[i]);
		}
		return containing_children[0];
	}
}

void WidgetManager::_printWidget(Accessible *child, int l)
{
	// FIXME: string leak
	std::cerr << "<Widget level=" << l
		  << " name='" << Accessible_getName(child) << "'"
		  << " role='" << Accessible_getRoleName(child)  << "'>\n";

	AccessibleApplication *app = Accessible_getApplication(child);
	if (app != 0)
	{
		std::cerr << "<application id="
			  << AccessibleApplication_getID(app);
		char *tn = AccessibleApplication_getToolkitName(app);
		if (tn)
		{
			std::cerr << " toolkit='" << tn << "'";
			//SPI_freeString(tn);
		}
		else
		{
			std::cerr << " toolkit=Unknown";
		}
		char *ver = AccessibleApplication_getVersion(app);
		if (ver)
		{
			std::cerr << " version='" << ver << "'";
		}
		else
		{
			std::cerr << " toolkit=Unknown";
		}
		std::cerr << "</>\n";
		AccessibleApplication_unref(app);
		
	}

	AccessibleComponent *comp = Accessible_getComponent(child);
	if (comp != 0)
	{
		long x,y,w,h;
		AccessibleComponentLayer layer;

		AccessibleComponent_getExtents(
			comp, &x, &y, &w, &h, SPI_COORD_TYPE_WINDOW);
		layer = AccessibleComponent_getLayer(comp);
		std::cerr << "<component x=" << x << " y=" << y
			  << " w=" << w << " h=" << h
			  << " layer=" << layer << "/>\n";
		AccessibleComponent_unref(comp);
	}

	AccessibleAction *action =  Accessible_getAction(child);
	if (action)
	{
		long n = AccessibleAction_getNActions(action);
		for (long i = 0; i < n; i++)
		{
			std::cerr << "<action num=" << i;
			char *key = AccessibleAction_getKeyBinding(action, i);
			char *aname = AccessibleAction_getName(action, i);
			char *adesc = AccessibleAction_getDescription(action, i);
			if (aname)
			{
				std::cerr << " name='" << aname << "'";
				SPI_freeString(aname);
			}
			else
			{
				std::cerr << " name=Unknown";
			}
			if (adesc)
			{
				std::cerr << " desc='" << adesc << "'";
				SPI_freeString(adesc);
			}
			else
			{
				std::cerr << " desc=Unknown";
			}
			if (key)
			{
				std::cerr << " key='" << key << "'";
				SPI_freeString(key);
			}
			else
			{
				std::cerr << " key=None";
			}
			std::cerr << "/>\n";
		}
		AccessibleAction_unref(action);
	}

	AccessibleValue *val = Accessible_getValue(child);
	if (val)
	{
		double cur = AccessibleValue_getCurrentValue(val);
		double max = AccessibleValue_getMaximumValue(val);
		double min = AccessibleValue_getMinimumValue(val);
		std::cerr << "<value min=" << min
			  << " max=" << max
			  << " cur=" << cur
			  << "/>\n";
		AccessibleValue_unref(val);
	}

	AccessibleStateSet *ass =  Accessible_getStateSet(child);
	if (ass)
	{
		bool selected = 
			AccessibleStateSet_contains(ass,SPI_STATE_SELECTED);
		std::cerr << "<State selected=" << selected
			  << "/>\n";
		AccessibleStateSet_unref(ass);
	}
	#if 1
	AccessibleText *text = Accessible_getText(child);
	if (text)
	{
		char *t = AccessibleText_getText(text,0, 50);
		std::cerr << "<HasText " << t << "/>\n";
		AccessibleText_unref (text);
	}
	#endif
	#if 0
	AccessibleHypertext *ht = Accessible_getHypertext(child);
	if (ht)
	{
		int N = AccessibleHypertext_getNLinks(ht);
		int i;
		std::cerr << "<Hyperlink ";
		for (i = 0; i < N; i++)
		{
			AccessibleHyperlink *ahl = AccessibleHypertext_getLink(
				ht, i);
			char *uri = AccessibleHyperlink_getURI(ahl,0);
			if (uri)
			{
				std::cerr << uri << ":";	
			}
		}
		std::cerr << "/>\n";
		AccessibleHypertext_unref(ht);
	}
	#endif
	if (l < 0)
	{
		std::cerr << "</Widget>" << std::endl;
	}
}

void WidgetManager::_printWidgetRec(Accessible *child, int l)
{
	_printWidget(child,l);
	l++;
	int numchildren = Accessible_getChildCount(child);
	for (int i = 0; i < numchildren; i++)
	{
		Accessible *new_child;

		new_child = Accessible_getChildAtIndex(child, i);
		if (new_child)
		{
			_printWidgetRec(new_child,l);
			Accessible_unref(new_child);
		}
	}
	l--;
	std::cerr << "</Widget>" << std::endl;
}

void WidgetManager::_getSelectedIcons(
	Accessible *child, std::list<std::string *> *paths,
	std::list<std::string *> *icons)
{
	int numchildren = Accessible_getChildCount(child);
	if (numchildren == -1)
	{
		return;
	}
	if (Accessible_getRole(child) == SPI_ROLE_ICON)
	{
		AccessibleStateSet *ass =  Accessible_getStateSet(child);
		if (ass && AccessibleStateSet_contains(ass,SPI_STATE_SELECTED))
		{
			char *name = Accessible_getName(child);
			icons->push_front(new std::string(name));
			//std::cerr << "ICON " << name << " SELECTED\n";
			// FIXME: free name ?
		}
		if (ass)
		{
			AccessibleStateSet_unref(ass);
		}
	}
	AccessibleText *text = Accessible_getEditableText(child);
	if (text)
	{
		char *t = AccessibleText_getText(text,0, 1024);
		if (t && t[0] == '/')
		{
			paths->push_front(new std::string(t));
		}
		AccessibleText_unref (text);
	}
	for (int i = 0; i < numchildren; i++)
	{
		Accessible *new_child;

		new_child = Accessible_getChildAtIndex(child, i);
		if (new_child)
		{
			_getSelectedIcons(new_child, paths, icons);
			Accessible_unref(new_child);
		}
	}

}

void WidgetManager::_getInnerAAction(
	Accessible *child, std::list<AAction *> *aa)
{
	int numchildren = Accessible_getChildCount(child);
	if (numchildren == -1)
	{
		return;
	}
	if (numchildren == 0)
	{
		AccessibleAction *action = Accessible_getAction(child);
		if (action != 0)
		{
			int index = -1;
			char *aname, *adesc;
			long n = AccessibleAction_getNActions(action);
			for (long i = 0; i < n; i++)
			{
				// FIXME: string leaks
				aname = AccessibleAction_getName(action, i);
				adesc = AccessibleAction_getDescription(
					action, i);
				if (!StrEquals(aname,"click"))
				{
					continue;
				}
				if (adesc == 0 || StrEquals(adesc,""))
				{
					adesc = Accessible_getName(child);	
				}
				if (adesc == 0 || StrEquals(adesc,""))
				{
					continue;
				}
				index = i;
				break;
			}
			if (index >= 0)
			{
				aa->push_front(
					new AAction(
						new std::string(adesc), action,
						index));
			}
			else
			{
				AccessibleAction_unref(action);	
			}
		}
	}
	for (int i = 0; i < numchildren; i++)
	{
		Accessible *new_child;

		new_child = Accessible_getChildAtIndex(child, i);
		if (new_child)
		{
			_getInnerAAction(new_child, aa);
			if (new_child != child)
			{
				Accessible_unref(new_child);
			}
		}
	}
}

void WidgetManager::_getAllValuators(
	Accessible *child, MetisseWindow *win, std::list<AWidgetValuator *> *vlist)
{
	int numchildren = Accessible_getChildCount(child);
	if (numchildren == -1)
	{
		return;
	}
	if (numchildren == 0)
	{
		AccessibleValue *value = Accessible_getValue(child);
		if (value != 0)
		{
			vlist->push_front(new AWidgetValuator(win,child));
		}
		AccessibleValue_unref(value);		
	}
	for (int i = 0; i < numchildren; i++)
	{
		Accessible *new_child;

		new_child = Accessible_getChildAtIndex(child, i);
		if (new_child)
		{
			_getAllValuators(new_child, win, vlist);
			if (new_child != child)
			{
				Accessible_unref(new_child);
			}
		}
	}
}

void WidgetManager::_getAllAccessibleAction(
	Accessible *child, std::list<AccessibleAction *> *aa)
{
	int numchildren = Accessible_getChildCount(child);
	if (numchildren == -1)
	{
		return;
	}
	AccessibleAction *action = Accessible_getAction(child);
	if (action != 0)
	{
		aa->push_front(action);
	}
	for (int i = 0; i < numchildren; i++)
	{
		Accessible *new_child;

		new_child = Accessible_getChildAtIndex(child, i);
		if (new_child)
		{
			_getAllAccessibleAction(new_child, aa);
			if (new_child != child)
			{
				Accessible_unref(new_child);
			}
		}
	}
}


void WidgetManager::_getZoomInZoomOut(
	Accessible *child, AccessibleAction **zin, int *inindex,
	AccessibleAction **zout, int *outindex)
{
	std::list<AccessibleAction *> aa;
	_getAllAccessibleAction(child, &aa);

	std::list<AccessibleAction *>::iterator iter;
	*zin = 0;
	*zout = 0;
	for (iter = aa.begin(); iter != aa.end(); iter++)
	{
		Bool unref = true;
		AccessibleAction *action = (*iter);
		long n = AccessibleAction_getNActions(action);
		for (long i = 0; i < n; i++)
		{
			char *key = AccessibleAction_getKeyBinding(action, i);
			if (key && matchWildcards("*plus*", key))
			{
				//std::cerr << "found zin\n";
				*inindex = i;
				*zin = action;
				unref = false;
				SPI_freeString(key);
				if (*zout != 0)
				{
					break;
				}
				unref = false;;
				continue;
			}
			else if (key && matchWildcards("*minus*", key))
			{
				//std::cerr << "found zout\n";
				*outindex = i;
				*zout = action;
				unref = false;
				SPI_freeString(key);
				if (*zin != 0)
				{
					break;
				}
				continue;
			}
			else if (key)
			{
				SPI_freeString(key);
			}
		}
		if (unref)
		{
			AccessibleAction_unref(action);
		}
	}
}
 
void WidgetManager::_getAllRegionsRec(
	Accessible *child, std::list<WinRegions::region > *reg,
	int px, int py, int pw, int ph)
{
	long x,y,w,h;

	AccessibleStateSet *ass =  Accessible_getStateSet(child);
	AccessibleComponent *comp = Accessible_getComponent(child);
	if (comp != NULL)
	{
		AccessibleStateSet *ass =  Accessible_getStateSet(child);
		if (!ass || !AccessibleStateSet_contains(ass,SPI_STATE_SHOWING)
		    || !AccessibleStateSet_contains(ass,SPI_STATE_VISIBLE))
		{
			if (ass)
			{
				AccessibleStateSet_unref(ass);
			}
			return;
		}
		AccessibleStateSet_unref(ass);
		AccessibleComponent_getExtents(
			comp, &x, &y, &w, &h, SPI_COORD_TYPE_SCREEN);
		AccessibleComponent_unref(comp);
		if (w > 0 && h > 0)
		{
			if (x != px || y != py || w != pw || h != ph)
			{
				WinRegions::region r =
					WinRegions::region(x,y,w,h);
				reg->push_back(r);
			}
		}
		else
		{
			x = px; y = py; w = pw; h = ph;
			//return;
		}
	}
	else
	{
		x = px; y = py; w = pw; h = ph;
		//return;
	}

	if (Accessible_getRole(child) == SPI_ROLE_MENU)
	{
		// FIXME!!
		return;
	}
	int numchildren = Accessible_getChildCount(child);
	for (int i = 0; i < numchildren; i++)
	{
		Accessible *new_child;

		new_child = Accessible_getChildAtIndex(child, i);
		if (new_child)
		{
			_getAllRegionsRec(new_child, reg, x, y, w, h);
			Accessible_unref(new_child);
		}
	}
}

Accessible *WidgetManager::_getTopLevelAccessible(
	MetisseWindow *win, Accessible **app, int *x, int *y, int *w, int *h)
{
	Accessible *desktop = NULL, *application = NULL, *ret_child = NULL,
		*child = NULL;
	int numapps, numchildren;
	
	if (win == 0)
	{
		return 0;
	}

	int wx,wy,wh=-1,ww=-1;
	win->getSize(&ww, &wh);
	win->getRealPosition(&wx, &wy);
	
	desktop = SPI_getDesktop(0); // FIXME ... in fact not a pb for now
	numapps = Accessible_getChildCount(desktop);

	#if 0
	fprintf(stderr,"numapps %i: ", numapps);
	#endif
	for (int i = 0; i < numapps; i++)
	{
		#if 0
		fprintf(stderr,"app %i", i);
		#endif
		application = Accessible_getChildAtIndex(desktop, i);
		#if 0
		fprintf(stderr,",");
		#endif
		numchildren = Accessible_getChildCount(application);
		#if 0
		fprintf(stderr,"%i ",numchildren);
		#endif
		for (int j = 0; j < numchildren; j++)
		{
			//_printWidget(application);
			child = Accessible_getChildAtIndex(application, j);
			// Should be a top level window
			// SPI_ROLE_{FRAME,WINDOW,DIALOG...}
			long int fx,fy,fw,fh;
			AccessibleComponent *comp =
				Accessible_getComponent(child);
			if (comp == NULL)
			{
				if (child)
				{
					Accessible_unref(child);
				}
				continue;
			}
			AccessibleComponent_getExtents(
				comp, &fx, &fy, &fw, &fh, SPI_COORD_TYPE_SCREEN);
			AccessibleComponent_unref(comp);
			#if 0
			std::cerr << "Top level "
				  << Accessible_getName(child) << " "
				  << Accessible_getRoleName(child) << " "
				  << fx << " " << fy << " " << fw << " "
				  << fh << " " << std::endl;
			#endif
			if ((fh > 0 && fw > 0) &&
			    (fx == wx && fy == wy && fw == ww && fh == wh))
			{
				*x = fx;
				*y = fy;
				*w = fw;
				*h = fh;
				*app = application;
				ret_child = child;
				goto end;
			}
			else
			{
				#if 0
				std::cerr << "Ignored" << std::endl;
				#endif
			}
			if (child)
			{
				Accessible_unref(child);
			}
		}
	}

end:
	#if 0
	fprintf(stderr,"\n");
	#endif
	if (app == NULL && application != NULL)
	{
		Accessible_unref(application);
	}
	if (desktop != NULL)
	{
		Accessible_unref(desktop);
	}

	return ret_child;
}

Accessible *WidgetManager::_getTopLevelAccessible(
	MetisseWindow *win, int *x, int *y, int *w, int *h)
{
	Accessible *app = NULL;
	Accessible *ret;

	ret = _getTopLevelAccessible(win, &app, x, y, w, h);
	if (app)
	{
		Accessible_unref(app);
	}
	return ret;
}

Accessible *WidgetManager::_getTopLevelAccessible(MetisseWindow *win)
{
	int x, y, w, h;
	Accessible *app = NULL;
	Accessible *ret;

	ret = _getTopLevelAccessible(win, &app, &x, &y, &w, &h);
	if (app)
	{
		Accessible_unref(app);
	}
	return ret;
}

void WidgetManager::getWidgetRegions(
	MetisseWindow *win, std::list<WinRegions::region> &reg)
{
	int px,py,pw,ph;
	Accessible *child = _getTopLevelAccessible(win,&px,&py,&pw,&ph);

	if (child == NULL)
	{
		return;
	}

	_getAllRegionsRec(child, &reg, px, py, pw, ph);

	Accessible_unref(child);

	#if 0
	for (std::list<WinRegions::region >::iterator i=reg.begin();
		i!= reg.end(); ++i) {
		std::cerr << (*i).x << " " << (*i).y << " "
			  << (*i).w << " " << (*i).h
			  << std::endl;
	}
	#endif
}

void WidgetManager::printWidgets(MetisseWindow *win, int verbose)
{
	int px,py,pw,ph;
	Accessible *app = 0;
	Accessible *child = _getTopLevelAccessible(win,&app,&px,&py,&pw,&ph);

	if (child != 0)
	{
		_printWidget(app,0);
		_printWidgetRec(child,1);
		std::cerr << "</Widget>\n";
		Accessible_unref(child);
	}
	if (app != 0)
	{
		Accessible_unref(app);
	}
}

void WidgetManager::getSelectedIcons(
	MetisseWindow *win, std::list<std::string *> *paths,
	std::list<std::string *> *icons)
{
  	int px,py,pw,ph;
	Accessible *app = 0;
	Accessible *child = _getTopLevelAccessible(win,&app,&px,&py,&pw,&ph);


	if (child != 0)
	{
		_getSelectedIcons(child, paths, icons);
	}
}

/* attempts to locate the widget at the position indicated by point x,
y and return the boundaries of that widget. Returns the extent, the x,
y position of the upper left corner and the width and height of the
widget.
*/
WinRegions::region WidgetManager::getExtent(MetisseWindow *win, int x, int y)
{
	Accessible *child;
	Accessible *inner_child = NULL;
	AccessibleComponent *inner_child_comp = NULL;
	WinRegions::region ret_val = WinRegions::region(-1,-1,-1,-1);

	child = _getTopLevelAccessible(win);
	if (child)
	{
		inner_child = _findInnerChild(child, x, y);
		if (inner_child != child)
		{
			Accessible_unref(child);
		}
	}
	if (inner_child == 0)
	{
		ret_val.x = -1;
		ret_val.y = -1;
		ret_val.w = -1;
		ret_val.h = -1;
	}
	else
	{
		long int x,y,w,h;
		inner_child_comp = Accessible_getComponent(inner_child);
		if (inner_child_comp)
		{
			AccessibleComponent_getExtents(
				inner_child_comp, &x, &y, &w, &h,
				SPI_COORD_TYPE_SCREEN);
			AccessibleComponent_unref(inner_child_comp);
		}
		ret_val.x = (double)x;
		ret_val.y = (double)y;
		ret_val.w = (double)w;
		ret_val.h = (double)h;
		Accessible_unref(inner_child);
	}

	return ret_val;
}

AWidgetValuator *WidgetManager::getValuatorAtPoint(
	MetisseWindow *win, int x, int y)
{
	Accessible *child;
	Accessible *inner_child = NULL;
	AccessibleAction *zin, *zout;
	int inindex, outindex;

	child = _getTopLevelAccessible(win);
	if (child)
	{
		// FIXME: found the first widget with values at x,y
		inner_child = _findInnerChild(child, x, y);
		_getZoomInZoomOut(child, &zin, &inindex, &zout, &outindex);
		if (inner_child != child)
		{
			Accessible_unref(child);
		}
	}
	else
	{
		return 0;
	}

	if (inner_child == 0 || !Accessible_isValue(inner_child))
	{
		if (inner_child)
		{
			Accessible_unref(inner_child);
		}
		if (zin != 0)
		{
			AccessibleAction_unref(zin);
		}
		if (zout != 0)
		{
			AccessibleAction_unref(zout);
		}
		return 0;
	}


	if (zin != 0 && zout != 0)
	{
		return new AWidgetOrthoZoom(
			win, inner_child, zin, inindex, zout, outindex);
	}
	if (zin != 0)
	{
		AccessibleAction_unref(zin);
	}
	if (zout != 0)
	{
		AccessibleAction_unref(zout);
	}
	return  new AWidgetValuator(win, inner_child);
}

std::list<AWidgetValuator *>WidgetManager::getAllValuators(MetisseWindow *win)
{
	Accessible *child;
	Accessible *inner_child = NULL;
	std::list<AWidgetValuator *> vlist;

	child = _getTopLevelAccessible(win);
	if (child)
	{
		// FIXME: found the first widget with values at x,y
		_getAllValuators(child, win, &vlist);
		Accessible_unref(child);
	}

	return vlist;
}

AWidget *WidgetManager::getAWidgetAtPoint(MetisseWindow *win, int x, int y)
{
	Accessible *child;
	Accessible *inner_child = NULL;
	AccessibleAction *zin = 0, *zout = 0;
	int inindex, outindex;

	child = _getTopLevelAccessible(win);
	if (child)
	{
		// FIXME: found the first widget with values at x,y
		inner_child = _findInnerChild(child, x, y);
		if (inner_child == 0)
		{
			Accessible_unref(child);
			return 0;
		}
	}
	else
	{
		return 0;
	}

	bool found = false;
	if (Accessible_isValue(inner_child))
	{
		_getZoomInZoomOut(child, &zin, &inindex, &zout, &outindex);
		found = true;
	}
	else if (Accessible_getRole(inner_child) == SPI_ROLE_COMBO_BOX)
	{
		found = true;
	}

	if (inner_child != child)
	{
		Accessible_unref(child);
	}

	if (!found)
	{
		Accessible_unref(inner_child);
		return 0;
	}


	if (zin != 0 && zout != 0)
	{
		return new AWidgetOrthoZoom(
			win, inner_child, zin, inindex, zout, outindex);
	}
	if (zin != 0)
	{
		AccessibleAction_unref(zin);
	}
	if (zout != 0)
	{
		AccessibleAction_unref(zout);
	}

	if (Accessible_isValue(inner_child))
	{
		return  new AWidgetValuator(win, inner_child);
	}
	else if (Accessible_getRole(inner_child) == SPI_ROLE_COMBO_BOX)
	{
		return  new AWidget(win, inner_child);
	}
	return 0;
}

void WidgetManager::buildActionMenus(MetisseWindow *win)
{
	Accessible *child;
	child = _getTopLevelAccessible(win);

	if (child == 0)
	{
		return;
	}
	std::list<AAction *> aa;
	_getInnerAAction(child, &aa);

	std::list<AAction *>::iterator iter;
	std::cerr << "Nbr Of Action " << aa.size()<< "\n";
	for (iter =  aa.begin(); iter != aa.end(); iter++)
	{
		std::cerr << (*iter)->descr->c_str() << "\n";
	}
}
 
Accessible *WidgetManager::registerWindow(MetisseWindow *win)
{
	Accessible *acc;
	acc = _getTopLevelAccessible(win);

	return acc;
}

void WidgetManager::unregisterWindow(MetisseWindow *win)
{
	Accessible *acc;
	acc = win->getTopAccessible();
	if (acc)
	{
		//std::cerr << "unresiterWindow\n";
		Accessible_unref(acc);
	}
}

static
void accessible_event_callback(
	const AccessibleEvent *event, void *user_data)
{
	static long count = 0;
	char *s = Accessible_getRoleName (event->source);
	fprintf (stderr, "accessible_event_callback: %s %s\n", event->type, s);
	if (s) SPI_freeString (s);
}

WidgetManager::WidgetManager(AScreen *ascr, MetisseDesktop *wd)
{
	char *compositor_display = getenv("DISPLAY") ;
	char *server_display = wd->getMetisseServerDisplay() ;
#if 1
	std::cerr << "WidgetManager: compositor display is " 
		  << (compositor_display?compositor_display:"???") << std::endl ;
	std::cerr << "WidgetManager: server display is " 
		  << (server_display?server_display:"???") << std::endl ;
#endif

	if (server_display) setenv("DISPLAY",server_display,1) ;
	setenv("AT_SPI_DEBUG","256",1) ;

	// std::cerr << __FILE__ << " (" << __LINE__ << "): SPI_init" << std::endl ;
	if (SPI_init() != 0)
	{
		setenv("DISPLAY",compositor_display?compositor_display:"",1);
		throw std::runtime_error(
			"WidgetManager: SPI initialization fail");
	}

	std::cerr << __FILE__ << " (" << __LINE__ << "): SPI_init ok"
		  << std::endl ;
	std::cerr << __FILE__ << " (" << __LINE__ << "): bonobo_init"
		  << std::endl ;

	if (bonobo_init(0, NULL) == FALSE)
	{
		setenv("DISPLAY",compositor_display?compositor_display:"",1);
		throw std::runtime_error("WidgetManager: Can not bonobo_init");
	}

	std::cerr << __FILE__ << " (" << __LINE__ << "): bonobo_init ok"
		  << std::endl ;

	bonobo_activate();

	CORBA_Environment ev ;
	CORBA_exception_init(&ev);
	Bonobo_Unknown ref = bonobo_get_object(
		"OAFIID:Accessibility_Registry:1.0",
		"Accessibility/Registry", &ev);

	if (ref == CORBA_OBJECT_NIL)
		throw std::runtime_error(
			"WidgetManager: can not get Accessibility Registry");

#if 0
	// Does not work (this leads to some looking ... any way I do not think
	// that we need that: when needed query the at-spi interface!
	AccessibleEventListener *_eventListener =
		SPI_createAccessibleEventListener(
			accessible_event_callback, this);
	SPI_registerGlobalEventListener (
		_eventListener,"object:visible-data-changed");
#endif

	_registry = (Accessibility_Registry)ref;
	_ascr = ascr;

	setenv("DISPLAY",compositor_display?compositor_display:"",1) ;
}

WidgetManager::~WidgetManager(void)
{
	int r = SPI_exit();

	if (r != 0)
	{
		std::cerr << "WidgetManager: there were some leaks: "
			  << r << std::endl;
	}
}
