/*
 *
 * main/Persistence.cxx --
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

#include "Persistence.H"

#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <time.h>

#define P_FACADES_FILE_NAME ".facades"

static std::list<FacadeDescription *> allFacades;
static char *facadesFile = NULL;
static FacadeDescription *currentFD = 0;
static namedCut *currentNamedCut = 0;

static
bool _persistence_window_check_desc(FacadeDescription *fd, MetisseWindow *w);

static 
void _execute(char *action)
{
	char *rest;
	char *opt;

	if (action == 0 || action[0] == '#' || action[0] == '\0')
	{
		return;
	}

	opt = PeekToken(action, &rest);
	if (StrEquals(opt, "Facade"))
	{
		if (currentFD != 0)
		{
			if (currentNamedCut != 0)
			{
				currentFD->namedCutList.push_front(
					currentNamedCut);
				currentNamedCut = 0;
			}
			allFacades.push_back(currentFD);
			// fprintf(stderr,"loaded: %s\n", currentFD->name);
		}
		currentFD = new FacadeDescription();

		//  name
		opt = PeekToken(rest, &rest);
		if (opt == NULL || opt[0] == '\0')
		{
			fprintf(stderr,"Error in persitence file!\n");
			// leak!
			delete currentFD;
			currentFD = 0;
			return;
		}

		currentFD->name = strdup(opt);

		int val[2];
		if (!GetIntegerArguments(rest, NULL, val, 2) == 2)
		{
			fprintf(stderr,"Error in persitence file!\n");
			delete currentFD;
			currentFD = 0;
			return;
		}
		currentFD->automatic = (val[0])? true:false;
		currentFD->hideSources = (val[1])? true:false;

		
		return;
	}

	if (!StrEquals(opt, "+"))
	{
		fprintf(stderr,"_execute Unkown Command: %s\n", opt);
		return;
	}

	opt = PeekToken(rest, &rest);

	if ((StrEquals(opt, "W") || StrEquals(opt, "O")) && currentFD != 0)
	{
		bool loadOverFacade = false;

		if (StrEquals(opt, "O"))
		{
			loadOverFacade = true;
		}
		// window
		if (currentNamedCut != 0)
		{
			currentFD->namedCutList.push_front(currentNamedCut);
		}
		currentNamedCut = new namedCut();

		opt = PeekToken(rest, &rest);
		if (opt == NULL || opt[0] == '\0')
		{
			currentNamedCut->windowName = NULL;
		}
		else
		{
			currentNamedCut->windowName = strdup(opt);
		}

		opt = PeekToken(rest, &rest);
		if (opt == NULL)
		{
			fprintf(stderr,"Error in persitence file!\n");
			delete currentNamedCut;
			currentNamedCut = 0;
			return;
		}
		currentNamedCut->resClass = strdup(opt);

		
		opt = PeekToken(rest, &rest);
		if (opt == NULL)
		{
			fprintf(stderr,"Error in persitence file!\n");
			delete currentNamedCut;
			currentNamedCut = 0;
			return;
		}

		currentNamedCut->resName = strdup(opt);

		int val[2];
		if (GetIntegerArguments(rest, NULL, val, 2) != 2)
		{
			fprintf(stderr,"Error in persitence file!\n");
			delete currentNamedCut;
			currentNamedCut = 0;
			return;
		}
		currentNamedCut->width = val[0];
		currentNamedCut->height = val[1];

		if (loadOverFacade)
		{
			//std::cerr << "Over Facade Loaded\n";
			currentFD->overWin = currentNamedCut;
			currentNamedCut = 0;
		}
		return;
	}

	if (StrEquals(opt, "C") && currentNamedCut != 0)
	{
		int val[6];
		if (GetIntegerArguments(rest, NULL, val, 6) != 6)
		{
			fprintf(stderr,"Error in persitence file (cut)!\n");
		}

		currentNamedCut->cuts.push_back(
			cut(val[0],val[1],val[2],val[3],val[4],val[5]));
	}
}

static
bool _load_config(char *file)
{
	FILE* f;

	f = fopen(file, "r");
	if (f == NULL)
	{
		fprintf(stderr,"No %s file\n", file);
		return true;
	}

	char *tline;
	char line[1024];

	/* Set close-on-exec flag */
	fcntl(fileno(f), F_SETFD, 1);

	tline = fgets(line, (sizeof line) - 1, f);
	while (tline)
	{
		int l;
		while (tline && (l = strlen(line)) < sizeof(line) && l >= 2 &&
		      line[l-2]=='\\' && line[l-1]=='\n')
		{
			tline = fgets(line+l-2,sizeof(line)-l+1,f);
		}
		tline=line;
		while (isspace((unsigned char)*tline))
		{
			tline++;
		}
		l = strlen(tline);
		if (l > 0 && tline[l - 1] == '\n')
		{
			tline[l - 1] = '\0';
		}
		_execute(tline);
		tline = fgets(line, (sizeof line) - 1, f);
	}

	fclose(f);

	if (currentFD != 0)
	{
		if (currentNamedCut != 0)
		{
			currentFD->namedCutList.push_front(currentNamedCut);
		}
		allFacades.push_back(currentFD);
		//fprintf(stderr,"Loaded: %s\n", currentFD->name);
	}

	return true;
}

