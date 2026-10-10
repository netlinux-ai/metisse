/*
 *
 * fvwm/FvwmModule.cxx
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

#include "FvwmModule.H"

#include <nucleo/core/ReactiveEngine.H>
#include <nucleo/utils/FileUtils.H>
#include <nucleo/utils/StringUtils.H>

#include <X11/Xlib.h>

#include "desktop/MetisseDesktop.H"
#include "desktop/MetisseWindow.H"
#include "main/AScreen.H"
#include "main/Persistence.H"

#include "AFvwm.H"

#include <unistd.h>
#include <stdio.h>

void FvwmModule::parseOptions(void)
{
	char *tline= NULL;
	int desk;
	int dx = 3;
	int dy = 3;

	std::cerr << "Parse Options 1" << std::endl;
	InitGetConfigLine(_fd,CatString3("*","FvwmAmetsita",0));

	std::cerr << "Parse Options 2" << std::endl;
	for (GetConfigLine(_fd,&tline); tline != NULL; GetConfigLine(_fd,&tline))
	{
		    int g_x, g_y, flags;
		    unsigned width,height;
		    char *resource;
		    char *arg1;
		    char *arg2;
		    char *tline2;
		    char *token;
		    char *next;

		    arg1 = arg2 = NULL;

		    token = PeekToken(tline, &next);
		    std::cerr << tline << std::endl;

		    if (StrEquals(token, "Colorset"))
		    {
			    continue;
		    }
		    else if (StrEquals(token, XINERAMA_CONFIG_STRING))
		    {
			    continue;
		    }
		    else if (StrEquals(token, "DesktopSize"))
		    {
			    token = PeekToken(next, &next);
			    if (token)
			    {
				    sscanf(token, "%d", &dx);
				    token = PeekToken(next, &next);
				    if (token)
				    {
					    sscanf(token, "%d", &dy);
				    }
			    }
			    continue;
		    }
		    else if (StrEquals(token, "ImagePath"))
		    {
			    continue;
		    }
		    else if (StrEquals(token, "MoveThreshold"))
		    {
		    }
		    else if (StrEquals(token, "DesktopName"))
		    {
			    // may be useful
			    continue;
		    }

		    tline2 = GetModuleResource(tline, &resource, "FvwmCompositor");
		    if (!resource)
		    {
			    continue;
		    }
		    tline2 = GetNextToken(tline2, &arg1);
		    if (!arg1)
		    {
			    arg1 = (char *)safemalloc(1);
			    arg1[0] = 0;
		    }
		    tline2 = GetNextToken(tline2, &arg2);
		    if (!arg2)
		    {
			    arg2 = (char *)safemalloc(1);
			    arg2[0] = 0;
		    }
		    
		    if(StrEquals(resource,"AnOption"))
		    {
		    }
		    else if(StrEquals(resource,"An Other Option ..."))
		    {
		    }
		    // etc
		    free(resource);
		    free(arg1);
		    free(arg2);
	}	
}

void FvwmModule::_processOperation(MetisseWindow *mWin, char *rest)
{
	char *op = NULL;
	char *dummy;
	int drag_rotate = DRAG_ROTATE_NO;


	rest = GetNextToken(rest, &op);

	int val[2];
	if (!rest || GetIntegerArguments(rest, &rest, val, 2) != 2)
	{
		/* should not happen */
		return;
	}

	if (StrEquals(op, "Fold"))
	{
		_ascr->fold(mWin, val[0], val[1]);
	}
	else if (StrEquals(op,"SelectWinRegion"))
	{
		_ascr->selectRegion(mWin, val[0], val[1], false);
	}
	else if (StrEquals(op,"AddSelectWinRegion"))
	{
		_ascr->selectRegion(mWin, val[0], val[1], true);
	}
	else if (StrEquals(op, "Move") || StrEquals(op, "TmpMove"))
	{
		float alpha = 0.7;
		bool tmp = false;

		if (StrEquals(op, "TmpMove"))
		{
			tmp = true;
		}

		if (rest)
		{
			char *a;
			a = PeekToken(rest, &rest);
			if (a && sscanf(a,"%f",&alpha))
			{
				if (alpha > 1.0)
					alpha = 1.0;
				else if (alpha < 0.1)
					alpha = 0.1;
			}
			else
			{
				alpha = 0.7;
			}
		}
		_ascr->startInternalMove(
			mWin, val[0], val[1], alpha, tmp);
	}
	else if (StrEquals(op, "InteractiveScale") ||
		 StrEquals(op, "TmpInteractiveScale"))
	{
		float alpha = 0.7;
		bool tmp = false;

		if (StrEquals(op, "TmpInteractiveScale"))
		{
			tmp = true;
			alpha = 1.0;
		}

		if (rest)
		{
			char *a;
			a = PeekToken(rest, &rest);
			if (a && sscanf(a,"%f",&alpha))
			{
				if (alpha > 1.0)
					alpha = 1.0;
				else if (alpha < 0.1)
					alpha = 0.1;
			}
			else
			{
				alpha = 0.7;
			}
		}
		_ascr->startInteractiveScale(
			mWin, val[0], val[1], alpha, tmp);
	}
	else if (StrEquals(op,"DragRotateXY"))
	{
		drag_rotate = DRAG_ROTATE_XY;
	}
	else if (StrEquals(op,"DragRotateX")) {
		drag_rotate = DRAG_ROTATE_X;
	}
	else if (StrEquals(op,"DragRotateY")) {
		drag_rotate = DRAG_ROTATE_Y;
	}
	else if (StrEquals(op,"DragRotateZ")) {
		drag_rotate = DRAG_ROTATE_Z;
	}

	if (drag_rotate != DRAG_ROTATE_NO)
	{
		bool tmp = false;

		if (rest)
		{
			char *a;
			a = PeekToken(rest, &rest);
			if (StrEquals(a,"Tmp")) {
				tmp = true;
			}
		}
		_ascr->startDragRotate(mWin, drag_rotate, tmp);
	}

	if (op)
	{
		free(op);
	}
}

void FvwmModule::cutWindow(MetisseWindow *mWin, WinRegions::region *reg)
{
	std::list<WinRegions::region > r = mWin->getSelectedRegions();
	std::list<WinRegions::region >::iterator i;
	bool found = false;

	if (r.size() == 0)
	{
		return;
	}

	float width,height;
	mWin->getSize(&width,&height);

	for (i=r.begin(); i!= r.end(); ++i)
	{
		if ((int)(reg->x + width/2) != (int)(*i).x ||
		    (int)(-reg->y + height/2) != (int)(*i).y ||
		    (int)reg->w != (int)(*i).w || (int)reg->h != (int)(*i).h)
		{
			continue;
		}
		found = true;
		break;
	}
	if (found)
	{
		mWin->getRenderer()->addtoCutShape(
				(*i).x, (*i).y, (*i).w, (*i).h);
		mWin->getRenderer()->rmMatchingSelectedRegions(reg);
	}
}

void FvwmModule::cutWindow(MetisseWindow *mWin)
{
	std::list<WinRegions::region > r = mWin->getSelectedRegions();

	if (r.size() == 0)
	{
		return;
	}

	mWin->getRenderer()->setCutShape(r);
	mWin->getRenderer()->rmSelectedRegions();
}

void FvwmModule::buildFacade(MetisseWindow *mWin, bool cutToo)
{
	char cmd[1024];
	char tmp[56];

	std::list<WinRegions::region > r = mWin->getSelectedRegions();
	std::list<WinRegions::region >::iterator i;

	//std::cerr << "build win cut " <<  r.size() << std::endl;

	if (r.size() == 0)
	{
		return;
	}

	sprintf(cmd, "Exec exec facade-holder -i 0x%lx  -c",
		mWin->getFrameID());

	for (i=r.begin(); i!= r.end(); ++i)
	{
		sprintf(tmp, " -g %ix%i+%i+%i",
			(int)(*i).w, (int)(*i).h, (int)(*i).x, (int)(*i).y);
		strcat(cmd, tmp);
	}

	sendText(cmd,0);
	if (cutToo)
	{
		mWin->getRenderer()->setCutShape(r);
	}
	mWin->getRenderer()->rmSelectedRegions();
}

