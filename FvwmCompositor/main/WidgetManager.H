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
#ifndef WidgetManager_H
#define WidgetManager_H

#include <string>
#include <iostream>
#include <list>
#include <vector>

#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-application.h>
#include <libbonobo.h>
#include <cspi/spi.h>
#include <libspi/Accessibility.h>
#include <orbit/poa/poa.h>

#include "renderer/WinRegions.H"

class AScreen;
class MetisseWindow;
class MetisseDesktop;

struct AAction
{
	std::string *descr;
	AccessibleAction *action;
	int index;
	AAction(std::string *d, AccessibleAction *a, int i) :
		descr(d), action(a), index(i) {}
};

class AWidget
{
private:
	MetisseWindow *_win;
	Accessible *_accessible;
	AccessibleComponent *_comp;
	AccessibleValue *_value;
	char *_appName;
	char *_name;
	//char *name;
	//int _role;
	//WinRegions::region _pos;

	//bool _hasValue;
	//double _cur;
	//double _max;
	//double _min;

public:
	bool getExtents(long *x, long *y, long *w, long *h);
	bool getValues(double *cur, double *min, double *max);
	bool setValue(double val);
	char *getName(void);
	char *getAppName(void);
	AWidget(MetisseWindow *win, Accessible *accessible);
	virtual ~AWidget(void);
};

class AWidgetValuator : public AWidget
{
private:
	double _speed;
	bool _inaction;
public:
	double getSpeed(void);
	void setSpeed(double speed);
	void setInaction(bool b);
	bool getInaction(void);
	AWidgetValuator(MetisseWindow *win, Accessible *accessible);
};

class AWidgetOrthoZoom : public AWidgetValuator
{
private:
	AccessibleAction *_zin;
	int _inindex;
	AccessibleAction *_zout;
	int _outindex;
	int _whatDone;

public:
	void zoomIn(int count);
	void zoomOut(int count);
	void start(void);
	bool doaction(int dist);
	void end(void);
	AWidgetOrthoZoom(
		MetisseWindow *win, Accessible *accessible,
		AccessibleAction *zin, int inindex, 
		AccessibleAction *zout, int outindex);
	~AWidgetOrthoZoom(void);
};

class WidgetManager
{
private:
	Accessibility_Registry _registry;
	AccessibleEventListener *_eventListener;
	AScreen *_ascr;
	MetisseDesktop *msDesktop;

	Accessible *_findInnerChild(Accessible *child, int x, int y);
	void _printWidget(Accessible *child, int l = 0);
	void _printWidgetRec(Accessible *child, int l);
	void _getSelectedIcons(
		Accessible *child, std::list<std::string *>  *paths,
		std::list<std::string *> *icons);
	void _getAllValuators(
		Accessible *child, MetisseWindow *win,
		std::list<AWidgetValuator *> *vlist);
	void _getInnerAAction(Accessible *child, std::list<AAction *> *aa);
	void _getAllAccessibleAction(
		Accessible *child, std::list<AccessibleAction *> *aa);
	void _getZoomInZoomOut(
		Accessible *child, AccessibleAction **zin, int *inindex,
		AccessibleAction **zout, int *outindex);
	void _getAllRegionsRec(
		Accessible *child, std::list<WinRegions::region > *reg,
		int px, int py, int pw, int ph);
	Accessible *_getTopLevelAccessible(
		MetisseWindow *win, Accessible **app,
		int *x, int *y, int *w, int *h);
	Accessible *_getTopLevelAccessible(
		MetisseWindow *win, int *x, int *y, int *w, int *h);
	Accessible *_getTopLevelAccessible(MetisseWindow *win);

public:
	void getWidgetRegions(
		MetisseWindow *win, std::list<WinRegions::region> &regionlist);
	void printWidgets(MetisseWindow *win, int verbose);
	void getSelectedIcons(
		MetisseWindow *win, std::list<std::string *> *paths,
		std::list<std::string *> *icons);
	WinRegions::region getExtent(MetisseWindow *win, int x, int y);
	AWidgetValuator *getValuatorAtPoint(MetisseWindow *win, int x, int y);
	std::list<AWidgetValuator *> getAllValuators(MetisseWindow *win);
	AWidget *getAWidgetAtPoint(MetisseWindow *win, int x, int y);
	void buildActionMenus(MetisseWindow *win);
	Accessible *registerWindow(MetisseWindow *win);
	void unregisterWindow(MetisseWindow *win);
	WidgetManager(AScreen *ascr, MetisseDesktop *wd);
	~WidgetManager(void);
};

#endif
