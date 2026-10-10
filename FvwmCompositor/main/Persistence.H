#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "AScreen.H"
#include "desktop/MetisseDesktop.H"

using namespace nucleo ;

class namedCut
{
public:
	char *windowName;
	char *resClass;
	char *resName;
	int width, height;
	std::set<MetisseWindow *> matchWin;
	cutlist cuts;
	int matchCount;
	namedCut(void)
	{
		matchCount = 0;
		windowName = NULL;
		resClass = NULL;
		resName = NULL;
	};
};


class FacadeRealisation
{
public:
	MetisseWindow *overRealWin; // over facade
	MetisseWindow *realWin; // facade
	std::set<MetisseWindow *> WindowCutList;
	bool complet;
	FacadeRealisation(void)
	{
		overRealWin = 0;
		realWin = 0;
		complet = true;
	}
};

class FacadeDescription
{
public:
	char *name;
	namedCut *overWin;
	std::list<namedCut *> namedCutList;
	// options
	bool automatic;
	bool hideSources;
	// dynamic
	int constructedCount;
	bool matched;
	bool dirty;
	std::set<FacadeRealisation *> realisations;
	FacadeDescription(void)
	{
		overWin = 0;
		matched = false;
		dirty = false;
		constructedCount = 0;
	}
};

void persitence_facade_realised(MetisseWindow *win, unsigned long facadeReal);
void persistence_save_facade(
	MetisseWindow *win, MetisseDesktop *desktop, char *name, bool useWinName,
	bool automatic, bool hide);
void persistence_unsave_facade(MetisseWindow *win, AScreen *ascr);
void persistence_recall_facade(MetisseWindow *w, char *name, AScreen *ascr);
void persistence_window_remove(MetisseWindow *w, AScreen *ascr);
void persistence_window_check(MetisseWindow *w, bool instartup, AScreen *ascr);
void persistence_checkall(AScreen *ascr, MetisseDesktop *desktop);
void persistence_init(void);

#endif