void FvwmModule::buildFacade(
	MetisseWindow *mWin, WinRegions::region *reg, int x, int y, bool cutToo)
{
	char cmd[1024];
	char tmp[56];

	std::list<WinRegions::region > r = mWin->getSelectedRegions();
	std::list<WinRegions::region >::iterator i;
	bool found = false;

	//std::cerr << "build win cut " <<  r.size() << std::endl;

	if (r.size() == 0)
	{
		return;
	}

	float width,height;
	mWin->getSize(&width,&height);

	sprintf(cmd, "Exec exec facade-holder -i 0x%lx  -p +%i+%i -c",
		mWin->getFrameID(), (int)(x - reg->w/2), (int)(y - reg->h/2));

	for (i=r.begin(); i!= r.end(); ++i)
	{
		if ((int)(reg->x + width/2) != (int)(*i).x ||
		    (int)(-reg->y + height/2) != (int)(*i).y ||
		    (int)reg->w != (int)(*i).w || (int)reg->h != (int)(*i).h)
		{
			continue;
		}
		found = true;
		sprintf(tmp, " -g %ix%i+%i+%i",
			(int)(*i).w, (int)(*i).h, (int)(*i).x, (int)(*i).y);
		strcat(cmd, tmp);
		break;
	}

	if (found)
	{
		if (cutToo)
		{
			mWin->getRenderer()->addtoCutShape(
				(*i).x, (*i).y, (*i).w, (*i).h);	
		}
		sendText(cmd,0);
		mWin->getRenderer()->rmMatchingSelectedRegions(reg);
	}
}

void FvwmModule::_buildFacade(bool cutToo)
{
	char cmd[1024];
	char tmp[56];

	WindowMapping wins = _metisseDesktop->getWindowMapping();
	WindowMapping::iterator iter;
	bool foundAFacade = false;

	sprintf(cmd, "Exec exec facade-holder -c");

	for (iter=wins.begin(); iter!= wins.end(); ++iter)
	{
		MetisseWindow *w = (*iter).second;

		std::list<WinRegions::region > r = w->getSelectedRegions();
		std::list<WinRegions::region >::iterator i;
		bool first = true;
		for (i=r.begin(); i!= r.end(); ++i)
		{
			if (first)
			{
				sprintf(tmp, " -i 0x%lx", w->getFrameID());
				first = false;
				strcat(cmd, tmp);
			}
			sprintf(tmp, " -g %ix%i+%i+%i",
				(int)(*i).w, (int)(*i).h, (int)(*i).x,
				(int)(*i).y);
			strcat(cmd, tmp);
			foundAFacade = true;
		}
		if (r.size()>0 && cutToo)
		{
			w->getRenderer()->addtoCutShape(r);	
		}
		w->getRenderer()->rmSelectedRegions();
	}

	if (foundAFacade)
	{
		sendText(cmd,0);
	}
}

