#ifndef WindowRenderer_H
#define WindowRenderer_H

/*
 *
 * renderer/WindowRenderer.H --
 *
 * Copyright (C) Olivier Chapuis
 * Copyright (C) Nicolas Roussel
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <nucleo/gl/scenegraph/sgNode.H>
#include <nucleo/core/ReactiveObject.H>
#include <nucleo/core/TimeKeeper.H>

#include <X11/Xmd.h>
// #include <X11/Xutil.h>

#include "WinRegions.H"
#include "FoldablePolygon.H"

#include <nucleo/gl/shading/glShader.H>
#include <string>
#include <map>

using namespace nucleo;

class LayerManager;
class MetisseWindow;
class FvwmModule;
class AScreen;

#define LENS_TYPE_NONE       0
#define LENS_TYPE_VERT       1
#define LENS_TYPE_HORIZ      2
#define LENS_TYPE_CROSS      3
#define LENS_TYPE_CENTER     4
#define LENS_TYPE_BORDER     5

#define SHADER_RENDERER_ANIMATE  (1<<0)
#define SHADER_RENDERER_PERIODIC (1<<1)
#define SHADER_RENDERER_TMP      (1<<2)

class ShaderRendererOpt
{
public:
	unsigned int nbr_anim_step;
	unsigned int anim_total_time; // in ms
	unsigned int life_time; // in ms
	unsigned int stale_time;
	unsigned long type;

	ShaderRendererOpt(void)
	{
		nbr_anim_step = 0;
		anim_total_time = 0;
		life_time = 0;
		type = 0;
	};
};

class ShaderRenderer
{
public:
	glShader *shader;
	ShaderRendererOpt *opt;

	unsigned int current_step;
	bool back;

	ShaderRenderer(glShader *s, ShaderRendererOpt *sro)
	{
		shader = s;
		opt = sro;
	};
	~ShaderRenderer()
	{
		delete opt;
	};
};

class WindowRenderer : public sgNode, public ReactiveObject
{

private:

	AScreen *_ascr;
	MetisseWindow  *_window;

	bool _shadow;

	int _nShapeRects;
	CARD16 *_shape;
	bool _canUseStencilBuffer;
  
	CARD16 *_cutShape;
	int _nCutShapeRects;

	WinRegions *_winRegions;
	bool _selectingRegion;

	std::list<WinRegions::xyregion > _regionSet;

	bool _markingFeedback;
	int _mf_dirs,_mf_dist,_mf_draw_dist;
	float _mf_x,_mf_y;

	
	FoldablePolygon _polygon;
	std::list<glTiledTexturedImage::point>  _img_points;
	//std::list<glTiledTexturedImage::point>  _shadow_points;
	
	std::list<glTiledTexturedImage::point>  _lines_points;

	bool _cropToCircle;
	bool _testPolygon;
	TimeKeeper *_animTk; /* for animation */
	TimeKeeper *_foldTk; /* for folding back */
	TimeKeeper *_geneTk; /* generic timer */

	LayerManager *_root;
	FvwmModule *_fvwmModule;
	
	float _foldAlpha;
	int _foldAnimStep;
	double _foldReduction;

	bool _folding, _foldingBack, _foldingOn, _scaleBack, _transBack;
	double _x_from, _y_from, _x_to, _y_to, _x_to_curr, _y_to_curr;
	float _scaleBackStep;
	int _scaleBackGravity;
	GLfloat _transBackSteps[18];
	GLfloat _transBackTrans[18];
	GLfloat _transMark[18];
	GLfloat _beforeIconTrans[18];

	int _backStep;
	bool _inResetOrSet;
	bool _inIconification;
	bool _iconify;
	bool _scheduleHide;
	bool _doFixCursor;

	bool _do_x_snapping, _do_y_snapping;
	bool _snap_right, _snap_top;
	GLfloat _snap_xoff, _snap_yoff;

	GLfloat _lens_ax, _lens_ay, _lens_x, _lens_y, _lens_w, _lens_h,
		_lens_sx, _lens_sy;
	GLfloat _blens_ax, _blens_ay, _blens_x, _blens_y, _blens_w, _blens_h,
		_blens_sx, _blens_sy;
	bool _do_lens,_do_blens;
	bool _in_pm_lens;

	int _auto_lens_type;
	GLfloat _lens_vert_w, _lens_horiz_h, _lens_wh_w, _lens_wh_h;
	int _menu_lens;

	glTiledTexturedImage::tiledTessellation _tiled_tessellation;
	bool _useTessCache;

	bool _mp_in;
	int _mp_wix, _mp_wiy, _mp_wiw, _mp_wih;
	GLfloat _mp_mx, _mp_my;
	int _csc_ix, _csc_iy;
	GLfloat _mp_scale_x, _mp_scale_y; 
	bool _mp_warp;

	void _shapeDisplay(bool on);
	void _reset(void);
	void pseudoSelect();

