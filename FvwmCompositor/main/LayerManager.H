/*
 *
 * main/LayerManaer.H --
 *
 * Copyright (C) Nicolas Roussel
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef LAYERMANAGER_H
#define LAYERMANAGER_H

#include <vector>
#include <nucleo/core/ReactiveObject.H>
#include <nucleo/core/TimeKeeper.H>
#include <nucleo/gl/scenegraph/sgNode.H>
#include <nucleo/gl/scenegraph/sgViewpoint.H>

#include "main/AUtils.H"

class AScreen;
class MetisseWindow;

#define SURFACE_SPHERE            0
#define SURFACE_TBCYLINDER        1
#define SURFACE_LRCYLINDER        2
#define SURFACE_SPHERE_CYLINDER   3
#define SURFACE_CYLINDER_SPHERE   4
#define SURFACE_TABLETOP          5
#define SURFACE_SCALE             6
#define SURFACE_LAST            SURFACE_SCALE
#define SURFACE_FIRST           SURFACE_SPHERE

#define SURFACE_DEFAULT SURFACE_SPHERE

#define SURFACE_DEFAULT_CUT 0.5

#define SNAP_ATTRAC_MODE_NONE        0
#define SNAP_ATTRAC_MODE_CLASSICAL   1
#define SNAP_ATTRAC_MODE_SG          2
#define SNAP_ATTRAC_MODE_VISSG       3
#define SNAP_ATTRAC_MODE_METISSE     4

#define DEFAULT_SNAP_ATTRAC_MODE SNAP_ATTRAC_MODE_NONE
#define DEFAULT_SNAP_ATTRACTION_DIST 10

using namespace nucleo ;

class LayerManager : public sgNode, public ReactiveObject {

 private:
	sgNode *_bg;
	sgNode *_rootWin;
	sgViewpoint *_viewpoint;

	AScreen *_ascr;

	float _depth;

	float _desktopWidth;
	float _desktopHeight;
	float _eyeDist;

	double _vScale;
	double _extScaleX;
	double _extScaleY;
	double _extZRotate;
	double _totalScaleX;
	double _totalScaleY;

	// virtual desktop
	float _pagerVWidth, _pagerVHeight;  // size of the virtual desktop
	float _pagerVxPages, _pagerVyPages; // number of cols and rows of pages
	float _pagerVx, _pagerVy;           // position of the root window in px
	long _pagerCurrentDesk;

	float _transX;
	float _transY;
	float _scaleX;
	float _scaleY;

	float _surfaceTop;
	float _surfaceLeft;
	float _surfaceBottom;
	float _surfaceRight;
	float _surfaceX;
	float _surfaceY;
	int _surface;
	bool _surfaceOn;

	class Slot {
	public:
		Slot(float x, float y, float width, float height, MetisseWindow *inWin = NULL) :
			rect(x, y, width, height),
			center((x+width)/2.0, (y+height)/2.0),
			orig(x, y),
			temp_rect(x, y, width, height),
			temp_center((x+width)/2.0, (y+height)/2.0),
			win(inWin) {}

		Rect  rect;
		Point center;

		Point orig;

		Rect  temp_rect;
		Point temp_center;

		MetisseWindow *win;
	};

	bool _inExposeMode;
	bool _exposeModeCanceled;
	TimeKeeper *_exposeModeTk;
	int _exposeModeStep;
	std::vector<Slot> _exposeModeList;

	bool _inPagerMode;
	bool _schedulePagerModeOff;
	TimeKeeper *_pagerModeTk;
	int _pagerModeStep;
	float _pagerModeSX;
	float _pagerModeSY;
	float _pagerModeStepX;
	float _pagerModeStepY;
	float _pagerModePrevSX;
	float _pagerModePrevSY;

	TimeKeeper *_saveFrameTk;

	// snap attraction
	int _snapAttractionDist;
	int _snapAttracMode;

	void _printStacking(void);
	unsigned int _guessGravity(MetisseWindow *win);
	bool _overlap(
		MetisseWindow *win1,
		GLdouble bbleft, GLdouble bbtop, GLdouble bbright, GLdouble bbbottom,
		MetisseWindow *win2,
		GLdouble *rleft, GLdouble *rtop, GLdouble *rright, GLdouble *rbottom,
		GLdouble *rx1, GLdouble *ry1, GLdouble *rx2, GLdouble *ry2,
		GLdouble *rx3, GLdouble *ry3, GLdouble *rx4, GLdouble *ry4,
		float *top, float *right, float *bottom, float *left);

 protected:
	void display(dlPolicy policy) ;
	void select(void) ;
	void react(Observable *obs);

 public:
	LayerManager(AScreen *_ascr, std::string name) ;

	void saveFrame(unsigned int delay);
	void preDisplay(void);
	void setBg(sgNode *bg);
	void setRootWin(sgNode *rootWin);
	MetisseWindow *getRootWindow();
	void setDepth(float depth);
	float getDepth(float depth);
	void restack(sgNode *node, sgNode *nodeNext, bool add);
	MetisseWindow *getUnmanagedWindow(int x, int y);
	MetisseWindow *getUnmanagedWindow(void);
	void setDesktopProperties(
		float desktopWidth, float desktopHeight, double eyeDist,
		double near, double far, sgViewpoint *viewpoint, double vscale,
		double extScaleX, double extScaleY, double extZRotate);
	void translate_rel(float x, float y, float z);
	void checkPaneAction(int x, int y, bool inInternalMove = false);
	bool checkMoveWindowPaneAction(
		int x, int y, int *r_npx, int *r_npy, int *r_wx, int *r_wy,
		int *r_tx, int *r_ty);
	void setSnapAttractDist(void);
	void setSnapAttractDist(int dist);
	int getSnapAttractDist(void);
	void setSnapAttractMode(void);
	void setSnapAttractMode(int mode);
	int  getSnapAttractMode(void);
	bool doSnapAttract(
		MetisseWindow *win, int *px, int *py, bool *do_x, bool *do_y,
		bool *right, bool *top, MetisseWindow **win_x, MetisseWindow **win_y);
	bool checkMove(MetisseWindow *win, int x, int y);
	void zoomedout_translate_rel(float x, float y, float z);
	void getTranslation(float *x, float *y);

	// input and output are X11 geometries
	void x11project(int x, int y, int *rx, int *ry);
	void x11projectWarp(
		int x, int y, int *px, int *py, bool warp = true);

	// 
	bool getWinBoundingBox(
		MetisseWindow *win, GLdouble *ret_left, GLdouble *ret_top,
		GLdouble *ret_right, GLdouble *ret_bottom, bool x11 = false);

	// translate the desktop (if scaled) so that win is visible
	void translateForWindow(MetisseWindow *win);

	void scaleWindow(
		MetisseWindow *win, GLfloat fx, GLfloat fy, GLfloat fz,
		unsigned int gravity = 0, bool absolute = false,
		bool animate = false, bool fixCursor = false);
	void rotateWindow(
		MetisseWindow *win, GLfloat angle, GLfloat fx, GLfloat fy,
		GLfloat fz, unsigned int gravity = 0, bool absolute = false);
	void attachWindow(MetisseWindow *main, MetisseWindow *sub);

	void scale(float x, float y, float z);
	void getScale(float *x, float *y);
	void resetTransformations(void);

	// pager mode stuff
	bool inMovePage(void);
	void _setInMovePage(void);
	void setVirtualDesktopPara(
		int pagerVx, int pagerVy, long currentDesk, int pagerVxPages,
		int pagerVyPages);
	bool getPointerEvent(
		int x, int y, unsigned char button_mask, int *rx, int *ry,
		char *cmd);
	void getPagerModeScale(float *x, float *y);
	void pagerMode(bool on);
	void togglePagerMode(void);

	//
	void foldOverWindows(
		MetisseWindow *startWindow, bool set, float alpha = 0.9,
		bool anim = true);
	void scaleOverWindows(MetisseWindow *startWindow, bool set);
	void setTmpAlphaAbove(
		MetisseWindow *startWindow, bool set = true, float alpha = 0.5);

	void setSurfaceParameters(
		float surfaceTop, float surfaceLeft, float _surfaceBottom,
		float _surfaceRight);
	void setSurfaceParameters(float cutX, float cutY);
	void setSurfaceParameters(float cut);
	// surface by name
	void setSurface(int surface, float cutX, float cutY);
	void setSurface(int surface, float cut);
	void setSurface(int surface);
	void setSurface(float cutX, float cutY);
	void setSurface(float cut);
	//
	void enableSurface(int toggle);
	void getSurfaceRotation(
		float x, float y, float w, float h, double prevScale,
		double *rx, double *ry, double *rz, double *scale);

	//
private:
	bool _isOnCurrentPage(MetisseWindow *win);
	bool _doOverlap(MetisseWindow *win1, MetisseWindow *win2);
	//
public:
	std::list<MetisseWindow *> getWindowOnCurrentPage(void);
	std::list<std::list<MetisseWindow *> *> getWindowOnCurrentPageByStack(
		bool add_desktop = false);
	void exposeMode(bool cancel = false);
	bool inExposeMode(void);
} ;

#endif