void FvwmModule::_parseMessageLine(Window id, char *line)
{
	char *opt;
	char *rest;

	MetisseWindow *mWin = _metisseDesktop->findWindow(id);

	WindowRenderer *winRenderer = 0;
	
	if (mWin)
	{
		winRenderer = mWin->getRenderer();
	}

	opt = PeekToken(line, &rest);
	if (StrEquals(opt,"Draw"))
	{
		float pct;

		_ascr->draw(false);
		opt = PeekToken(rest, &rest);	
		if (opt && sscanf(opt,"%f",&pct))
		{
			usleep(1000*(int)pct);
		}
	}
	else if (StrEquals(opt,"Scale"))
	{
		if (winRenderer)
		{
			float pct0,pct1;
			if (rest && sscanf(rest,"%f %f",&pct0, &pct1) == 2)
			{
				_ascr->root->scaleWindow(mWin, pct0, pct1, 1);
			}
			else if (rest && sscanf(rest,"%f",&pct0))
			{
				_ascr->root->scaleWindow(mWin,pct0,pct0,pct0);
			}
			
		}
	}
	else if (StrEquals(opt,"AbsoluteScale"))
	{
		if (winRenderer)
		{
			float pct;
			unsigned int gravity = 0;
			opt = PeekToken(rest, &rest);
			
			if (opt && sscanf(opt,"%f",&pct))
			{
				_ascr->root->scaleWindow(
					mWin, pct,pct,pct, gravity, true);
			}
			else if (opt && StrEquals(opt,"C"))
			{
				opt = PeekToken(rest, &rest);
				gravity = CenterGravity;
				if (opt && sscanf(opt,"%f",&pct))
				{
					_ascr->root->scaleWindow(
						mWin, pct,pct,pct, gravity, true);
				}
			}
		}
	}
	else if (StrEquals(opt,"Rotate"))
	{
		int val[5];
		int num_val;

		val[4] = 0;
		num_val = GetIntegerArguments(rest, NULL, val, 5);

		if (num_val >= 4)
		{
			if (winRenderer)
			{
				if (num_val == 4 || val[4] == 0)
				{
					_ascr->root->rotateWindow(
						mWin,
						val[0],val[1],val[2],val[3]);
				}
				else if (val[4] > 0)
				{
					winRenderer->rotate_rel(
						val[0],val[1],val[2],val[3]);
					sendMetisseTransform(mWin, false);
				}
			}
		}
	}
	else if (StrEquals(opt,"FoldWindow"))
	{
		float alpha;
		float reduction;
		float ox,oy;
		float px,py;
		if (winRenderer && sscanf(rest,"%f %f %f %f %f %f",
					  &ox, &oy, &px, &py,&reduction, &alpha) == 6)
		{
			_ascr->foldByCmd(mWin,(int)ox,(int)oy,alpha,reduction);
			_ascr->foldByCmd(mWin,(int)px,(int)py,alpha,reduction);
		}
		else if (winRenderer && sscanf(rest,"%f %f %f %f",
					  &px, &py, &reduction, &alpha) == 4)
		{
			_ascr->foldByCmd(mWin,(int)px,(int)py,alpha,reduction);
		}
	}
	else if (StrEquals(opt,"UnFoldWindow"))
	{
		if (winRenderer)
			winRenderer->fold(0, 0, false);
	}
	else if (StrEquals(opt,"FoldForWindowAnim"))
	{
		float alpha;
		float offset;

		if (winRenderer && sscanf(rest,"%f %f",&alpha,&offset) == 2)
		{
			_ascr->root->foldOverWindows(mWin, true, alpha, true);
		}
	}
	else if (StrEquals(opt,"FoldForWindow"))
	{
		float alpha;
		float offset;

		if (winRenderer && sscanf(rest,"%f %f",&alpha,&offset) == 2)
		{
			_ascr->root->foldOverWindows(mWin, true, alpha, false);
		}
	}
	else if (StrEquals(opt,"UnFoldForWindow"))
	{
		if (mWin)
			_ascr->root->foldOverWindows(
				mWin, false);	
	}
	else if (StrEquals(opt,"AbsoluteRotate"))
	{
		int val[4];
		if (GetIntegerArguments(rest, NULL, val, 4) == 4)
		{
			if (mWin)
			{
				_ascr->root->rotateWindow(
					mWin, val[0],val[1],val[2],val[3],
					0, true);
			}
		}
	}
	else if (StrEquals(opt,"Reset"))
	{
		int toggle = ParseToggleArgument(rest, &rest, -1, 0);
		if (winRenderer)
		{
			// FIXME: start-stop please
			if (toggle == -1)
			{
				winRenderer->resetOrSet(true);
			}
			else
			{
				winRenderer->reset(false, true, true);
			}
		}
	}
	else if (StrEquals(opt,"Operation"))
	{
		/* fvwm send us a window */
		if (mWin)
		{
			_processOperation(mWin, rest);
		}
	}
	else if (StrEquals(opt,"FixCursor"))
	{
		/* fvwm send us a window */
		opt = PeekToken(rest, &rest);
		if (StrEquals(opt,"Start"))
		{
			_ascr->fixCursorPos(true, mWin);
		}
		else
		{
			_ascr->fixCursorPos(false);
		}
	}
	else if (StrEquals(opt,"CutWindow"))
	{
		/* fvwm send us a window */
		if (mWin)
		{
			cutWindow(mWin);
		}
	}
	else if (StrEquals(opt,"UnCutWindow"))
	{
		/* fvwm send us a window */
		if (mWin)
		{
			std::list<WinRegions::region > newReg;
			newReg.clear();
			winRenderer->setCutShape(newReg);
		}
	}
	else if (StrEquals(opt,"CreateFacade"))
	{
		/* fvwm send us a window */
		if (mWin)
		{
			buildFacade(mWin);
		}
	}
	else if (StrEquals(opt,"CreateAllFacade"))
	{
		_buildFacade();
	}
	else if (StrEquals(opt,"rmFacade")) // FIXME: bas name
	{	
		if (mWin && !mWin->isAFacadeWindow())
		{
			FacadeMapping wcm;
			wcm.clear();
			bool changed = mWin->setFacade(wcm, false);
			if (changed)
			{
				_metisseDesktop->windowUpdateRequest(mWin);
			}
		}
	}
	else if (StrEquals(opt,"RemoveFacadeElement"))
	{	
		if (mWin)
		{
			_ascr->actionOnFacade(mWin, "Remove");
		}
	}
	else if (StrEquals(opt,"EchoWidgets"))
	{
		if (mWin)
		{
			_ascr->printWidgets(mWin);
		}
	}
	else if (StrEquals(opt,"EchoWidgetsExtent"))
	{
		int val[2];
		if (mWin)
		{
			if (GetIntegerArguments(rest, NULL, val, 2) == 2)
			{
				WinRegions::region r = _ascr->getWidgetExtent(
					mWin, val[0], val[1]);
				std::cerr << val[0] << " " << val[1] << " "
					  << r.x << " " << r.y << " "
					  << r.w << " " << r.h << std::endl;
			}
			else
			{
				std::cerr << "EchoWidgetExtent need exactly 2"
					  << " int arg"
					  << std::endl;
			}
		}
	}
	else if (StrEquals(opt,"ShowWidgets"))
	{
		if (mWin && winRenderer)
		{
			if (winRenderer->getRegionsSetSize() > 0)
			{
				winRenderer->clearRegionsSet();
			}
			else
			{
				std::list<WinRegions::xyregion > reg;
				reg = _ascr->getWidgetRegionsXY(mWin);
				winRenderer->setRegionsSet(reg);
			}
		}
	}
	else if (StrEquals(opt,"ActionOnWidget"))
	{
		if (mWin)
		{
			_ascr->actionOnWidget(mWin);
		}
	}
	else if (StrEquals(opt,"SetValuatorSpeed"))
	{
		float speed;

		if (mWin && rest && sscanf(rest,"%f",&speed) == 1)
		{
			_ascr->setWidgetValuator(mWin, speed);
		}
	}
	else if (StrEquals(opt,"SetAllValuatorSpeed"))
	{
		float speed;

		if (mWin && rest && sscanf(rest,"%f",&speed) == 1)
		{
			_ascr->setAllWidgetValuator(mWin, speed);
		}
	}
	else if (StrEquals(opt,"ComboBoxAction"))
	{
		opt = PeekToken(rest, &rest);
		if (mWin && opt)
		{
			_ascr->actionOnComboBox(mWin, opt);
		}
	}
	else if (StrEquals(opt,"SaveFacadeDialogue"))
	{
		if (mWin)
		  {
			char cmd[256];
			
			sprintf(cmd, "AFuncStartMetisseFvwmScript "
				"FvwmScript-SaveFacade %lu",
				(unsigned long)id);
			sendText(cmd, mWin);
		}
	}
	else if (StrEquals(opt,"DoSaveFacade"))
	{
		unsigned long nid;
		int use_win_name, automatic, hide_source;
		opt = PeekToken(rest, &rest);
		if (opt && *opt != '\0' && rest &&
		    sscanf(rest,"%d %d %d %d", &nid, &use_win_name,
			   &automatic, &hide_source) == 4)
		{
			MetisseWindow *win = _metisseDesktop->findWindow(nid);
			if (win != 0)
			{
				persistence_save_facade(
					win, _metisseDesktop, opt,
					(use_win_name)? true:false,
					(automatic)? true:false, 
					(hide_source)? true:false);
			}

			//std::cerr << "Save Facade: " << opt << " " <<  nid
			//	  << "\n";
		}
	}
	else if (StrEquals(opt,"RecallFacade"))
	{
		opt = PeekToken(rest, &rest);
		if (opt && mWin)
		{
			persistence_recall_facade(mWin, opt, _ascr);
		}
	}
	else if (StrEquals(opt,"UnSaveFacade"))
	{
		if (mWin)
		{
			char cmd[256];
			
			persistence_unsave_facade(mWin, _ascr);
		}
	}
	else if (StrEquals(opt,"RegionSelectionSnapDist"))
	{
		int val[1];
		if (GetIntegerArguments(rest, NULL, val, 1) == 1 && val[0] >= 0)
		{
			_ascr->setRegionSelectionSnapDist(val[0]);
		}
		else
		{
			std::cerr << "RegionSelectionSnapDist need exactly 1 "
				  << "positive or nul integer arg"
				  << std::endl;
		}
	}
	else if (StrEquals(opt,"MarkingFeedback"))
	{
		MetisseWindow *lw = mWin;
		WindowRenderer *lwr = winRenderer;
		bool on = False;
		static WindowRenderer *prev_winRenderer = 0;

		opt = PeekToken(rest, &rest);
		if(StrEquals(opt,"On"))
		{
			on = true;
		}
		else
		{
			lwr = prev_winRenderer;
		}
		if (on && lwr == 0)
		{
			lw = _ascr->getSelectedWindow();
			if (lw != 0)
			{
				lwr = lw->getRenderer();
			}
			prev_winRenderer = lwr;
		}
		if (lwr)
		{
			int val[5];

			if (!rest ||
			    GetIntegerArguments(rest, &rest, val, 5) != 5)
			{
				/* should not happen */
				return;
			}
			lwr->setMarkingFeedback(
				on, val[0], val[1], val[2], val[3], val[4]);
		}
		else
		{
			// FIXME should draw on root window
		}
		if (on)
		{
			prev_winRenderer = lwr;
		}
	}
	else if (StrEquals(opt,"StartStroke"))
	{
		_ascr->stroke(true);
	}
	else if (StrEquals(opt,"StopStroke"))
	{
		int val[1];
		bool replay = false;
		if (GetIntegerArguments(rest, NULL, val, 1) == 1 && val[0] == 1)
		{
			replay = true;
		}
		_ascr->stroke(false,replay);
	}
	else if (StrEquals(opt,"StrokeFeed"))
	{
		//TODO
	}
	else if (StrEquals(opt,"ShowVMouse"))
	{
		_ascr->showVMouse();
	}
	else if (StrEquals(opt,"IconifyAnimate"))
	{
		if (mWin == 0 || GetIntegerArguments(
			    rest, &rest, _iconifyGeometries, 8) != 8)
		{
			return;
		}
		_iconifyGeometriesSet = true;
		//std::cerr << "IconifyAnimate Win: " << mWin << std::endl;
		
	}
	else if (StrEquals(opt,"AllowsSurfaceRotation"))
	{
		if (mWin)
		{
			mWin->surfaceRotation(false);
		}
	}
	else if (StrEquals(opt,"NoSurfaceRotation"))
	{
		if (mWin)
		{
			mWin->surfaceRotation(true);
		}
	}
	else if (StrEquals(opt,"RestoreUnscaledPosition"))
	{
		if (mWin)
		{
			float x,y;
			char cmd[256];

			if (mWin->getSurfaceScaleSavedRealPosition(&x, &y))
			{
				sprintf(cmd, "AnimatedMove %ip %ip",
					(int)(x), (int)(y));
				sendText(cmd, mWin);
			}
		}
	}
	else if (StrEquals(opt,"Surface") || StrEquals(opt,"DefineSurface"))
	{
		int surface = -1;
		int toggle;
		float ft[4];
		int nbr = 0;

		if (StrEquals(opt,"Surface"))
		{
			toggle = ParseToggleArgument(rest, &rest, -1, 0);
			_ascr->root->enableSurface(toggle);
		}

		opt = PeekToken(rest, &rest);

		if (StrEquals(opt,"Sphere"))
		{
			surface = SURFACE_SPHERE;
		}
		else if (StrEquals(opt,"Cylinder") ||
			 StrEquals(opt,"LRCylinder"))
		{
			surface = SURFACE_LRCYLINDER;	
		}
		else if (StrEquals(opt,"TBCylinder"))
		{
			surface = SURFACE_TBCYLINDER;	
		}
		else if (StrEquals(opt,"SphereCylinder"))
		{
			surface = SURFACE_SPHERE_CYLINDER;	
		}
		else if (StrEquals(opt,"CylinderSphere"))
		{
			surface = SURFACE_CYLINDER_SPHERE;	
		}
		else if (StrEquals(opt,"TableTop"))
		{
			surface = SURFACE_TABLETOP;
			_ascr->root->setSurface(surface, 0, 0);
			return;
		}
		else if (StrEquals(opt, "Scale"))
		{
			surface = SURFACE_SCALE;
			_ascr->root->setSurface(surface, 0, 0);
			return;
		}
		else if (StrEquals(opt,"Current"))
		{
			surface = -2;	
		}

		if (surface != -1)
		{
			opt = PeekToken(rest, &rest);
		}

		while(opt && *opt != '\0')
		{
			float f;

			if (nbr < 4 && sscanf(opt,"%f",&f))
			{
				ft[nbr++] = f;
			}
			opt = PeekToken(rest, &rest);
		}

		if (surface == -2)
		{
			if (nbr >= 2)
			{
				_ascr->root->setSurface(ft[0], ft[1]);
			}
			else if (nbr >= 1)
			{
				_ascr->root->setSurface(ft[0]);
			}
		}
		else if (surface != -1)
		{
			if (nbr >= 2)
			{
				_ascr->root->setSurface(surface, ft[0], ft[1]);
			}
			else if (nbr >= 1)
			{
				_ascr->root->setSurface(surface, ft[0]);
			}
			else
			{
				_ascr->root->setSurface(surface);
			}
		}
		else
		{
			if (nbr >= 4)
			{
				_ascr->root->setSurfaceParameters(
					ft[0], ft[1], ft[2], ft[3]);
			}
			else if (nbr >= 2)
			{
				_ascr->root->setSurfaceParameters(ft[0], ft[1]);
			}
			else if (nbr >= 1)
			{
				_ascr->root->setSurfaceParameters(ft[0]);
			}
		}
	}
	else if (StrEquals(opt,"CropToCircule"))
	{
		if (winRenderer)
		{
			winRenderer->cropToCircule() ;
		}
	}
	else if (StrEquals(opt,"TestPolygon"))
	{
		if (winRenderer)
		{
			winRenderer->testPolygon() ;
		}
	}
	else if (StrEquals(opt,"Animate"))
	{
		if (winRenderer)
		{
			winRenderer->animate() ;
		}
	}
	else if (StrEquals(opt,"Shadow"))
	{
		if (winRenderer)
		{
			winRenderer->shadow() ;
		}
	}
	else if (StrEquals(opt,"SaveFrame"))
	{
		int delay;

		if (GetIntegerArguments(rest, NULL, &delay, 1))
		{
			_ascr->root->saveFrame(delay);	
		}
		else
		{
			_ascr->saveFrame();
		}
	}
	else if (StrEquals(opt,"StartVideoCapture"))
	{
	  char *sink = PeekToken(rest, &rest);
		_ascr->startVideoCapture(sink);
	}
	else if (StrEquals(opt,"StopVideoCapture"))
	{
		_ascr->stopVideoCapture();
	}
	else if (StrEquals(opt,"ShowWindow"))
	{
		if (mWin)
		  {
			_ascr->root->translateForWindow(mWin);
			_ascr->draw(false);
		}
	}
	else if (StrEquals(opt,"MoreTransparency"))
	{
		if (mWin)
		{
			float d = 0.05;
			float a,pct;
			a = mWin->getAlpha();
			opt = PeekToken(rest, &rest);
			if (opt && sscanf(opt,"%f",&pct))
			{
				if (pct > 1.0) pct = 1.0;
				else if (pct < 0) pct = 0;
				d = pct;
			}
			a -= d;
			if (a < 0.05) a = 0.05;
			mWin->setAlpha(a);
		}
	}
	else if (StrEquals(opt,"LessTransparency"))
	{
		if (mWin)
		{
			float d = 0.05;
			float a,pct;
			a = mWin->getAlpha();
			opt = PeekToken(rest, &rest);
			if (opt && sscanf(opt,"%f",&pct))
			{
				if (pct > 1.0) pct = 1.0;
				else if (pct < 0) pct = 0;
				d = pct;
			}
			a += d;
			if (a > 1.0) a = 1;
			mWin->setAlpha(a);
		}
	}
	else if (StrEquals(opt,"SetOrRemoveTransparency"))
	{
		if (mWin)
		{
			float pct;
			float a = mWin->getAlpha();
			opt = PeekToken(rest, &rest);
			if (opt && a == 1.0 && sscanf(opt,"%f",&pct))
			{
				if (pct > 1.0) pct = 1.0;
				else if (pct < 0) pct = 0.05;
				mWin->setAlpha(pct);
			}
			else
			{
				mWin->setAlpha(1.0);
			}
		}
	}
	else if (StrEquals(opt,"SetTransparency"))
	{
		if (mWin)
		{
			float pct;
			opt = PeekToken(rest, &rest);
			if (opt && sscanf(opt,"%f",&pct))
			{
				if (pct > 1.0) pct = 1.0;
				else if (pct < 0) pct = 0;
				mWin->setAlpha(pct);
			}
		}
	}
	else if (StrEquals(opt,"ResizeTransparency"))
	{
		float pct;
		opt = PeekToken(rest, &rest);
		if (opt && sscanf(opt,"%f",&pct))
		{
			if (pct > 1.0) pct = 1.0;
			else if (pct < 0) pct = 0;
			_ascr->setResizeTransparency(pct);
		}
	}
	else if (StrEquals(opt,"TmpRestackMode"))
	{
		opt = PeekToken(rest, &rest);
		if (StrEquals(opt,"Fold"))
		{
			_ascr->setTmpRestackMode(TMP_RESTACK_MODE_FOLD);
		}
		else if (StrEquals(opt,"Scale"))
		{
			_ascr->setTmpRestackMode(TMP_RESTACK_MODE_SCALE);
		}
		else if (StrEquals(opt,"Rotate"))
		{
			_ascr->setTmpRestackMode(TMP_RESTACK_MODE_ROTATE);
		}
		else
		{
			std::cerr << "Invalide option to TmpRestackMode\n";
		}
	}
	else if (StrEquals(opt,"SnapAttraction"))
	{
		int val[1];
		val[0] = -1;
		if (GetIntegerArguments(rest, NULL, val, 1) == 1 && val[0] >= 0)
		{
			_ascr->root->setSnapAttractDist(val[0]);
			opt = PeekToken(rest, &rest);
			opt = PeekToken(rest, &rest);
			if (StrEquals(opt,"Classical"))
			{
				_ascr->root->setSnapAttractMode(
					SNAP_ATTRAC_MODE_CLASSICAL);
			}
			else if (StrEquals(opt,"SnapAndGo"))
			{
				_ascr->root->setSnapAttractMode(
					SNAP_ATTRAC_MODE_SG);
			}
			else if (StrEquals(opt,"VisSnapAndGo"))
			{
				_ascr->root->setSnapAttractMode(
					SNAP_ATTRAC_MODE_VISSG);
			}
			else if (StrEquals(opt,"Metisse") ||
				 StrEquals(opt,"Rubber"))
			{
				_ascr->root->setSnapAttractMode(
					SNAP_ATTRAC_MODE_METISSE);
			}
			else if (StrEquals(opt,"None"))
			{
				_ascr->root->setSnapAttractMode(
					SNAP_ATTRAC_MODE_NONE);
			}
			else
			{
				std::cerr << "Unknown SnapAttraction Mode\n";
				_ascr->root->setSnapAttractMode(
					SNAP_ATTRAC_MODE_NONE);
				
			}
				  
		}
		else if (!rest || *rest == '\0')
		{
			_ascr->root->setSnapAttractDist();
			_ascr->root->setSnapAttractMode();
		}
		else
		{
			std::cerr << "SnapAttraction syntaxe error\n";
		}
	}
	else if (StrEquals(opt,"SnapAttractionDist"))
	{
		int val[1];
		val[0] = -1;
		if (GetIntegerArguments(rest, NULL, val, 1) == 1 && val[0] >= 0)
		{
			_ascr->root->setSnapAttractDist(val[0]);
		}
		else if (!rest || *rest == '\0')
		{
			_ascr->root->setSnapAttractDist();
		}
		else
		{
			std::cerr << "SnapAttractionDist syntaxe error\n";
		}
	}
	else if (StrEquals(opt,"SnapAttractionMode"))
	{
		opt = PeekToken(rest, &rest);
		if (StrEquals(opt,"Classical"))
		{
			_ascr->root->setSnapAttractMode(
				SNAP_ATTRAC_MODE_CLASSICAL);
		}
		else if (StrEquals(opt,"SnapAndGo"))
		{
			_ascr->root->setSnapAttractMode(
				SNAP_ATTRAC_MODE_SG);
		}
		else if (StrEquals(opt,"VisSnapAndGo"))
		{
			_ascr->root->setSnapAttractMode(
				SNAP_ATTRAC_MODE_VISSG);
		}
		else if (StrEquals(opt,"Metisse") ||
			 StrEquals(opt,"Rubber"))
		{
			_ascr->root->setSnapAttractMode(
				SNAP_ATTRAC_MODE_METISSE);
		}
		else if (StrEquals(opt,"Metisse"))
		{
			_ascr->root->setSnapAttractMode(
				SNAP_ATTRAC_MODE_METISSE);
		}
		else if (StrEquals(opt,"None"))
		{
			_ascr->root->setSnapAttractMode(
				SNAP_ATTRAC_MODE_NONE);
		}
		else if (!opt || opt == '\0')
		{
			_ascr->root->setSnapAttractMode();
		}
		else
		{
			std::cerr << "Unknown SnapAttraction Mode\n";
			_ascr->root->setSnapAttractMode(
				SNAP_ATTRAC_MODE_NONE);	
		}
	}
	else if (StrEquals(opt,"BorderLens"))
	{
		float pct;
		opt = PeekToken(rest, &rest);
		if (opt && sscanf(opt,"%f",&pct))
		{
			WindowRenderer::borderVisLens = pct;
		}
		else
		{
			WindowRenderer::borderVisLens = 1.0;
		}
	}
	else if (StrEquals(opt,"LensRegions"))
	{
		float pct1,pct2;
		opt = PeekToken(rest, &rest);
		if (winRenderer && opt && *opt != '\0')
		{
			if (sscanf(opt,"%f %f",&pct1, &pct2) == 2)
			{
				winRenderer->lensRegions(pct1,pct2);
			}
			else if (sscanf(opt,"%f",&pct1) == 1)
			{
				winRenderer->lensRegions(pct1,pct1);
			}
			else
			{
				winRenderer->unlens();
			}
		}
		else if (winRenderer)
		{
			winRenderer->unlens();
		}
	}
	else if (StrEquals(opt,"LensType"))
	{
		if (winRenderer)
		{
			opt = PeekToken(rest, &rest);
			if (StrEquals(opt,"None") || !opt || *opt == '\0')
			{
				winRenderer->setAutoLensType(LENS_TYPE_NONE);
			}
			else if (StrEquals(opt,"Vert"))
			{
				winRenderer->setAutoLensType(LENS_TYPE_VERT);
			}
			else if (StrEquals(opt,"Horiz"))
			{
				winRenderer->setAutoLensType(LENS_TYPE_HORIZ);
			}
			else if (StrEquals(opt,"CROSS"))
			{
				winRenderer->setAutoLensType(LENS_TYPE_CROSS);
			}
			else if (StrEquals(opt,"CENTER"))
			{
				winRenderer->setAutoLensType(LENS_TYPE_CENTER);
			}
			else
			{
				std::cerr << "Unknown LensType\n";
			}
		}
	}
	else if (StrEquals(opt,"RootTranslate"))
	{
		int val[3];
		if (GetIntegerArguments(rest, NULL, val, 3) == 3)
		{
			_ascr->root->zoomedout_translate_rel(
				val[0],val[1],val[2]);
			_ascr->resetSelection();
		}
	}
	else if (StrEquals(opt,"RootRotate")) {
		int val[4];
		if (GetIntegerArguments(rest, NULL, val, 4) == 4)
		{
			_ascr->root->rotate(val[0],val[1],val[2],val[3]);
		}
	}
	else if (StrEquals(opt,"RootScale"))
	{
		float pct;
		opt = PeekToken(rest, &rest);
		if (opt && sscanf(opt,"%f",&pct))
		{
			_ascr->root->scale(pct,pct,1);
			_ascr->resetSelection();
		}
	}
	else if (StrEquals(opt,"RootReset"))
	{
		_ascr->root->resetTransformations();
		_ascr->resetSelection();
	}
	else if (StrEquals(opt,"ExposeMode"))
	{
		_ascr->root->exposeMode();
	}
	else if (StrEquals(opt,"PagerMode"))
	{
		int toggle = ParseToggleArgument(rest, NULL, -1, 0);
		if (toggle == -1) {
			_ascr->root->togglePagerMode();
		}
		else
		{
			_ascr->root->pagerMode((bool)toggle);
		}
	}
	else if (StrEquals(opt,"ExtendedInput"))
	{
		// Pointer_Name On|Off|ToolGlass
		opt = PeekToken(rest,&rest);
		_ascr->configureExtInput(mWin, opt, rest);
		
	}
	else if (StrEquals(opt,"SendTransformations"))
	{
		int toggle = ParseToggleArgument(rest, &rest, -1, 0);
		if (toggle == -1)
		{
			_sendTransformations = !_sendTransformations;
		}
		else
		{
			_sendTransformations = (bool)toggle;
		}
	}
	else if (StrEquals(opt,"SetFullScreenMode")) {
	  _ascr->compositor->setFullScreen(true) ;
	}
	else if (StrEquals(opt,"SetWindowMode"))
	{
	  unsigned int width, height ;
	  _ascr->compositor->getGeometry(&width, &height) ;
	  _ascr->compositor->setFullScreen(false) ;
	  _ascr->compositor->setGeometry(width, height,0,0) ;
	}
	else if (StrEquals(opt,"DefineShader")) {
	  if (rest) {
	    _ascr->makeCurrent() ;
	    std::string therest = rest ;
	    std::string shadername = extractNextWord(therest) ;
	    _ascr->getWinShaders()->defineShader(shadername,therest, false);
	  }
	}
	else if (StrEquals(opt,"DestroyShader")) {
	  if (rest) {
	    _ascr->makeCurrent() ;
	    _ascr->getWinShaders()->destroyShader(rest) ;
	  }
	}
	else if (StrEquals(opt,"AddToShader")) {
	  if (rest) {
	    _ascr->makeCurrent() ;
	    std::string therest = rest ;
	    std::string shadername = extractNextWord(therest) ;
	    _ascr->getWinShaders()->defineShader(shadername, therest, true);
	  }
	}
	else if (StrEquals(opt,"PushShaderWindow"))
	{
		if (rest && winRenderer)
		{
			_ascr->getWinShaders()->addShaderWindow(
				winRenderer, rest);
		}
		else
		{
			std::cerr << "Warnning: PushShaderWindow need at least a"
				  << "shader name as an argument"
				  << std::endl ;
		}
	}
	else if (StrEquals(opt,"PushBackShaderWindow"))
	{
		if (rest && winRenderer)
		{
			_ascr->getWinShaders()->addShaderWindow(
				winRenderer, rest, false /* clear */,
				true /* back */);
		}
		else
		{
			std::cerr << "Warnning: PushBackShaderWindow need at "
				  << "least a"
				  << "shader name as an argument"
				  << std::endl ;
		}
	}
	else if (StrEquals(opt,"RemoveShaderWindow"))
	{
		if (rest && winRenderer)
		{
			_ascr->getWinShaders()->removeShaderWindow(
				winRenderer, rest);
		}
		else
		{
			std::cerr << "Warnning: RemoveShaderWindow need at "
				  << "least a"
				  << "shader name as an argument"
				  << std::endl ;
		}
	}
	else if (StrEquals(opt,"ClearShadersWindow"))
	{
		if (winRenderer)
		{
			winRenderer->removeAllShaders() ;
		}
	}
	else if (StrEquals(opt,"SetShaderWindow"))
	{
		if (rest && winRenderer)
		{
			_ascr->getWinShaders()->addShaderWindow(
				winRenderer, rest, true /* clear */);
		}
		else
		{
			std::cerr << "Warnning: SetShaderWindow need at least a"
				  << "shader name as an argument"
				  << std::endl ;
		}
	}
	else if (StrEquals(opt,"AnimBackShaderWindow"))
	{
		if (winRenderer)
		{
			_ascr->getWinShaders()->animShaderWindow(
				winRenderer, rest, true);
		}
	}
	else if (StrEquals(opt,"AnimShaderWindow"))
	{
		if (winRenderer)
		{
			_ascr->getWinShaders()->animShaderWindow(
				winRenderer, rest, false);
		}
	}
	else if (StrEquals(opt,"FvwmTrace"))
	{
		opt = PeekToken(rest,&rest);
		if (opt)
		{ 
			if (StrEquals(opt,"Started"))
			{
				std::cerr << "FvwmTrace Started\n";
				_ascr->fvwmTraceRunning(true);
			}
			else
			{
				std::cerr << "FvwmTrace Stoped\n";
				_ascr->fvwmTraceRunning(false);
			}
		}
	}
	else if (StrEquals(opt,"PasteClipboard"))
	{
		_ascr->clipPasteClipboard();
	}
	else if (StrEquals(opt,"DragAndDropStart"))
	{
		_ascr->dragAndDropStart();
	}
	else if (StrEquals(opt,"DragAndDropStop"))
	{
		//std::cerr << "dragAndDropStart By Module" <<  std::endl ;
		_ascr->dragAndDropStop();
	}
	else if (StrEquals(opt,"ScrollLeafing"))
	{
		//std::cerr << "dragAndDropStart By Module" <<  std::endl ;
		_ascr->popupScrollLeafingWidget(0);
	}
	else if (StrEquals(opt,"RightLongClickTimeOut"))
	{
		int val[1];
		val[0] = -1;
		if (GetIntegerArguments(rest, NULL, val, 1) == 1 && val[0] >= 0)
		{
			_ascr->setRightLongPressTimeOut(val[0]);
		}
	}
	else if (StrEquals(opt,"PseudoSwallow"))
	{
		if (mWin)
		{
			WindowMapping wins =
				_metisseDesktop->getWindowMapping();
			WindowMapping::iterator iter;
			MetisseWindow *ewin = 0;

			for (iter=wins.begin(); iter!= wins.end(); ++iter)
			{
				MetisseWindow *w = (*iter).second;
				if (w->isEwmhDesktop())
				{
					ewin = w;
					break;
				}
			}
			if (ewin && mWin->getPseudoSwallowedBy() == 0)
			{
				ewin->pseudoSwallow(mWin);
				mWin->setPseudoSwallowedBy(ewin);
				ewin->getRenderer()->simpleReset();
			}
			else if (ewin)
			{
				ewin->pseudoSwallow(0);
				ewin->getRenderer()->unlens();
				ewin->getRenderer()->simpleReset();
				mWin->setPseudoSwallowedBy(0);
			}
		}
		
	}
	else if (StrEquals(opt,"Restart"))
	{
		sendText((char *)_restartString.c_str(), 0);
	}
	else if (StrEquals(opt,"Quit") || StrEquals(opt,"Exit"))
	{
		// ask fvwm to quit
		SendText(_fd, "Quit", 0);
	}
	else
	{
		std::cerr << "[" <<_myName << "] WARNING -- " <<
			"Unknown command " << line << std::endl;
	}
	_ascr->draw(false);
}