static
void _write_facades_file(void)
{
	FILE* f;

	if ((f = fopen(facadesFile, "w")) == NULL)
	{
		fprintf(stderr, "FvwmCompositor: Fail to (re)create file %s\n",
			facadesFile);
		return;
	}
	time_t t;
	time(&t);
	fprintf(f, "# Auto Generated \t%s\n", asctime(localtime(&t)));

	std::list<FacadeDescription *>::iterator iter;
	for (iter = allFacades.begin(); iter != allFacades.end(); iter++)
	{
		FacadeDescription *fd = (*iter);
		fprintf(f, "Facade \"%s\" %i %i\n", fd->name, fd->automatic,
			fd->hideSources);
		if (fd->overWin != 0)
		{
			fprintf(f, "+ O \"%s\" \"%s\" \"%s\" %d %d\n",
				(fd->overWin->windowName)?
				fd->overWin->windowName:"",
				fd->overWin->resClass,
				fd->overWin->resName,
				fd->overWin->width, fd->overWin->height);
		}
		std::list<namedCut *>::iterator jter;
		for (jter = fd->namedCutList.begin();
		     jter != fd->namedCutList.end(); jter++)
		{
			fprintf(f, "+ W \"%s\" \"%s\" \"%s\" %d %d\n",
				((*jter)->windowName)?  (*jter)->windowName:"",
				(*jter)->resClass,
				(*jter)->resName,
				(*jter)->width, (*jter)->height);
			cutlist cl = (*jter)->cuts;
			cutlist::iterator cli;
			for (cli = cl.begin(); cli != cl.end(); cli++)
			{
				fprintf(f, "+ C %i %i %i %i %i %i\n",
					(*cli).sx, (*cli).sy,
					(*cli).sw, (*cli).sh,
					(*cli).dx, (*cli).dy);
			}
		}
	}

	fclose(f);
}

