/*
 *
 * desktop/MetisseWindow.H --
 *
 * Copyright (C) Nicolas Roussel
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */


#ifndef _MetisseWindow_H
#define _MetisseWindow_H


#include <string>
#include <map>

#include <nucleo/gl/scenegraph/sgNode.H>

#include "texture/glTiledTexturedImage.H"
#include "renderer/WindowRenderer.H"
#include "MetisseSource.H"
#include "fvwmmodule/AFvwm.H"
#ifdef HAVE_SPI
#include "main/WidgetManager.H"
#endif

class FacadeDescription;
class FacadeRealisation;
class AScreen;

struct cut
{
	int sx,sy,sw,sh,dx,dy ;
	cut(int a, int b, int c, int d, int e, int f) :
		sx(a), sy(b), sw(c), sh(d), dx(e), dy(f) {}
	cut(void) { cut(0,0,0,0,0,0); }
      
};

typedef std::list<cut > cutlist;
typedef std::map<sgNode*,cutlist> FacadeMapping;

class FacadeMatchStatus
{
public:
	bool to_be_removed;
	bool in_menu;
	bool over_win;
	int hideCount;
	bool realized;
	FacadeMatchStatus(void)
	{
		to_be_removed = false;
		in_menu = false;
		over_win = false;
		hideCount = 0;
		realized = false;
	};
};

typedef std::map<FacadeDescription *, FacadeMatchStatus *> FacadeMatchingMap;

class MetisseWindow : public sgNode {

 private:

	std::string _title ;
	MetisseSource *_metisseSource ;
	AScreen *_ascr;

	bool _resetTexture;
	float _rwidth;
	float _rheight;
	float _trx;
	float _try;
	float _tx;
	float _ty;
	float _rrx;
	float _rry;

	glTiledTexturedImage *_texture;

	int _texCoordsSize ;
	float *_texCoords ;

	// X11 geometry
	float _real_x;
	float _real_y;
	float _width;
	float _height;

	int _stackingPos;

	// additional translation vs the X11 translation
	float _interTransX;
	float _interTransY;
	float _attachedTransX;
	float _attachedTransY;

	unsigned long _frameID;
	unsigned long _clientID;

	bool _mapped;
	bool _neverMapped;

	float _red, _green, _blue, _alpha ;
	float _alpha_saved;

	WindowRenderer* _renderer;
	
	bool _unmanaged;
	bool _isRootWindow;
	bool _isEwmhDesktop;
	bool _isToolTips;

	int _desk;

	sgNode *_transientFor;
  	sgNode *_unmanagedFor;
	sgNode *_transientForOrig;
  	sgNode *_unmanagedForOrig;
	cut *_cutOrig;

	sgNode *_duplicateFor;

	sgNode *_pseudoSwallowNode;
	sgNode *_pseudoSwallowedBy;

	bool _isAToolGlass;

	FacadeMapping _facade;
	bool _isAFacadeWindow;
	bool _facadeChanged;
	int _facadeHideCount;

	FacadeMatchingMap _matchingFacades;
	FacadeMatchingMap _matchingFacadesOver;
	int _numOfMatchingFacades;

	// for pure facade only
	FacadeDescription *_facadeDesc;
	FacadeRealisation *_facadeReal;

#ifdef HAVE_SPI
	Accessible *_accessible;
	typedef std::list<AWidgetValuator*> vallist;
	vallist _widgetValuators;
	
	
private:
	AWidgetValuator *_findMatchingValuator(AWidgetValuator *val);
public:
#endif
	int _sb_relaction;
	bool _sb_cross;
	long _sb_x, _sb_y, _sb_w, _sb_h; 
	unsigned long _prev_button_mask, _start_button_mask;
	int _prev_x, _start_x;
	int _prev_y, _start_y;
	bool _some_action_done;

	// From fvwm
	window_flags *_flags;
	int _titleHeight;
	int _borderWidth;

	int _decorLeft;
	int _decorTop;
	int _decorRight;
	int _decorBottom;
	unsigned long int _fclient_width, _fclient_height;