void
FvwmModule::_processMessage(FvwmPacket* packet)
{
	unsigned long type = packet->type;
	// unsigned long length = packet->size;
	unsigned long* body = packet->body;
	MetisseWindow *win = 0;
	static int moveX, moveY;

	switch (type)
	{
	case M_STRING:
		_parseMessageLine((Window)body[1], (char *)&body[3]);
		break;
	case M_NEW_DESK:
		if (_currentDesk == (long)body[0])
		{
			SendUnlockNotification(_fd);
		}
		else
		{
			_currentDesk = (long)body[0];

			// FIXME: does not work well
			#if 0
			int step;
			int d = 20;
			for (step = 1; step <= 180; step += d)
			{
				_ascr->root->rotate(d,0,d,0);
				_ascr->draw(false);
				usleep(1000);
			}
			SendUnlockNotification(_fd);
			for (step = 1; step <= 180; step += d)
			{
				_ascr->root->rotate(d,0,d,0);
				_ascr->draw(false);
				usleep(1000);
			}
			#endif
			SendUnlockNotification(_fd);
			_ascr->resetSelection();
		}
		break;
	case M_NEW_PAGE:
		_currentDesk = (long)body[2];
		_ascr->root->setVirtualDesktopPara(
			body[0], body[1], //  position of the root window in px
			(long)body[2], // desk
			body[5], body[6] // number of columns and rows of pages
			);
		SendUnlockNotification(_fd);
		break;
	case MX_STARTSTOP_OPERATION:
		/* Warning: fvwm wait for an unlock notification */
#if 0
		fprintf(stderr, "MX_STARTSTOP_OPERATION "
			"0x%lx, 0x%lx, %lu, %lu\n",
			(Window)body[1], (Window)body[0], body[2], body[3]);
#endif
		win = _metisseDesktop->findWindow((Window)body[1]);
		if (win)
		{
			bool start = false;
			if (body[3] == MX_STARTSTOP_OPERATION_START)
			{
				start = true;
			}
			if (body[2] == MX_STARTSTOP_OPERATION_MOVE)
			{
				_ascr->wmMove(win, start);
			}
			else if (body[2] == MX_STARTSTOP_OPERATION_RESIZE)
			{
				_ascr->wmResize(win, start);
			}
			else if (body[2] == MX_STARTSTOP_OPERATION_MODULE_MOVE)
			{
				_ascr->wmMove(win, start);
				if (!start)
				{
					int x,y;
					char cmd[256];

					win->getRealPosition(&x, &y);
					sprintf(cmd, "Move w%ip w%ip",
						moveX - x, moveY - y);
					sendText(cmd, win);
				}
				else
				{
					win->getRealPosition(&moveX, &moveY);
				}
			}
			else if (body[2] == MX_STARTSTOP_OPERATION_SELECT &&
				start)
			{
				#if 0
				fprintf(stderr, "MX_STARTSTOP_OPERATION_SELECT"
					" %s\n", (start)? "Start":"Stop");
				#endif
				_ascr->tmpRestack(
					win, start,
					MX_STARTSTOP_OPERATION_SELECT);
			}
			else if (body[2] == MX_STARTSTOP_OPERATION_CLICK_MOTION
				&& start)
			{
				#if 0
				fprintf(stderr,
					"MX_STARTSTOP_OPERATION_CLICK_MOTION"
					" %s\n", (start)? "Start":"Stop");
				#endif
				_ascr->tmpRestack(
					win, start,
					MX_STARTSTOP_OPERATION_CLICK_MOTION);
			}
		}
		/* unlock fvwm */
		SendUnlockNotification(_fd);
		break;
	case MX_MODULE_INTERACTIVE_MOVE:
	{
#if 0
		fprintf(stderr, "MX_MODULE_INTERACTIVE_MOVE "
			"0x%lx, 0x%lx, %i, %i\n",
			(Window)body[1], (Window)body[0],
			(int)body[2], (int)body[3]);
#endif
		int x = (int)body[2];
		int y = (int)body[3];
		win = _metisseDesktop->findWindow((Window)body[1]);
		if (win)
		{
			int oldX, oldY;
			win->getRealPosition(&oldX, &oldY);

			if (x != oldX || y != oldY)
			{
				win->translate_rel(-oldX + x, oldY - y, 0);
				win->setRealPosition(x, y);
			}
		}
		SendUnlockNotification(_fd);
	}
	break;
	case MX_METISSE_SET_TRANSFORM:
	{
		GLfloat f[18];
		int num;
#if 0
		fprintf(stderr, "MX_METISSE_SET_TRANSFORM "
			"0x%lx, 0x%lx, %i, %s\n",
			(Window)body[1], (Window)body[0],
			(int)body[2], (char *)(&body[3]));
#endif
		win = _metisseDesktop->findWindow((Window)body[1]);
		if (!win)
		{
			break;
		}

		num = sscanf(
			(char *)(&body[3]),
			"%f %f %f %f %f %f "
			"%f %f %f %f %f %f "
			"%f %f %f %f %f %f",
			&f[0], &f[1], &f[2], &f[3], &f[4], &f[5],
			&f[6], &f[7], &f[8], &f[9], &f[10], &f[11],
			&f[12], &f[13], &f[14], &f[15], &f[16], &f[17]);
		if (num != 18)
		{
			std::cerr << "[FvwmCompositor] "
				  << "Get an invalid MX_MODULE_INTERACTIVE_MOVE "
				  << "Message" << std::endl;
			break;
		}
		// FIXME: start-stop
		win->getRenderer()->transformBack(f);
	}
	break;
	case M_ADD_WINDOW:
	case M_CONFIGURE_WINDOW:
	{
		win = _metisseDesktop->findWindow((Window)body[1]);
		if (win == 0)
		{
			struct ConfigWinPacket *cfgpacket =
				(ConfigWinPacket *)body;

			//fprintf(stderr, "handleConfigureWindow by module\n");
			_metisseDesktop->handleConfigureWindow(
				(Window)body[1], cfgpacket->frame_x,
				cfgpacket->frame_y, cfgpacket->frame_width,
				cfgpacket->frame_height, false);
			win = _metisseDesktop->findWindow((Window)body[1]);
		}
		if (win)
		{
			window_flags *flags;
			struct ConfigWinPacket *cfgpacket =
				(ConfigWinPacket *)body;

			flags = (window_flags *)malloc(sizeof(window_flags));

			memcpy(flags, &(cfgpacket->flags),
			       sizeof(cfgpacket->flags));
			win->setFvwmParameters(
				flags,
				(int)cfgpacket->title_height,
				(int)cfgpacket->border_width,
				(unsigned long)cfgpacket->w,
				(unsigned long)cfgpacket->frame_width,
				(unsigned long)cfgpacket->frame_height,
				(int)cfgpacket->desk,
				_inStartup);
			#if 0
			if (flags->is_transient)
			{
				fprintf(stderr, "Transient\n");
			}
			#endif
		}
		else
		{
			//fprintf(stderr, "CONF/ADD WINDOW NOT FOUND\n");
		}
		break;
	}
	case M_RES_NAME:
	case M_RES_CLASS:
	case M_WINDOW_NAME:
	{
		win = _metisseDesktop->findWindow((Window)body[1]);
		char *string = (char *) &body[3];
		
		if (win == 0)
		{
			break;
		}
		bool first = (win->getResName() == 0 || win->getResClass() == 0
			      || win->getWindowName() == 0);
		bool win_name_changed = false;
		switch(type)
		{
		case M_RES_NAME:
			win->setResName(string);
			break;
		case M_RES_CLASS:
			win->setResClass(string);
			break;
		case M_WINDOW_NAME:
			if (string &&
			    (win->getWindowName() == 0 ||
			     (win->getWindowName() &&
			      strcmp(string,win->getWindowName()) != 0)))
			{
				win_name_changed = true;
				win->setWindowName(string);
			}
			break;
		default:
			break;
		}
		if (_inStartup || win->getResName() == 0 ||
		    win->getResClass() == 0 || win->getWindowName() == 0)
		{
			break;
		}
		// Should we check on window name change, a priorie ... yes!
		// but we may do something special
		if (first || win_name_changed)
		{
			persistence_window_check(win, false,_ascr);
		}
		break;
	}
	case M_END_WINDOWLIST:
		_inStartup = false;
		persistence_checkall(_ascr, _metisseDesktop);
		break;
	case M_ICONIFY:
	{
		//std::cerr << "M_ICONIFY\n";
		win = _metisseDesktop->findWindow((Window)body[1]);
		if (win != 0 && !(packet->size < 15 || body[5] == 0))
		{
			// WARNING: this should send a SendUnlockNotification
			// at some point
			win->getRenderer()->iconify(
				_ascr->root, this, true,
				(int)body[3],     /* t->icon_x_loc */
				(int)body[4],     /* t->icon_y_loc */
				(int)body[5],     /* t->icon_p_width */
				(int)body[6],     /* t->icon_p_height */
				(int)body[7],     /* t->frame_x */
				(int)body[8],     /* t->frame_y */
				(int)body[9],     /* t->frame_width */
				(int)body[10]     /* t->frame_height */
				);
			#if 0
			fprintf(stderr,
				"Iconify, args %d+%d+%dx%d %d+%d+%dx%d\n",
				(int)body[3],     /* t->icon_x_loc */
				(int)body[4],     /* t->icon_y_loc */
				(int)body[5],     /* t->icon_p_width */
				(int)body[6],     /* t->icon_p_height */
				(int)body[7],     /* t->frame_x */
				(int)body[8],     /* t->frame_y */
				(int)body[9],     /* t->frame_width */
				(int)body[10]     /* t->frame_height */
				);
			#endif
		}
		else if (win != 0 && _iconifyGeometriesSet)
		{
			win->getRenderer()->iconify(_ascr->root, this, true,
				_iconifyGeometries[4],     /* t->icon_x_loc */
				_iconifyGeometries[5],     /* t->icon_y_loc */
				_iconifyGeometries[6],     /* t->icon_p_width */
				_iconifyGeometries[7],     /* t->icon_p_height */
				_iconifyGeometries[0],     /* t->frame_x */
				_iconifyGeometries[1],     /* t->frame_y */
				_iconifyGeometries[2],     /* t->frame_width */
				_iconifyGeometries[3]      /* t->frame_height */
				);
			#if 0
			fprintf(stderr,
				"Iconify, args %d+%d+%dx%d %d+%d+%dx%d\n",
				_iconifyGeometries[4],     /* t->icon_x_loc */
				_iconifyGeometries[5],     /* t->icon_y_loc */
				_iconifyGeometries[6],     /* t->icon_p_width */
				_iconifyGeometries[7],     /* t->icon_p_height */
				_iconifyGeometries[0],     /* t->frame_x */
				_iconifyGeometries[1],     /* t->frame_y */
				_iconifyGeometries[2],     /* t->frame_width */
				_iconifyGeometries[3]      /* t->frame_height */
				);
			#endif
			_iconifyGeometriesSet = false;
		}
		else if (win != 0)
		{
			win->getRenderer()->hide();
			float x,y;
			unsigned int w,h;
			win->getRealPosition(&x, &y);
			win->getSize(&w,&h);
			
			win->getRenderer()->iconify(
				_ascr->root, this, true,
				0,     /* t->icon_x_loc */
				0,     /* t->icon_y_loc */
				10,     /* t->icon_p_width */
				10,     /* t->icon_p_height */
				x,     /* t->frame_x */
				y,     /* t->frame_y */
				w,     /* t->frame_width */
				h      /* t->frame_height */
				);
			SendUnlockNotification(_fd);
		}
		else
		{
			SendUnlockNotification(_fd);
		}
		if (win != 0)
		{
			win->setIconified(true);
		}
		_iconifyGeometriesSet = false;
		break;
	}
	case M_DEICONIFY:
	{
		//std::cerr << "M_DEICONIFY\n";
		win = _metisseDesktop->findWindow((Window)body[1]);
		if (win != 0 && !(packet->size < 15 || body[5] == 0))
		{
			// we may want to schedule this when we recive
			// the unmap notification from the server ...
			win->getRenderer()->unHide();
			win->getRenderer()->iconify(
				_ascr->root, this, false,
				(int)body[3],     /* t->icon_x_loc */
				(int)body[4],     /* t->icon_y_loc */
				(int)body[5],     /* t->icon_p_width */
				(int)body[6],     /* t->icon_p_height */
				(int)body[7],     /* t->frame_x */
				(int)body[8],     /* t->frame_y */
				(int)body[9],     /* t->frame_width */
				(int)body[10]     /* t->frame_height */
				);
			#if 0
			fprintf(stderr,
				"DeIconify, args %d+%d+%dx%d %d+%d+%dx%d\n",
				(int)body[3],     /* t->icon_x_loc */
				(int)body[4],     /* t->icon_y_loc */
				(int)body[5],     /* t->icon_p_width */
				(int)body[6],     /* t->icon_p_height */
				(int)body[7],     /* t->frame_x */
				(int)body[8],     /* t->frame_y */
				(int)body[9],     /* t->frame_width */
				(int)body[10]     /* t->frame_height */
				);
			#endif
		}
		else if (win != 0 && _iconifyGeometriesSet)
		{
			win->getRenderer()->unHide();
			win->getRenderer()->iconify(
				_ascr->root, this, false,
				_iconifyGeometries[0],     /* t->icon_x_loc */
				_iconifyGeometries[1],     /* t->icon_y_loc */
				_iconifyGeometries[2],     /* t->icon_p_width */
				_iconifyGeometries[3],     /* t->icon_p_height */
				_iconifyGeometries[4],     /* t->frame_x */
				_iconifyGeometries[5],     /* t->frame_y */
				_iconifyGeometries[6],     /* t->frame_width */
				_iconifyGeometries[7]      /* t->frame_height */
				);
			#if 0
			fprintf(stderr,
				"DeIconify, args %d+%d+%dx%d %d+%d+%dx%d\n",
				_iconifyGeometries[0],     /* t->icon_x_loc */
				_iconifyGeometries[1],     /* t->icon_y_loc */
				_iconifyGeometries[2],     /* t->icon_p_width */
				_iconifyGeometries[3],     /* t->icon_p_height */
				_iconifyGeometries[4],     /* t->frame_x */
				_iconifyGeometries[5],     /* t->frame_y */
				_iconifyGeometries[6],     /* t->frame_width */
				_iconifyGeometries[7]      /* t->frame_height */
				);
			#endif
			_iconifyGeometriesSet = false;
		}
		else if (win != 0)
		{
			win->getRenderer()->unHide();
			float x,y;
			unsigned int w,h;
			win->getRealPosition(&x, &y);
			win->getSize(&w,&h);
			
			win->getRenderer()->iconify(
				_ascr->root, this, false,
				0,     /* t->icon_x_loc */
				0,     /* t->icon_y_loc */
				10,     /* t->icon_p_width */
				10,     /* t->icon_p_height */
				x,     /* t->frame_x */
				y,     /* t->frame_y */
				w,     /* t->frame_width */
				h      /* t->frame_height */
				);
		}
		if (win != 0)
		{
			win->setIconified(false);
		}
		_iconifyGeometriesSet = false;
		SendUnlockNotification(_fd);
		break;
	}
	default:
		/* ignore unknown packet */
		break;
    }
}