void persistence_save_facade(
	MetisseWindow *win, MetisseDesktop *desktop, char *name, bool useWinName,
	bool automatic, bool hide)
{
	FacadeMapping wcm =  win->getFacade();
	FacadeDescription *fd = new FacadeDescription();
	FacadeRealisation *fr = new FacadeRealisation;

	FacadeMapping::iterator iter;
	for (iter = wcm.begin(); iter != wcm.end(); iter++)
	{
		MetisseWindow *w = dynamic_cast<MetisseWindow *>((*iter).first);
		if (w == 0)
		{
			continue;
		}
		fr->WindowCutList.insert(w);
		namedCut *nm = new namedCut();
		if (useWinName)
		{
			if (w->getWindowName())
			{
				nm->windowName = strdup(w->getWindowName());
			}
			else
			{
				fprintf(stderr,"Warnning: No window name\n");
			}
		}
		if (w->getResClass())
		{
			nm->resClass = strdup(w->getResClass());
		}
		else
		{
			fprintf(stderr,"Warnning: No resource class\n");
		}
		if (w->getResName())
		{
			nm->resName = strdup(w->getResName());
		}
		else
		{
			fprintf(stderr,"Warnning: No resource name\n");
		}
		w->getClientSize(&(nm->width), &(nm->height));
		// what about a memdup?
		cutlist cl = (*iter).second;
		cutlist::iterator cli;
		for (cli = cl.begin(); cli != cl.end(); cli++)
		{
			nm->cuts.push_front((*cli));
		}
		// must order (lexicographically) ?
		fd->namedCutList.push_front(nm);	
	}

	if (fd->namedCutList.size() == 0)
	{
		delete fd;
		delete fr;
		return;
	}

	
	fd->name = strdup(name);
	fd->automatic = automatic;
	fd->hideSources = hide;

	if (win->isAFacadeWindow())
	{
		desktop->setFacadeName(win, fd->name);
		fr->realWin = win;
		win->setFacadeRealisation(fd, fr);
	}
	else
	{
		fd->overWin = new namedCut();
		if (useWinName && win->getWindowName())
		{
			fd->overWin->windowName = strdup(win->getWindowName());
		}
		if (win->getResClass())
		{
			fd->overWin->resClass = strdup(win->getResClass());
		}
		if (win->getResName())
		{
			fd->overWin->resName = strdup(win->getResName());
		}
		win->getClientSize(
			&(fd->overWin->width), &(fd->overWin->height));
		fr->overRealWin = win;
		//win->setOverFacadeRealisation(fd);
	}
	fd->realisations.insert(fr);
	allFacades.push_front(fd);

	_write_facades_file();

	WindowMapping wins = desktop->getWindowMapping();
	WindowMapping::iterator i;
	bool changed;
	for (i = wins.begin(); i != wins.end(); i++)
	{
		MetisseWindow *w = (*i).second;
		changed |=_persistence_window_check_desc(fd, w);
	}

	if (!win->isAFacadeWindow())
	{
		win->setOverFacadeRealisation(fd);
	}

	for (i = wins.begin(); i != wins.end(); i++)
	{
		MetisseWindow *dw = (*i).second;
		if (dw->isUnmanaged())
		{
			continue;
		}
		dw->checkFacadeMenu();
	}
}

void persistence_unsave_facade(MetisseWindow *win, AScreen *ascr)
{
	if (win->hasAFacade() && !win->isAFacadeWindow())
	{
		FacadeDescription *fd = 
			win->deleteOverRealisedDescription();
		FacadeDescription *rfd = NULL;

		if (!fd)
		{
			return;
		}

		std::list<FacadeDescription *>::iterator iter;
		for (iter = allFacades.begin(); iter != allFacades.end(); iter++)
		{
			if (fd == (*iter))
			{
				rfd = fd;
				break;
			}
		}
		
		if (!rfd)
		{
			return;
		}
		allFacades.remove(fd);
		WindowMapping wins = ascr->getMetisseDesktop()->getWindowMapping();
		WindowMapping::iterator i;
		for (i = wins.begin(); i != wins.end(); i++)
		{
			MetisseWindow *ow = (*i).second;
			if (ow->deleteFacadeDescription(fd))
			{
				ow->buildSpecialMenu();
			}
		}
		delete fd;
		_write_facades_file();
		win->buildSpecialMenu();
	}
	else if (win->isAFacadeWindow())
	{
		FacadeDescription *fd = win->deleteRealisedDescription();
		FacadeDescription *rfd = NULL;

		if (!fd)
		{
			return;
		}

		std::list<FacadeDescription *>::iterator iter;
		for (iter = allFacades.begin(); iter != allFacades.end(); iter++)
		{
			if (fd == (*iter))
			{
				rfd = fd;
				break;
			}
		}
		
		if (!rfd)
		{
			return;
		}
		allFacades.remove(fd);
		WindowMapping wins = ascr->getMetisseDesktop()->getWindowMapping();
		WindowMapping::iterator i;
		for (i = wins.begin(); i != wins.end(); i++)
		{
			MetisseWindow *ow = (*i).second;
			if (ow->deleteFacadeDescription(fd))
			{
				ow->buildSpecialMenu();
			}
		}
		delete fd;
		_write_facades_file();
		win->buildSpecialMenu();
	}
}

static
bool _match_namedCut(MetisseWindow *w, namedCut *nc)
{
	int width,height;
	w->getClientSize(&width, &height);

	#if 0
	fprintf(stderr, "_match_namedCut :%s: :%s: :%s: %i %i (win)\n",
		w->getWindowName(), w->getResClass(), w->getResName(),
		width, height);
	fprintf(stderr, "_match_namedCut :%s: :%s: :%s: %i %i (conf)\n",
		nc->windowName, nc->resClass, nc->resName,
		nc->width, nc->height);
	#endif
	if ((nc->windowName == NULL ||
	     matchWildcards(nc->windowName, w->getWindowName())) &&
	    StrEquals(nc->resClass, w->getResClass()) &&
	    StrEquals(nc->resName, w->getResName()) && 
	    (nc->width == width) && nc->height == height)
	{
		return true;
	}

	return false;
}