protected:
	void react(Observable *obs);
	void select(void);
	void display(dlPolicy policy);

public:
	static bool shadowByDefault;
	static float borderVisLens;

	MetisseWindow *getWindow(void);

	void shadow(void);

	void setShape(CARD16 *buf, int nrecs, bool cusb);
	bool hasShape(void);

	void setCutShape(std::list<WinRegions::region > reg);
	void addtoCutShape(std::list<WinRegions::region > reg);
	void addtoCutShape(GLdouble x, GLdouble y, GLdouble w, GLdouble h);
	bool hasCutShape(void);

	bool isInsideWindow(float x, float y);
	//bool isInsideWindow2(int x, int y);

	void selectRegion(GLdouble x, GLdouble y, bool stop);
	void selectRegion(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
	std::list<WinRegions::region > getSelectedRegions(void);
	bool hasSelectedRegions(void);
	WinRegions::region *getSelectRegionFromXY(GLdouble x, GLdouble y);
	void rmSelectedRegions();
	void rmMatchingSelectedRegions(WinRegions::region *reg);

	void setRegionsSet(std::list<WinRegions::xyregion > reg);
	void clearRegionsSet(void);
	int getRegionsSetSize(void);

	void setMarkingFeedback(
		bool on, int x, int y, int dist, int draw_dist, int dirs);
  
	void cropToCircule(void);
	void testPolygon(void);
	void simpleReset(void);
	void fold(
		int px, int py, bool fold, bool anim = false,
		float alpha = 0.5, float reduction = -1.0f);
	void foldOff(void);
	void scaleBack(LayerManager *root, int gravity);
	void transformBack(float *trans);
	void animate(void);
	void resetOrSet(bool fixCursor);
	void reset(
		bool store = false, bool animate = false, 
		bool fixCursor = false);
	void iconify(
		LayerManager *root, FvwmModule *fvwmModule, bool on,
		float icon_x, float icon_y, float icon_w, float icon_h,
		float frame_x, float frame_y, float frame_w,float frame_h);
	void snap(
		bool do_x, MetisseWindow *win_x, int snap_x, bool right,
		bool do_y, MetisseWindow *win_y, int snap_y, bool top);
	void unsnap(void);
	void unlens(void);
	void lens(
		GLfloat ax, GLfloat ay, GLfloat x, GLfloat y,
		GLfloat width, GLfloat height,
		GLfloat scale_x, GLfloat scale_y);
	void borderLens(
		GLfloat xax, GLfloat xay, GLfloat xx, GLfloat xy,
		GLfloat xwidth, GLfloat xheight,
		GLfloat xscale_x, GLfloat xscale_y,
		GLfloat yax, GLfloat yay, GLfloat yx, GLfloat yy,
		GLfloat ywidth, GLfloat yheight,
		GLfloat yscale_x, GLfloat yscale_y);
	void lensSnapAndGo(
		bool do_x, GLfloat offx, bool right, bool left_to_right, 
		bool do_y, GLfloat offy, bool top, bool bottom_to_top);
	void lensRegions(float sx, float sy);
	void setAutoLensType(int lt);

	
private:
	bool _checkCurrentMotorPointing(float x, float y);
	bool _getNewMotorPointingWidget(float x, float y);
	void _doMotorPointing(float x, float y, float *rx, float *ry);
public:
	void keyEvent(unsigned long key, bool down_flag=true);
	void pointerLeave(void);
	void pointerEvent(float x, float y, unsigned long button_mask=0);

	// shaders
protected:
	std::list<ShaderRenderer *>_shaders;
	TimeKeeper *_shaderTk;
private:
	ShaderRenderer *_getCurrentShaderRenderer(void);
public:
	bool removeShader(glShader *shader);
	void removeAllShaders(void);
	void addShader(
		glShader *shader, ShaderRendererOpt *sro, bool back);
	void replaceShader(glShader *rmshader, glShader *addshader);
	void animShader(bool back);

	// constructor
public:
	WindowRenderer(AScreen *ascr, MetisseWindow  *window);
	~WindowRenderer(void);
};
#endif
