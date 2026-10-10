/*
 *
 * FvwmModule.H --
 *
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef FvwmModule_H_
#define FvwmModule_H_

#include <nucleo/core/ReactiveObject.H>
#include <nucleo/core/FileKeeper.H>

#include <X11/Xlib.h>

#include "desktop/MetisseWindow.H"
#include "desktop/MetisseDesktop.H"
#include "AFvwm.H"

class AScreen;

class FvwmModule : public ReactiveObject {
  
private:

	char *_myName;
	
	int _fd[2];
	FileKeeper *_fk;

	long _currentDesk;
	bool _sendTransformations;

	AScreen *_ascr;
	MetisseDesktop *_metisseDesktop;

	std::string _restartString ;

	int _iconifyGeometries[8];
	bool _iconifyGeometriesSet;
	bool _inStartup;

	void _buildFacade(bool cutToo = false);
	void _processOperation(MetisseWindow *mWin, char *rest);
	void _processMessage(FvwmPacket* packet);
	void _parseMessageLine(Window id, char *line);

protected:
	void react(Observable *);

public:

	FvwmModule(AScreen *ascr, MetisseDesktop *metisseDesktop,
		   int fd0, int fd1,
		   char *myName,
		   std::string restartString) ;

	void cutWindow(MetisseWindow *mWin);
	void cutWindow(MetisseWindow *mWin, WinRegions::region *reg);
	void buildFacade(MetisseWindow*, bool cutToo = false);
	void buildFacade(
		MetisseWindow *mWin, WinRegions::region *reg, int x, int y,
		bool cutToo = false);
	void parseOptions(void);
	void sendUnlockNotification(void);
	void sendMetisseTransform(MetisseWindow *win, bool mark);
	void sendMetisseTransform(MetisseWindow *win, GLfloat *f, bool mark);
	void sendText(const char *text, MetisseWindow *win);
	void sendWindowList(void);
	void parseConfig(void);
} ;

#endif