void persitence_facade_realised(MetisseWindow *win, unsigned long facadeReal)
{
	// fprintf(stderr, " persitence_facade_realised\n");
	std::list<FacadeDescription *>::iterator iter;
	FacadeRealisation *fr = (FacadeRealisation *)facadeReal;
	FacadeDescription *f_fd = NULL;

	if (!fr)
	{
		return;
	}
	for (iter = allFacades.begin(); iter != allFacades.end(); iter++)
	{
		FacadeDescription *fd = (*iter);

		if (fd->realisations.find(fr) != fd->realisations.end())
		{
			// fprintf(stderr, "FOUND!\n");
			fr->realWin = win;
			win->setFacadeRealisation(fd, fr);
			break;
		}
	}
}

static
void _recall_facade(
	FacadeDescription *recallFD, MetisseWindow *w, char *name, AScreen *ascr)
{
	char cmd[1024];
	char tmp[128];
	FacadeRealisation *fr = new FacadeRealisation;

	if (recallFD->overWin == 0)
	{
		sprintf(cmd,"Exec facade-holder -c -a %lu -n \"%s\"",
			(unsigned long)fr, recallFD->name);
		
		std::list<namedCut *>::iterator jter;
		for (jter = recallFD->namedCutList.begin();
		     jter != recallFD->namedCutList.end(); jter++)
		{
			namedCut *nc = (*jter);

			// FIXME: we take the first!
			MetisseWindow *win = (*((nc->matchWin).begin()));
			fr->WindowCutList.insert(win);
			sprintf(tmp," -i 0x%lx", win->getClientID());
			strcat(cmd,tmp);
			cutlist cl = nc->cuts;
			cutlist::iterator cli;
			for (cli = cl.begin(); cli != cl.end(); cli++)
			{
				sprintf(tmp," -s %i %i %i %i %i %i",
					(*cli).sx, (*cli).sy, (*cli).sw,
					(*cli).sh, (*cli).dx, (*cli).dy);
				strcat(cmd,tmp);
			}
			if (recallFD->hideSources)
			{
				win->hideAsFacadeSource(true);
			}
		}
		fprintf(stderr, "Executing %s\n", cmd);
		recallFD->constructedCount++;
		ascr->sendFvwmCmd(cmd, 0);
		// call to setFacadeRealisation cannot be done here
		// see MetisseDesktop!
	}
	else
	{
		//fprintf(stderr, "Recall Over facade %s\n", name);
		fr->overRealWin = w;
		FacadeMapping wcm;
		wcm.clear();
		w->setFacade(wcm, false);
		ascr->getMetisseDesktop()->windowUpdateRequest(w);
		std::list<namedCut *>::iterator jter;
		for (jter = recallFD->namedCutList.begin();
		     jter != recallFD->namedCutList.end(); jter++)
		{
			namedCut *nc = (*jter);

			// FIXME: we take the first!
			MetisseWindow *win = (*((nc->matchWin).begin()));
			fr->WindowCutList.insert(win);
			cutlist cl = nc->cuts;
			cutlist::iterator cli;
			for (cli = cl.begin(); cli != cl.end(); cli++)
			{
				// add the cut over the window
				WinRegions::region r = WinRegions::region(
					(*cli).sx, (*cli).sy, (*cli).sw,
					(*cli).sh);
				ascr->getMetisseDesktop()->addOverFacade(
					&r ,win, w, (*cli).dx, (*cli).dy);
			}
			if (recallFD->hideSources)
			{
				win->hideAsFacadeSource(true);
			}
		}
		w->setOverFacadeRealisation(recallFD);
		recallFD->constructedCount++;
	}
	recallFD->realisations.insert(fr);
}