	char *_windowName;
	char *_resClass;
	char *_resName;

	bool _transientForCorrectionDirty;
	float _desktopWidth;
	float _desktopHeight;
	float _eyeDist;

	float _rootTransY;
	float _rootTransX;

	double _surfaceRotationY;
	double _surfaceRotationX;
	double _surfaceRotationZ;
	double _surfaceScale;

	float _surfaceScaleSaveX;
	float _surfaceScaleSaveY;

	bool _surfaceRotationAllowed;
	void _surfaceCorrection(float tx, float ty);
	void _resetSurfaceCorrection(void);
	void _translate_rel2(float x, float y, float z);
	
	float _absoluteScaleX;
	float _absoluteScaleY;
	float _absoluteScaleZ;

	
	float _absoluteRotateAngle;
	float _absoluteRotateX;
	float _absoluteRotateY;
	float _absoluteRotateZ;

	void _applyTransientTransform(void);
	void _attachTransient(void);

	float _forceX;
	float _forceY;
	bool _currentTranslationForced;
	bool _inMovePage;

	void _transientForCorrection(void);

 public:

	MetisseWindow(
		std::string title, unsigned long id,
		MetisseSource *metisseSource, AScreen *ascr,
		int x, int y, unsigned int width, unsigned int height);
	~MetisseWindow();

	static float rootTransY;
	static float rootTransX;

	std::string getTitle(void);
	
	void setRenderer(WindowRenderer *renderer);
	WindowRenderer *getRenderer(void);
	
	void setSize(float width, float height);
	void getCurrentSize(float *width, float *height);
	void getSize(int *width, int *height);
	void getSize(float *width, float *height);
	void getSize(double *width, double *height);
	void getSize(unsigned int *width, unsigned int *height);
	void getClientSize(int *width, int *height);
	void setRealPosition(float x, float y, bool saveScalePos = false);
	void getCurrentRealPosition(float *x, float *y);
	void getRealPosition(float *x, float *y);
	void getRealPosition(int *x, int *y);
	int getStackingPos(void);
	void setStackingPos(int s);
	bool inMovePage(void);
	void setInMovePage(bool v);
	void getCurrentTranslation(float *tx, float *ty);
	void forceCurrentTranslation(float tx, float ty);
	void unforceCurrentTranslation(void);
	void setMapped(bool mapped);
	bool isMapped(void);
	bool neverMapped(void);
	void setUnmanaged(bool u);
	void setUnmanaged(void);
	bool isUnmanaged(void);
	void setRootWindow(void);
	bool isRootWindow(void);
	void setEwmhDesktop(void);
	bool isEwmhDesktop(void);
	void setToolTips(void);
	bool isToolTips(void);
	void pseudoSwallow(sgNode *n);
	sgNode *getPseudoSwallowedNode(void);
	void setPseudoSwallowedBy(sgNode *n);
	sgNode *getPseudoSwallowedBy(void);
	void setTransientFor(sgNode *n1, sgNode *n2 = 0, cut *c = 0);
	void setUnmanagedFor(sgNode *n1, sgNode *n2 = 0, cut *c = 0);
	void setDuplicateFor(sgNode *n);
	sgNode *getTransientFor(void);
  	sgNode *getUnmanagedFor(void);
	sgNode *getTransientForOrig(void);
  	sgNode *getUnmanagedForOrig(void);
	cut *getCutOrig(void);
	bool isSticky(void);
	void setToolGlass(bool v);
	void toggleToolGlass();
	bool isAToolGlass();

	sgNode *getDuplicateFor(void);

	bool setFacade(FacadeMapping wcm, bool add);
	FacadeMapping getFacade(void);
	void rmFacade(sgNode *n);
	void rmFacade(sgNode *n, cut *c);
	bool isOneOfOurFacade(sgNode *n);
	bool hasAFacade(void);
	Bool isAFacadeWindow(void);
	Bool setIsAFacadeWindow(void);

