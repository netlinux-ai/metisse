/*
 *
 * main/AScreen.H --
 *
 * Copyright (C) Nicolas Roussel
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef AScreen_H
#define AScreen_H

#include <nucleo/gl/window/glWindow.H>
#include <nucleo/gl/scenegraph/sgNode.H>
#include <nucleo/gl/scenegraph/sgViewpoint.H>
#include <nucleo/image/sink/ImageSink.H>
#include <nucleo/core/ReactiveEngine.H>
#include <nucleo/core/TimeKeeper.H>

#include "renderer/WindowRenderer.H"
#include "renderer/WindowShaders.H"

#ifdef HAVE_SPI
#include "main/WidgetManager.H"
#else
typedef void Accessible;
#endif

#include "LayerManager.H"
#include "AUtils.H"
#include "AWidgets.H"

#define DEBUG_PUSH_NAME 0

#define DRAG_ROTATE_NO 0
#define DRAG_ROTATE_Z  1
#define DRAG_ROTATE_X  2
#define DRAG_ROTATE_Y  3
#define DRAG_ROTATE_XY 4

typedef enum
{
	TMP_RESTACK_MODE_FOLD,
	TMP_RESTACK_MODE_SCALE,
	TMP_RESTACK_MODE_ROTATE
} restack_mode_t;

#define TMP_RESTACK_MODE_FIRST TMP_RESTACK_MODE_FOLD
#define TMP_RESTACK_MODE_LAST  TMP_RESTACK_MODE_ROTATE

class MetisseDesktop;
class ExtendedInputs;

using namespace nucleo ;

class AScreen : public ReactiveObject {

 private:
	DefaultSettings *_settings;
	bool _useTransparency;
	bool _orthoProj;
	bool _shapeWindow;
	bool _portrait;
	bool _force_fullscreen;

	PerfMeter *_perfMeter;
	ACursor *_cursor;
	AXorDraw *_stroke;
	DropWindow *_dropWindow;
	VMouse *_vMouse;
	bool _vMouseMove;

	GLint _winwidth;
	GLint _winheight;
	unsigned int _desktopWidth, _desktopHeight;

	unsigned long _button_mask;
	unsigned long _modifier_mask;

	bool _swallow_release;
	bool _motion;
	bool _moving;
	bool _rotate;
	int _rotateType;
	bool _rotateTmp;
	float _dragRotateTotaleX,_dragRotateTotaleY, _dragRotateTotaleZ;
	GLfloat _savedTrans[18];
	bool _folding;

	bool _selectingRegion;
	std::list<WinRegions::region > _widgetRegs;
	WinRegions::region _dropRegion;
	int _ssr_x;
	int _ssr_y;
	bool _srSnap;
	unsigned int _srSnapDist;
	int _srKeyCount;

	int _prevX;
	int _prevY;
	int _currX;
	int _currY;
	int _pressX;
	int _pressY;
	bool _forceDoublePointerEvent;

	bool _breakDisplayLists;
	bool _saveFrame;
	bool _isFvwmModule;

	bool _inPseudoSelectionMode;
	bool _doProjectCursor;

	bool _wmMove;
	bool _wmResize;
	float _resizeTransparency;
	int _opsDx;
	int _opsDy;
	MetisseWindow *_opWin;

	restack_mode_t _tmpRestackMode;
	bool _inTmpRestack;
	MetisseWindow *_restackWin;

	bool _ext_inputs;
	ExtendedInputs *_extInputs;


#ifdef HAVE_SPI
	WidgetManager *_widgetManager;
#else
	void *_widgetManager;
#endif
	int _actionX;
	int _actionY;
	MetisseWindow *_actionWin;

	// internal move
	bool _inInternalMove;
	MetisseWindow *_imWindow;
	int _imStartX, _imStartY;
	bool _waitForAConfigureWindow;
	bool _inClickMotion;
	MetisseWindow *_clickMotionWindow;
	bool _tmpInternalMove;
	TimeKeeper *_pageSwitchWinMove;
	int _pswmWarpX,_pswmWarpY;
	int _pswmNPX, _pswmNPY, _pswmPTX, _pswmPTY;
	bool _pswmInWarp;
	int _snapXOffset, _snapYOffset;
	int _doSGoX,_doSGoY;
	MetisseWindow *_sgWinX,  *_sgWinY;
	int _sgoStart;
	int _sgWarpX, _sgWarpY;
	int _sgWaitWarpX, _sgWaitWarpY;
	bool _sgInWarp;
	WindowRenderer *_snapFRWinX, *_snapFRWinY;
	bool _snapForbX, _snapForbY;
	bool _snapForbid;
	
	// FvwmTrace
	bool _fvwmTraceRunning;

	// clip op
	bool _clipIn;
	TimeKeeper *_clipOpTk;
	TimeKeeper *_clipOpFastTk;
	int _clipOpType;
	int _clipX,_clipY;
	unsigned long  _clipKeysym ;
	bool _clipOpWaitForCtrlReleased;
	bool _clipAutoRepeatOff;
	// interactive scale
	bool _inInteractiveScale;
	int _interactiveGravity;
	bool _tmpInteractiveScale;
	int _isDX,_isDY;

	// motion intercept
	bool _miIn;
	int _miStartX,_miStartY;
	TimeKeeper *_motionInterceptTk;
	int _miTkcount;
	DropWindow *_miFeed;
	unsigned _right_long_press_timeout;

	// replay
	int *_replay_x;
	int *_replay_y;
	int _replay_size;
	int _replay_i;

	// drag and drop event
	bool _inDragAndDrop;
	int _inDNDInterceptReleases;
	bool _inDNDragtoFreeDnd;
	unsigned long _dnd_button_mask;
	TimeKeeper *_dndRaiseTk;
	TrailWidget *_dndTrailing;
	MetisseWindow *_current_dnd_win;

	// scroll leafing
	ScrollLeafingWidget *_scrollLeafingW;
	TimeKeeper *_popScrollLeafingTk;
	int _xpopSL;
	int _ypopSL;


	sgViewpoint *_viewpoint;

	WindowRenderer *_selection;
	GLuint _selectionBuffer[256] ;
	int _selectionBufferSize;

	// for toolglass transformation
	WindowRenderer *_over_selection;
	GLuint _over_selectionBuffer[256] ;
	GLuint _over_selectionBufferSize;

	// the managed selection
	WindowRenderer *_managed_selection;
	GLuint _managed_selectionBuffer[256] ;
	GLuint _managed_selectionBufferSize;
	WindowRenderer *_dnd_managed_selection;

	MetisseWindow *_menuDragMasterWin;
	bool _inMenuDrag;

	bool _resetSelectionScheduled;
	bool _pagerModeOffScheduled;

	bool _noStencil;

	// video capture
	ImageSink *_capture ;
	TimeKeeper *_capture_timer ;
	Image _capture_image ; // reuse the same image to avoid memory allocation


	FvwmModule *_fvwmModule;
	MetisseDesktop *_metisseDesktop;
	WindowShaders * _winShaders;

public:
	glWindow *compositor;
	LayerManager *root;

	//
public:
	void setTitle(const char *title);
	void setGeometry(unsigned int width, unsigned int height);
	void makeCurrent(void);
	void useSoftwareCursor(void);
	void warpCursor(int x, int y);
	//
public:
	MetisseDesktop *getMetisseDesktop(void) { return _metisseDesktop; };
	WindowShaders *getWinShaders(void) { return _winShaders; };
	void getDesktopSize(unsigned int *width, unsigned int *height);
	bool hasStencilBuffer(void);
	//
public:
	void addACursor(ACursor *c);
	void removeACursor(ACursor *c);
	void stroke(bool on, bool replay = false);
	void startVideoCapture(char *captureSink);
	void stopVideoCapture(void);
	void saveFrame(void);
	void showFPS(void);
	void showVMouse(int width = 65, int height = 65);
private:
	void _strokeAdd(int x, int y);
	//
public:
	void setFvwmModule(FvwmModule *m);
	void sendFvwmCmd(const char *cmd, MetisseWindow *w);
	void sendMetisseTransform(MetisseWindow *win, bool mark);
	//
public:
	bool initWidgetManager(void);
	Accessible *registerToWidgetManager(MetisseWindow *win);
	void unregisterToWidgetManager(MetisseWindow *win);
	WinRegions::region getWidgetExtent(MetisseWindow *win, int x, int y);
	void printWidgets(MetisseWindow *win, int verb = 1);
	std::list<WinRegions::xyregion > getWidgetRegionsXY(MetisseWindow *win);
	std::list<WinRegions::region > getWidgetRegionsX11(MetisseWindow *win);
	void actionOnWidget(MetisseWindow *win);
	void setWidgetValuator(MetisseWindow *win, double speed);
	void setAllWidgetValuator(MetisseWindow *win, double speed);
	void actionOnComboBox(MetisseWindow *win, char *action);
	void actionOnFacade(MetisseWindow *win, char *action);
	//
private:
	void _checkSelectedIcons(void);
	//
private:
	void _x11project(int x, int y, int *px, int *py);
	void _x11projectOverWindow(
		MetisseWindow *win, int x, int y, int *px, int *py);
	void _myPointerEvent(int x, int y, unsigned long press_mask = 0);
	int _myPickClosest(
		int x, int y, GLuint *sbuffer, GLuint *over_sbuffer,
		GLuint *over_size, GLuint *managed_sbuffer,
		GLuint *managed_size, GLuint size);
	bool _isOverWinDecor(MetisseWindow *win, int x, int y);
	void _dragResetSelection(bool sendEvent);
public:
	MetisseWindow *getSelectedWindow(void);
	MetisseWindow *getSelectedFacade(cut *fcut);
	bool doPseudoSelection(void);
	void forceDoublePointerEvent(void);
	void resetSelection(bool sendEvend = true);
	void resetSelection(int x, int y);
	void scheduleResetSelection(void);
	bool unprojectOverWindow(
		MetisseWindow *win, int x, int y, GLdouble *ox,
		GLdouble *oy, GLdouble *oz);
	bool projectOverWindow(
		MetisseWindow *win, int x, int y, GLdouble *px, GLdouble *py,
		GLdouble *pz);
	void pointerEventOverWindow(MetisseWindow *win, int x, int y);
	unsigned long getButtonMask(void);
	void getPointerPosition(int *x, int *y);
	void getPrevPointerPosition(int *x, int *y);
	bool doProjectCursor(void);
	void fixCursorPos(bool start, MetisseWindow *metisseWin = 0);
	int obc2x11scX(GLfloat x, GLfloat hw, GLfloat rx);
	int obc2x11scY(GLfloat y, GLfloat hh, GLfloat ry);
	//
public:
	int extInputSendEvents(int x, int y, char button_mask, int num);
	void configureExtInput(MetisseWindow *win, char *name, char *opt);
	bool generateExtInputWindowMenu(MetisseWindow *win);
	//
private:
	void _replayMotion(void);
	//
private:
	void _snapForbidSwitch(void);
	void _interactiveMove(int ex, int ey);
	void _interactiveResize(int ex, int ey);
public:
	void wmMove(MetisseWindow *w, bool start); // do not use this one!
	void wmResize(MetisseWindow *w, bool start);
	void setResizeTransparency(float alpha);
	bool waitingForAConfigureWindow(void);
	bool inInteractiveMove(void);
	bool inInteractiveResize(void);
	bool inInternalMove(MetisseWindow *w);
	void setTmpRestackMode(restack_mode_t trm);
	void tmpRestack(MetisseWindow *w, bool start, unsigned long op);
	void schedulePagerModeOff(void);
	void windowDestroyed(MetisseWindow *win);
	//
private:
	void _inSelectRegion(int x, int y);
	void _dragRotate(MetisseWindow *win, int x, int y, int type);
	void _dragRotate(MetisseWindow *win, int x, int y);
	void _stopDragRotate(bool cancel = false);
	void _internalMove(int x, int y,  int *mod_x, int *mod_y);
	void _stopInternalMove(bool cancel = false);
	void _interactiveScale(int x, int y);
	void _stopInteractiveScale(bool cancel = false);
	void _stopDropingRegion(int button);
	bool _clipIntercept(int type, int x, int y, unsigned long keysym);
	void _clipInterceptFinish();
	bool _clipGiveUpInteception(int x, int y);
	void _clipInterceptTkExpire(void);
	bool _clipInterceptMotion(int x, int y);
	bool _clipInterceptRelease(int x, int y);
	void _clipAskClipboardMenu(bool isvv);
	void _clipCtrlReleased(void);
	void _clipFastVVGiveUp(void);
public:
	void setRegionSelectionSnapDist(int val);
	void selectRegion(
		MetisseWindow *win, int x, int y, bool add, bool snap = true);
	void fold(MetisseWindow *win, int x, int y);
	void foldByCmd(
		MetisseWindow *win, int x, int y, float alpha, float reduction);
	void foldNonInteractive(
		MetisseWindow *win, int x, int y, float alpha = 0.9,
		bool anime = true);
	void startDragRotate(MetisseWindow *win, int type, bool tmp);
	void startInternalMove(
		MetisseWindow *win, int x, int y, float alpha, bool tmp);
	void startInteractiveScale(
		MetisseWindow *win, int x, int y, float alpha, bool tmp);
	void clipPasteClipboard(void);
	void fvwmTraceRunning(bool run);
	void setRightLongPressTimeOut(unsigned to);
private:
	void _setupRightLongPressTimeOut(void);
	bool _motionInterceptPress(int x, int y, int button);
	void _motionInterceptFinish(void);
	bool _motionInterceptRelease(int x, int y);
	void _motionInterceptCancel(int x, int y);
	void _motionInterceptAction(int x, int y, int what);
	bool _motionInterceptMotion(int x, int y);
	void _motionInterceptTkExpire(void);
	//
public:
	void popupScrollLeafingWidget(sgNode *parent);
	//
public:
	void dragAndDropStart(void);
	void dragAndDropStop(void);
private:
	void _setModifierMask(unsigned long keysym, bool press);
	void _dragAndDropMotion(int x, int y);
	void _exposeModeHandleEvents(void);
	void _handleOneEvent(glWindow::event &e);
protected:
	void react(Observable *obs);
public:
	void draw(bool force);
	//
public:
  AScreen(MetisseDesktop *metisseDesktop, bool ortho,
		bool noStencil, bool portrait, bool force_fullscreen);
	~AScreen(void) ;

};

#endif