void persistence_recall_facade(MetisseWindow *w, char *name, AScreen *ascr)
{
	
	std::list<FacadeDescription *>::iterator iter;
	FacadeDescription *recallFD = 0;

	// fprintf(stderr,"Recall %s\n",name);
	for (iter = allFacades.begin(); iter != allFacades.end(); iter++)
	{
		FacadeDescription *fd = (*iter);
		if (!fd->matched || name == 0 || fd->name == 0 ||
		    strcmp(name,fd->name) != 0)
		{
			continue;
		}
		if (fd->overWin != 0)
		{
			if (_match_namedCut(w, fd->overWin))
			{
				recallFD = fd;
				break;
			}
		}
		std::list<namedCut *>::iterator jter;
		for (jter = fd->namedCutList.begin();
		     jter != fd->namedCutList.end(); jter++)
		{
			if (_match_namedCut(w, (*jter)))
			{
				recallFD = fd;
				break;
			}
		}
		if (recallFD != 0)
		{
			break;
		}
	}

	if (recallFD == 0)
	{
		return;
	}

	_recall_facade(recallFD, w, name, ascr);
}

void _remove_over_realistaion(
	FacadeDescription *fd, MetisseWindow *w, AScreen *ascr)
{
	WindowMapping wins = ascr->getMetisseDesktop()->getWindowMapping();

	std::set<FacadeRealisation *>::iterator iter;
	for (iter = fd->realisations.begin(); iter != fd->realisations.end();
	     iter++)
	{
		FacadeRealisation *fr = (*iter);

		if (!fr)
		{
			continue;
		}
		if (fr->overRealWin == w)
		{
			fd->realisations.erase(fr);
			std::set<MetisseWindow *>::iterator jter;
			if (fd->hideSources)
			{
				for (jter = fr->WindowCutList.begin();
				     jter != fr->WindowCutList.end();
				     jter++)
				{
					MetisseWindow *win = (*jter);
					WindowMapping::iterator i =
						wins.find(win->getFrameID()) ;
					if (i==wins.end())
					{
						continue;
					}
					win->hideAsFacadeSource(false);
				}
			}
			delete fr;
		}
	}
}

void _remove_realistaion(FacadeDescription *fd, MetisseWindow *w, AScreen *ascr)
{
	WindowMapping wins = ascr->getMetisseDesktop()->getWindowMapping();

	std::set<FacadeRealisation *>::iterator iter;
	for (iter = fd->realisations.begin(); iter != fd->realisations.end();
	     iter++)
	{
		FacadeRealisation *fr = (*iter);
		
		if (!fr)
		{
			continue;
		}
		if (fr->realWin == w)
		{
			fprintf(stderr, "FIXME: _remove_realistaion realWin\n");
			if (fd->hideSources)
			{
				std::set<MetisseWindow *>::iterator jter;
				for (jter = fr->WindowCutList.begin();
				     jter != fr->WindowCutList.end();
				     jter++)
				{
					MetisseWindow *win = (*jter);
					WindowMapping::iterator i =
						wins.find(win->getFrameID()) ;
					if (i == wins.end())
					{
						continue;
					}
					win->hideAsFacadeSource(false);
				}
			}
			// FIXME: this obviouslt cause a core dump
			// under a unmap/map ... but we lets leak
			w->deleteRealisedDescription();
			return;
		}
		if (fr->WindowCutList.find(w) != fr->WindowCutList.end())
		{
			fr->WindowCutList.erase(w);
			fr->complet = false;
			if (fr->WindowCutList.size() == 0)
			{
				fd->realisations.erase(fr);
				delete fr;
			}
			else if (fd->hideSources)
			{
				std::set<MetisseWindow *>::iterator jter;
				for (jter = fr->WindowCutList.begin();
				     jter != fr->WindowCutList.end();
				     jter++)
				{
					MetisseWindow *win = (*jter);
					WindowMapping::iterator i =
						wins.find(win->getFrameID()) ;
					if (i == wins.end())
					{
						continue;
					}
					win->hideAsFacadeSource(false);
				}
			}
		}
	}
}