	bool addMatchingFacade(FacadeDescription *fd);
	bool removeMatchingFacade(FacadeDescription *fd);
	void setFacadeRealisation(FacadeDescription *fd, FacadeRealisation *fr);
	FacadeDescription *deleteRealisedDescription(void);
	//
	bool addMatchingFacadeOver(FacadeDescription *fd);
	void setFacadeOverWin(FacadeDescription *fd);
	void setOverFacadeRealisation(FacadeDescription *fd);
	FacadeDescription *deleteOverRealisedDescription(void);
	//
	bool removeMatchingFacadeOver(FacadeDescription *fd);
	bool deleteFacadeDescription(FacadeDescription *fd);
	void hideAsFacadeSource(bool on);
	void checkFacadeMenu(void);

#ifdef HAVE_SPI
	Accessible *getTopAccessible(void);
	void setWidgetValuator(AWidgetValuator *sb);
#endif

	void setWindowName(char *name);
	void setResClass(char *name);
	void setResName(char *name);
	char *getWindowName(void);
	char *getResClass(void);
	char *getResName(void);
	bool isIconified(void);
	void setIconified(bool v);
	bool isManaged(void);
	int getDesk(void);
	void setFvwmParameters(
		window_flags *flags, int th, int bw, unsigned long frameID,
		unsigned long int frame_w, unsigned long int frame_h,
		int desk, bool instartup);
	int getBorderWidth(void);
	void getDecorStrut(int *left, int *top, int *right, int *bottom);
	int getMaxDecorStrut(void);
private:
	bool _buildFacadeMenu(void);
public:
	void buildSpecialMenu(void);
	unsigned long getFrameID(void);
	unsigned long getClientID(void);

	void setBgcolor(float red, float green, float blue, float alpha);
	void setAlpha(float alpha);
	void setTmpAlpha(float alpha);
	void resetAlpha(void);
	void getBgcolor(float *red, float *green, float *blue, float *alpha);
	float getAlpha(void);
	bool doPseudoSelection(void);

	void resetTexture(void);
	bool inTextureReset(void);
	void textureResetSetRealPosition(float x, float y);
	void resetTexture(float width, float height);
	void deleteTexture(void);
	bool doDraw(void);

	
	void myUpdateTexture(
		Image *img, int x, int y, unsigned int w, unsigned int h);
	void updateTexture(
		Image *img, int x, int y, unsigned int w, unsigned int h,
		bool redirected = false);
	void facadeUpdateTexture(
		sgNode *winc, Image *img, int x, int y, unsigned int w,
		unsigned int h);

	void saveSurfaceScaleRealPosition(void);
	bool getSurfaceScaleSavedRealPosition(float *x, float *y);

	void surfaceCorrection(void);

	void surfaceRotation(bool cancel);
	void preDisplay(
		float desktopWidth, float desktopHeight, float eyeDist,
		float depth);
	void restoreTranslation(void);
	void mapCorrection(void);
	void unmapCorrection(void);
	void internalTranslation(float x, float y, float z);
	void getInternalTranslation(float *x, float *y);
	void translate_rel(float x, float y, float z);
	void absoluteScale(float x, float y, float z);
	void getAbsoluteScale(float *x, float *y, float *z);
	void absoluteRotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void resetAbsoluteScale(void);

	void getTextureMapping(
		glTiledTexturedImage **texture, int *texCoordsSize,
		float **texCoords);

	void setTextureCoordinates(
		float left, float top, float right, float bottom);

	void  getRendererTransformation(
		GLfloat *trans, bool ignoreInterTrans = false);
	void  getRendererFullTransformation(GLfloat *trans);
	void  setRendererTransformation(
		GLfloat *trans, bool ignoreInterTrans = false);
	void  setRendererFullTransformation(GLfloat *trans);

	void sendMetisseTransform(bool mark);

	std::list<WinRegions::region > getSelectedRegions(void);

	bool getCrossPosition(double *x, double *y);
	sgNode *getFacadeAtPoint(int x, int y, cut *fcut);
	bool isOverDecor(int x, int y);
	void keyEvent(unsigned long key, bool down_flag);
	void pointerEvent(float x, float y, unsigned long button_mask);

};

#endif