void FvwmModule::sendUnlockNotification(void)
{
	SendUnlockNotification(_fd);
}

void
FvwmModule::sendMetisseTransform(MetisseWindow *win, bool mark)
{
	if (!_sendTransformations)
	{
		return;
	}
	GLfloat f[18] = {
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0,
		0.0, 0.0
	} ;
	char cmd[512];

	win->getRendererFullTransformation(f);

	sprintf(cmd,"MetisseTransform %s %f %f %f %f %f %f "
		"%f %f %f %f %f %f "
		"%f %f %f %f %f %f", (mark)? "Mark":"State",
		f[0], f[1], f[2], f[3], f[4], f[5],
		f[6], f[7], f[8], f[9], f[10], f[11],
		f[12], f[13], f[14], f[15], f[16], f[17]);
	//fprintf(stderr,"%s\n", cmd);
	sendText(cmd, win);
}

void
FvwmModule::sendMetisseTransform(MetisseWindow *win, GLfloat *f, bool mark)
{
	if (!_sendTransformations)
	{
		return;
	}

	char cmd[512];
	sprintf(cmd,"MetisseTransform %s %f %f %f %f %f %f "
		"%f %f %f %f %f %f "
		"%f %f %f %f %f %f", (mark)? "Mark":"State",
		f[0], f[1], f[2], f[3], f[4], f[5],
		f[6], f[7], f[8], f[9], f[10], f[11],
		f[12], f[13], f[14], f[15], f[16], f[17]);
	//fprintf(stderr,"2: %s\n", cmd);
	sendText(cmd, win);
}