void persistence_window_remove(MetisseWindow *w, AScreen *ascr)
{
	bool changed = false;
	bool local_change = false;
	std::list<FacadeDescription *>::iterator iter;

	// fprintf(stderr, "persistence_window_remove %s\n", w->getWindowName());
	for (iter = allFacades.begin(); iter != allFacades.end(); iter++)
	{
		FacadeDescription *fd = (*iter);
		bool matched = true;

		local_change = false;
		if (fd->overWin != 0)
		{
			if (fd->overWin->matchWin.find(w) !=
			    fd->overWin->matchWin.end())
			{
				if (w->removeMatchingFacadeOver(fd))
				{
					// remove a matchning
					//fprintf(stderr,
					//	"Removed Over Match\n");
					local_change = changed = true;
					_remove_over_realistaion(fd, w, ascr);
					fd->overWin->matchWin.erase(w);
				}
			}
			if (fd->overWin->matchWin.empty())
			{
				matched = false;
			}
		}
		else
		{
			std::set<FacadeRealisation *>::iterator kter;
			for (kter = fd->realisations.begin();
			     kter != fd->realisations.end();
			     kter++)
			{
				FacadeRealisation *fr = (*kter);
		
				if (!fr)
				{
					continue;
				}
				if (fr->realWin == w)
				{
					_remove_realistaion(fd, w, ascr);
					return;
				}
			}
			
		}
		std::list<namedCut *>::iterator jter;
		bool no_match_at_all = true;
		for (jter = fd->namedCutList.begin();
		     jter != fd->namedCutList.end(); jter++)
		{
			if ((*jter)->matchWin.find(w) !=
			    (*jter)->matchWin.end())
			{
				//fprintf(stderr, "Found Remove Match\n");
				if (fd->overWin != 0)
				{
					if (w->removeMatchingFacadeOver(fd))
					{
						// new matching
						//fprintf(stderr,
						//	"Removed Over "
						//	"Matching Win\n");
						local_change = changed = true;
						_remove_realistaion(
							fd, w, ascr);
						(*jter)->matchWin.erase(w);
					}
				}
				else
				{
					if (w->removeMatchingFacade(fd))
					{
						// remove a matchning
						local_change = changed = true;
						//fprintf(stderr,
						//	"Removed Matching\n");
						_remove_realistaion(
							fd, w, ascr);
						(*jter)->matchWin.erase(w);
					}
				}
			}
			if ((*jter)->matchWin.empty())
			{
				matched = false;
			}
			else
			{
				no_match_at_all = false;
			}
		}

		fd->matched = matched;

		if (local_change  && fd->overWin != 0 && no_match_at_all)
		{
			WindowMapping wins =
				ascr->getMetisseDesktop()->getWindowMapping();
			WindowMapping::iterator j;
			for (j = wins.begin(); j != wins.end(); ++j)
			{
				MetisseWindow *ow = (*j).second;
				//fprintf(stderr, "Over Removed Match\n");
			}
		}
	}

	if (!changed || ascr == 0)
	{
		return;
	}

	WindowMapping wins = ascr->getMetisseDesktop()->getWindowMapping();
	WindowMapping::iterator j;
	for (j = wins.begin(); j != wins.end(); j++)
	{
		MetisseWindow *dw = (*j).second;
		if (dw->isUnmanaged())
		{
			continue;
		}
		dw->checkFacadeMenu();
	}
}


static
bool _persistence_window_check_desc(FacadeDescription *fd, MetisseWindow *w)
{
	bool matched = true;
	bool changed = false;

	//std::cerr << "CHECK\n";
	if (fd->overWin != 0)
	{
		//fprintf(stderr, "Over _match_namedCut? %s %s\n",
		//	w->getWindowName(),fd->overWin->windowName);

		if (_match_namedCut(w, fd->overWin))
		{
			if (w->addMatchingFacadeOver(fd))
			{
				// new matching
				changed = true;
				fd->overWin->matchWin.insert(w);
				w->setFacadeOverWin(fd);
			}
			//fprintf(stderr, "Over Match %s %i\n",
			//	w->getWindowName(), changed);
		}
		else if (fd->overWin->matchWin.find(w) !=
			 fd->overWin->matchWin.end())
		{
			if (w->removeMatchingFacadeOver(fd))
			{
				// remove a matchning
				changed = true;
				fd->overWin->matchWin.erase(w);
			}
		}
		if (fd->overWin->matchWin.empty())
		{
			matched = false;
		}
	}
	std::list<namedCut *>::iterator jter;
	for (jter = fd->namedCutList.begin();
	     jter != fd->namedCutList.end(); jter++)
	{
		if (_match_namedCut(w, (*jter)))
		{
			//std::cerr << "Match " << w->getWindowName()
			//	  << " " <<  fd->overWin << "\n";
			if (fd->overWin != 0)
			{
				if (w->addMatchingFacadeOver(fd))
				{
					// new matching
					changed = true;
					(*jter)->matchWin.insert(w);
				}
			}
			else
			{
				if (w->addMatchingFacade(fd))
				{
					// new matching
					changed = true;
					(*jter)->matchWin.insert(w);
				}
			}
		}
		else if ((*jter)->matchWin.find(w) != (*jter)->matchWin.end())
		{
			
			if (fd->overWin != 0)
			{
				if (w->removeMatchingFacadeOver(fd))
				{
					// remove a matchning
					changed = true;
					(*jter)->matchWin.erase(w);
				}
			}
			else
			{
				if (w->removeMatchingFacade(fd))
				{
					// remove a matchning
					changed = true;
					(*jter)->matchWin.erase(w);
				}
			}
		}
		if ((*jter)->matchWin.empty())
		{
			matched = false;
		}
	}
	fd->matched = matched;

	return changed;
}

