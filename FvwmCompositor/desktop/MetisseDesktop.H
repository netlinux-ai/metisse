/*
 *
 * desktop/MetisseDesktop.H --
 *
 * Copyright (C) Nicolas Roussel
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */


#ifndef _MetisseDesktop_H
#define _MetisseDesktop_H

#include <string>
#include <map>

#include <nucleo/core/ReactiveObject.H>
#include <nucleo/gl/scenegraph/sgNode.H>
#include <nucleo/gl/window/glWindow.H>

#include <X11/Xmd.h>

#include "MetisseWindow.H"
#include "MetisseSource.H"
#include "main/AUtils.H"
#include "main/ACursor.H"


class AScreen;
typedef std::map<Window, MetisseWindow*> WindowMapping ;

class MetisseDesktop : public ReactiveObject {

private:

	std::string _style ;
	//sgNode *_parent ;

	Display *_xdisplay ;
	char *_msDisplayName;
	Window _xroot;

	WindowMapping _windows ;

	MetisseSource *_metisseSource;
	AScreen *_ascr;
	ACursor *_cursor;

	bool _modifierPressed[256];

	void _addWindow(
		Window id, int x, int y, unsigned int width, unsigned int height,
		bool map, int isroot);
	void _handleWindowFlags(
		MetisseWindow *win, Window transientFor, Window unmanagedFor,
		Window grabWin, Window duplicateFor, unsigned long facadeReal,
		unsigned long flags);
	void _closeWindow(MetisseWindow *win);
	void _setSubFacadeProperty(Window win, FacadeMapping facade);

protected:

  void react(Observable *obs) ;

public:

     std::string desktopName ;

	MetisseWindow *findWindow(Window id);
	WindowMapping getWindowMapping(void);
	bool isWindowValide(MetisseWindow *win);

	void handleConfigureWindow(
		Window id, int x, int y, int width, int height, int isroot);
	void handleCursorPosition(int x, int y);
	void handleImageFramebufferUpdate(
		MetisseWindow *win, bool isRoot, Image *img, int x, int y,
		unsigned int w, unsigned int h);
	void handleWindowShape(MetisseWindow *win, CARD16 *buf, int size);
	void handleWindowFacades(MetisseWindow *win, CARD32 *buf, CARD32 ncuts);
	void handleUnmapWindow(Window id);
	void handleDestroyWindow(Window id);
	void handleRestackWindow(
		Window id, Window nextId, Window transientFor,
		Window unmanagedFor, Window grabWin, Window duplicateFor,
		unsigned long facadeReal, unsigned long flags);

	void windowUpdateRequest(MetisseWindow *w);

	void setCursor(ACursor *cursor);

	void rootPointerEvent(int x, int y, unsigned long button_mask);
	void rootKeyEvent(unsigned long key, bool down_flag);
	void setModifiers(glWindow::event e, bool down_flag);
	void releaseModifiers(void);

	void getSize(unsigned int *width, unsigned int *height);

	void setAScreen(AScreen *ascr);
	char *getMetisseServerDisplay(void);
	void disconnect(void);

	MetisseDesktop(
		std::string rfburi, std::string style, bool use_shm,
		const char *encoding);
	MetisseDesktop(std::string rfburi);

	Bool HandleXCursor(
		int xhot, int yhot, int width, int height, char *buf,
		const rfbXCursorColors &colors);
	Bool HandleARGBCursor(
		int xhot, int yhot, int width, int height, char *buf);

	void addOverFacade(
		WinRegions::region *r, MetisseWindow *src_win,
		MetisseWindow *dest_win, int dest_x, int dest_y);
	void addFacade(
		WinRegions::region *r, MetisseWindow *src_win,
		MetisseWindow *dest_win, GLdouble dest_x, GLdouble dest_y,
		int pointer_x, int pointer_y);
	void rmFacade(
		cut c, MetisseWindow *src_win, MetisseWindow *dest_win);
	void setFacadeName(MetisseWindow *w, char *name);
} ;

#endif