void
FvwmModule::react(Observable *obs)
{
  // std::cerr << "FvwmModule::react this=" << this << " obs=" << obs << " fk=" << _fk << std::endl ;
  if (obs==_fk && _fk->getState()&FileKeeper::R) {
    FvwmPacket* packet = ReadFvwmPacket(_fd[1]);
    if ( packet == NULL ) {
	 // In general we are here when fvwm exits
	 ReactiveEngine::stop() ;
    } else {
	 _processMessage(packet);
	 notifyObservers() ;
    }
  }
}

void
FvwmModule::sendText(const char *text, MetisseWindow *win)
{
	SendText(_fd, text, (win)? win->getFrameID() : 0);
}

void
FvwmModule::sendWindowList(void)
{
	SendInfo(_fd,"Send_WindowList",0);
}

void
FvwmModule::parseConfig(void)
{
	parseOptions();
}

FvwmModule::FvwmModule(AScreen *ascr, MetisseDesktop *metisseDesktop,
				   int fd0, int fd1,
				   char *myName,
				   std::string restartString)
{

	_myName = myName;
	_restartString = restartString ;
	_ascr = ascr;
	_metisseDesktop = metisseDesktop;
	_sendTransformations = false;
	_iconifyGeometriesSet = false;
	_inStartup = true;;

	_fd[0] = fd0;
	_fd[1] = fd1;

	_currentDesk = 0;

	/* the msg type we want from fvwm (see libs/Module.h and the fvwm
	 * www site for more msg and info) */
	SetMessageMask(
		_fd,
		M_STRING | M_NEW_DESK | M_NEW_PAGE | M_ADD_WINDOW |
		M_CONFIGURE_WINDOW | M_ICONIFY | M_DEICONIFY |
		M_RES_CLASS | M_RES_NAME | M_WINDOW_NAME | M_END_WINDOWLIST);
	/* extended messages */
	SetMessageMask(
		_fd,
		MX_STARTSTOP_OPERATION     |
		MX_MODULE_INTERACTIVE_MOVE |
		MX_METISSE_SET_TRANSFORM);

	/* should synchronouse for these msg */
	SetSyncMask(_fd, M_NEW_DESK| M_ICONIFY | M_DEICONIFY | M_NEW_PAGE);
	SetSyncMask(_fd, MX_STARTSTOP_OPERATION|MX_MODULE_INTERACTIVE_MOVE);
	SetNoGrabMask(_fd, M_NEW_DESK|M_ICONIFY|M_DEICONIFY);
	SetNoGrabMask(_fd,MX_STARTSTOP_OPERATION|MX_MODULE_INTERACTIVE_MOVE);

	_fk = FileKeeper::create(_fd[1], FileKeeper::R);
	subscribeTo(_fk);
	// std::cerr << "FvwmModule: using FileKeeper " << _fk << std::endl ;

	_ascr->setFvwmModule(this);

	//std::cerr << "Parse Options" << std::endl;
	//_parseOptions();
	//std::cerr << "Options Parsed" << std::endl;
	//SendInfo(_fd,"Send_WindowList",0);

	/* tell fvwm we're running */
	SendFinishedStartupNotification(_fd);
}