void persistence_window_check(MetisseWindow *w, bool no_auto, AScreen *ascr)
{
	#if 0
	fprintf(stderr, "persistence_window_check %s %s %s\n",
		w->getWindowName(), w->getResClass(), w->getResName());
	#endif
	bool changed = false;
	std::list<FacadeDescription *>::iterator iter;
	for (iter = allFacades.begin(); iter != allFacades.end(); iter++)
	{
		FacadeDescription *fd = (*iter);
		changed |= _persistence_window_check_desc(fd, w);
	}

	if (!changed || no_auto || ascr == 0)
	{
		return;
	}

	WindowMapping wins = ascr->getMetisseDesktop()->getWindowMapping();
	WindowMapping::iterator j;
	for (j = wins.begin(); j != wins.end(); j++)
	{
		FacadeDescription *recallFD = 0;
		MetisseWindow *dw = (*j).second;
		if (dw->isUnmanaged() || ascr == NULL)
		{
			continue;
		}
		dw->checkFacadeMenu();

		if (no_auto)
		{
			continue;
		}
		for (iter = allFacades.begin(); iter != allFacades.end(); iter++)
		{
			FacadeDescription *fd = (*iter);
			if (!fd->matched || !fd->automatic)
			{
				continue;
			}
			//fprintf(stderr, "Check auto\n");
			recallFD = 0;
			if (fd->overWin != 0)
			{
				bool already_done = false;

				if (!_match_namedCut(dw, fd->overWin))
				{
					continue;
				}
				std::set<FacadeRealisation *>::iterator j;
				for (j = fd->realisations.begin();
				     j != fd->realisations.end();
				     j++)
				{

					FacadeRealisation *fr = (*j);

					if (!fr)
					{
						continue;
					}
					if (fr->overRealWin == w &&
					    fr->complet)
					{
						already_done = true;
						break;
					}
				}
				if (already_done)
				{
					fprintf(stderr, "Already done!\n");
					continue;
				}
				recallFD = fd;
			}
			else
			{
				// FIXME!
				#if 0
				std::list<namedCut *>::iterator jter;
				for (jter = fd->namedCutList.begin();
				     jter != fd->namedCutList.end(); jter++)
				{
					if (!_match_namedCut(dw, (*jter)))
					{
						continue;
					}
					
					recallFD = fd;
				}
				#endif
			}
			if (recallFD != 0)
			{
				//fprintf(stderr, "_recall_facade %s\n",
				//	fd->name);
				_recall_facade(recallFD, dw, fd->name, ascr);
			}
		}
	}
}

void persistence_checkall(AScreen *ascr, MetisseDesktop *desktop)
{
	WindowMapping wins = desktop->getWindowMapping();

	WindowMapping::iterator j;
	for (j = wins.begin(); j != wins.end(); ++j)
	{
		MetisseWindow *dw = (*j).second;
		if (dw->isUnmanaged())
		{
			continue;
		}
		persistence_window_check(dw, false, ascr);
	}
	for (j = wins.begin(); j != wins.end(); ++j)
	{
		MetisseWindow *dw = (*j).second;
		if (dw->isUnmanaged() || dw->isRootWindow())
		{
			continue;
		}
		dw->checkFacadeMenu();
	}
}

void persistence_init(void)
{
	char *dir = getenv("FVWM_USERDIR");
	
	if (dir == NULL || strlen(dir) == 1)
	{
		fprintf(stderr, "persistence_init: Initialization Failed!!\n");
		return;
	}

	int len = strlen(dir) + 1 + strlen(P_FACADES_FILE_NAME) + 1;
	
	facadesFile = (char *)malloc(len);

	sprintf(facadesFile, "%s/%s", dir, P_FACADES_FILE_NAME);

	_load_config(facadesFile);

	return;
} 
