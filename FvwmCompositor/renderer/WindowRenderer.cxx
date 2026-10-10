/*
 *
 * renderer/WindowRenderer.cxx --
 *
 * Copyright (C) Nicolas Roussel
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config-not-xserver.h"
#endif

#include "WindowRenderer.H"

#include <cmath>
#include <stdexcept>

#include <nucleo/core/TimeKeeper.H>
#include <nucleo/gl/window/keysym.H>
#include <nucleo/utils/StringUtils.H>
#include <nucleo/core/URI.H>

#include "desktop/MetisseWindow.H"
#include "main/LayerManager.H"
#include "fvwmmodule/FvwmModule.H"
#include "main/AScreen.H"

#include "FoldablePolygon.H"
#include "goodies.H"

using namespace std;


static GLfloat Identity[18] =
{
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0,
	0.0, 0.0
};

bool WindowRenderer::shadowByDefault = true;

float WindowRenderer::borderVisLens = 1.0;

// ---------------------------------------------------------------------------
//

MetisseWindow * WindowRenderer::getWindow(void)
{
	return _window;
}

void WindowRenderer::shadow(void)
{
	_shadow = !_shadow;
	_changed = true;
}

// ---------------------------------------------------------------------------
//

void WindowRenderer::setShape(CARD16 *buf, int nrecs, bool cusb)
{
	if (_shape != NULL)
	{
		delete[] _shape;
		_shape = NULL;
	}

	_shape = buf;
	_nShapeRects = nrecs;
	_canUseStencilBuffer = cusb;
	postRedisplay();
}

void WindowRenderer::setCutShape(std::list<WinRegions::region > reg)
{
	if (_cutShape != NULL)
	{
		free(_cutShape);
		_cutShape = NULL;
	}

	_nCutShapeRects = reg.size();
	if (_nCutShapeRects > 0)
	{
		_cutShape = new CARD16[_nCutShapeRects*4];
		std::list<WinRegions::region >::iterator iter;
		int i = 0;
		for (iter=reg.begin(); iter!= reg.end(); ++iter)
		{
			_cutShape[i++] = (CARD16)(*iter).x;
			_cutShape[i++] = (CARD16)(*iter).y;
			_cutShape[i++] = (CARD16)(*iter).w;
			_cutShape[i++] = (CARD16)(*iter).h;
		}
	}
	getWindow()->buildSpecialMenu();
	postRedisplay();
}

void WindowRenderer::addtoCutShape(std::list<WinRegions::region > reg)
{
	std::list<WinRegions::region > newReg;
	std::list<WinRegions::region >::iterator iter;

	newReg.clear();

	for (int i = 0; i < _nCutShapeRects; i++)
	{
		newReg.push_front(WinRegions::region(
					  _cutShape[i++], _cutShape[i++],
					  _cutShape[i++], _cutShape[i++])); 
	}
	for (iter=reg.begin(); iter!= reg.end(); ++iter)
	{
		newReg.push_front((*iter));
	}
	getWindow()->buildSpecialMenu();
	setCutShape(newReg);
}

void WindowRenderer::addtoCutShape(GLdouble x, GLdouble y, GLdouble w, GLdouble h)
{
	std::list<WinRegions::region > newReg;
	newReg.clear();
	newReg.push_front(WinRegions::region(x,y,w,h));
	addtoCutShape(newReg);
}

bool WindowRenderer::hasCutShape(void)
{
	if (_nCutShapeRects)
	{
		return true;
	}
	return false;
}

bool WindowRenderer::hasShape(void)
{
	if (_nShapeRects || _nCutShapeRects)
	{
		return true;
	}
	return false;
}


bool WindowRenderer::isInsideWindow(float x, float y)
{
	float width,height,rx,ry;
	_window->getSize(&width, &height);
	_window->getRealPosition(&rx, &ry);

	float realx = x + width/2.0 + rx;
	float realy = -y + height/2.0 + ry;

	if (realx < rx || realx > rx + width || realy < ry ||
	    realy > ry + height)
	{
		return false;
	}

	if ((!_nShapeRects || !_shape) && (!_nCutShapeRects || !_cutShape))
	{
		return true;
	}

	bool shapeOk = (!_nShapeRects || !_shape)? true:false;

	int i;
	for (i = 0; i < _nShapeRects; i++)
	{
		float x = _shape[i*4] + rx;
		float y = _shape[i*4 + 1] + ry;
		float w = _shape[i*4 + 2];
		float h = _shape[i*4 + 3];

		if (realx >= x && realx <= x+w && realy >= y && realy <= y + h)
		{
			shapeOk = true;
			break;
		}
	}

	if (!shapeOk)
	{
		return false;
	}

	for (i = 0; i < _nCutShapeRects; i++)
	{
		float x = _cutShape[i*4] + rx;
		float y = _cutShape[i*4 + 1] + ry;
		float w = _cutShape[i*4 + 2];
		float h = _cutShape[i*4 + 3];

		if (realx >= x && realx <= x+w && realy >= y && realy <= y + h)
		{
			return false;
		}
	}

	return true;
}

#if 0
bool WindowRenderer::isInsideWindow2(int x, int y)
{
	float width,height,rx,ry;
	_window->getSize(&width, &height);
     
	if (x < 0 || x > width || y < 0 || y > height)
	{
		return false;
	}

	if ((!_nShapeRects || !_shape) && (!_nCutShapeRects || !_cutShape))
	{
		return true;
	}

	bool shapeOk = (!_nShapeRects || !_shape)? true:false;

	int i;
	for (i = 0; i < _nShapeRects; i++)
	{
		float xx = _shape[i*4];
		float yy = _shape[i*4 + 1];
		float w = _shape[i*4 + 2];
		float h = _shape[i*4 + 3];

		if (x >= xx && x <= xx+w && y >= yy && y <= yy + h)
		{
			shapeOk = true;
			break;
		}
	}

	if (!shapeOk)
	{
		return false;
	}

	for (i = 0; i < _nCutShapeRects; i++)
	{
		float xx = _cutShape[i*4];
		float yy = _cutShape[i*4 + 1];
		float w = _cutShape[i*4 + 2];
		float h = _cutShape[i*4 + 3];

		if (x >= xx && x <= xx+w && y >= yy && y <= yy + h)
		{
			return false;
		}
	}

	return true;
}
#endif

// --------------------------------------------------------------
// Region stuff

void WindowRenderer::selectRegion(GLdouble x, GLdouble y, bool notStop)
{
	if (!notStop)
	{
		_selectingRegion = false;
		_winRegions->storeCurrent();
		return;
	}
	GLdouble hw, hh;
	_window->getSize(&hw, &hh);
	hw= hw/2.0; hh= hh/2.0;
	if (x > hw)
	{
		x = hw;
	}
	if (x < -hw)
	{
		x = -hw;
	}
	if (y > hh)
	{
		y = hh;
	}
	if (y < -hh)
	{
		y = -hh;
	}
	if (!_selectingRegion)
	{
		_selectingRegion = true;
		_winRegions->updateCurrentXYRegion(x,y,x,y);
	}
	else
	{
		WinRegions::xyregion c = _winRegions->getCurrentXYRegion();
		_winRegions->updateCurrentXYRegion(c.x1,c.y1,x,y);
	}
	postRedisplay();
}

void WindowRenderer::selectRegion(
	GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
	GLdouble hw, hh;
	_window->getSize(&hw, &hh);
	hw= hw/2.0; hh= hh/2.0;
	if (x1 > hw)
	{
		x1 = hw;
	}
	if (x1 < -hw)
	{
		x1 = -hw;
	}
	if (y1 > hh)
	{
		y1 = hh;
	}
	if (y1 < -hh)
	{
		y1 = -hh;
	}
	if (x2 > hw)
	{
		x2 = hw;
	}
	if (x2 < -hw)
	{
		x2 = -hw;
	}
	if (y2 > hh)
	{
		y2 = hh;
	}
	if (y2 < -hh)
	{
		y2 = -hh;
	}

	_winRegions->updateCurrentXYRegion(x1,y1,x2,y2);
	postRedisplay();
}

std::list<WinRegions::region > WindowRenderer::getSelectedRegions(void)
{
	std::list<WinRegions::region > c = _winRegions->getNormalizedRegions();
	return c;
}

bool WindowRenderer::hasSelectedRegions(void)
{
	if (_winRegions->size() > 0)
	{
		return true;
	}
	return false;
}

void WindowRenderer::rmSelectedRegions()
{
	_winRegions->clear();
	postRedisplay();
}

void WindowRenderer::rmMatchingSelectedRegions(WinRegions::region *reg)
{
	if (_winRegions->size() <= 0)
	{
		return;
	}
	_winRegions->rmMatchingRegion(reg);
	postRedisplay();
}

WinRegions::region *WindowRenderer::getSelectRegionFromXY(GLdouble x, GLdouble y)
{
	std::list<WinRegions::region > c = _winRegions->getNormalizedRegions();
	WinRegions::region *fr = 0;

	std::list<WinRegions::region >::iterator iter;
	
	for (iter = c.begin(); iter != c.end(); iter++)
	{
		if ((*iter).x <= x && x <= (*iter).x + (*iter).w &&
		    (*iter).y >= y && y >= (*iter).y - (*iter).h)
		{
			fr = &(*iter);
			return fr;
		}
	}
	return fr;
}

// ----------------------------------------------------------------------
//

void WindowRenderer::setRegionsSet(std::list<WinRegions::xyregion > reg)
{
	_regionSet.clear();
	std::list<WinRegions::xyregion >::iterator it;
	for (it = reg.begin(); it != reg.end(); it++)
	{
		_regionSet.push_front(
			WinRegions::xyregion(
				(*it).x1,(*it).y1,(*it).x2,(*it).y2));
	}
	postRedisplay();
}

void WindowRenderer::clearRegionsSet(void)
{
	_regionSet.clear();
	postRedisplay();
}

int WindowRenderer::getRegionsSetSize(void)
{
	return _regionSet.size();
}

// ----------------------------------------------------------------------
//

void WindowRenderer::setMarkingFeedback(
	bool on, int x, int y, int dist, int draw_dist, int dirs)
{
	_markingFeedback = on;
	_mf_dist = dist;
	_mf_draw_dist = draw_dist;
	_mf_dirs = dirs;

	// root coor to window coor
	float width,height,rx,ry;
	_window->getSize(&width, &height);
	_window->getRealPosition(&rx, &ry);

	_mf_x = (x-rx) - width/2.0;
	_mf_y = -(y-ry) + height/2.0;

	postRedisplay();
}

// ----------------------------------------------------------------------
//
#define TRIANGLE 1

void WindowRenderer::_reset(void)
{
	double hw,hh;
	_window->getSize(&hw,&hh);

	hw = hw/2.0;
	hh = hh/2.0;

	_useTessCache = false;
	_tiled_tessellation.clear();

	_polygon.points.clear();
	_img_points.clear();
	_lines_points.clear();

	//_shadow_points.clears();
	_shadow = WindowRenderer::shadowByDefault;

	if (_window->getPseudoSwallowedNode() != 0)
	{
		MetisseWindow *win =
			dynamic_cast<MetisseWindow *>
			(_window->getPseudoSwallowedNode());

		// intersection
		float sw_w, sw_h, sw_hw, sw_hh, sw_rx, sw_ry, sw_x, sw_y;
		win->getRealPosition(&sw_rx, &sw_ry);
		win->getSize(&sw_hw,&sw_hh);
		sw_hw = sw_hw/2.0;
		sw_hh = sw_hh/2.0;

		float w_rx,w_ry;
		_window->getRealPosition(&w_rx, &w_ry);

		#if 0
		std::cerr << "PS: " << w_rx << " " << w_ry << " "
			  << w_rx+2*hw << " " <<  w_ry+2*hh << " / "
			  << sw_rx << " " << sw_ry << " "
			  << sw_rx+2*sw_hw << " " <<  sw_ry+2*sw_hh
			  <<  std::endl;
		#endif
		if (sw_rx + 2*sw_hw > w_rx && sw_rx < w_rx + 2*hw &&
		    sw_ry + 2*sw_hh > w_ry && sw_ry < w_ry + 2*hh)
		{
			// windows intersect

			if (win->getTransientFor() == _window)
			{
				GLfloat ix,iy,iw,ih;
				ix = fmax(sw_rx,w_rx);
				iy = fmax(sw_ry,w_ry);
				iw = fmin(sw_rx + 2*sw_hw,w_rx + 2*hw) -
					fmax(sw_rx,w_rx);
				ih = fmin(sw_ry + 2*sw_hh,w_ry + 2*hh) -
					fmax(sw_ry,w_ry);

				// lens from the intersection
				GLfloat lax, laxx, layy, lay, lx, ly,
					lw, lh, lscale_x, lscale_y;
				int hsize = 1;
				lx = (ix+(iw/2)) - hsize - hw;
				ly = hh - (iy+(ih/2) - hsize);
				lw = 2*hsize;
				lh = 2*hsize;
				lscale_x = iw/2*hsize;
				lscale_y = ih/2*hsize;
				lax = lx + lw/2.0 - (lw*lscale_x/2.0);
				lay = ly + lh/2.0 - (lh*lscale_y/2.0);

				setup_lens_deformation(
					LENS_TYPE_CENTER, 161, false, hw, hh,
					lax, lay, lx, ly, lw, lh, lscale_x,
					lscale_y,
					0, 0, 0, 0, 0, 0, 0, 0,
					&_polygon.points, &_img_points,
					&_lines_points);
			}
			else
			{
				#if 1
				if (sw_hw <= sw_hh)
				{
					setup_swallowed_deformation(
						_window, win,
						STRANS_LEFT_CUT_RIGHT,
						&_polygon.points, &_img_points,
						&_lines_points);
				}
				else
				{
					setup_swallowed_deformation(
						_window, win,
						STRANS_TOP_CUT_BOTTOM,
						&_polygon.points, &_img_points,
						&_lines_points);
				}
				#elif 0  // up scale(left,right) down
				// merdique
				setup_swallowed_deformation(
					_window, win, STRANS_TOP_COMPLR_BOTTOM,
					&_polygon.points, &_img_points,
					&_lines_points);
				#elif 0
				// bof
				setup_swallowed_deformation(
					_window, win, STRANS_SIMPLE_MULTIPLE,
					&_polygon.points, &_img_points,
					&_lines_points);
				#elif 1
				setup_swallowed_deformation(
					_window, win, STRANS_TEST,
					&_polygon.points, &_img_points,
					&_lines_points);
				// an other one ?
				#endif
			}
			_useTessCache = false;
		}
		else
		{
		}
		

	}

	if (_do_lens && !(_do_x_snapping || _do_y_snapping))
	{
		_shadow = false;
		setup_lens_deformation(
			_auto_lens_type, 21, _do_blens, hw, hh,
			_lens_ax, _lens_ay, _lens_x, _lens_y,
			_lens_w, _lens_h, _lens_sx, _lens_sy,
			_blens_ax, _blens_ay, _blens_x, _blens_y,
			_blens_w, _blens_h, _blens_sx, _blens_sy,
			&_polygon.points, &_img_points, &_lines_points);

	}
	else if (_do_x_snapping || _do_y_snapping)
	{
		_shadow = false;
		setup_snapping_deformation(
			_do_x_snapping, _do_y_snapping,
			_snap_top, _snap_right, _snap_xoff, _snap_yoff,
			_window->getMaxDecorStrut(),
			_ascr->root->getSnapAttractDist(),
			hw, hh, &_polygon.points, &_img_points, &_lines_points);
	}
	else if (_cropToCircle)
	{
		_shadow = false;
		setup_circle_deformation(
			hw, hh, &_polygon.points, &_img_points, &_lines_points);
	}
	else if (_testPolygon)
	{
		GLdouble ch = 2*hh/3;
		GLdouble cw = 2*hw/3;
		_polygon.points.push_back(glTiledTexturedImage::point(-hw,-hh));
		_polygon.points.push_back(glTiledTexturedImage::point(0,-ch));
		_polygon.points.push_back(glTiledTexturedImage::point(hw,-hh));
		_polygon.points.push_back(glTiledTexturedImage::point(cw,0));
		_polygon.points.push_back(glTiledTexturedImage::point(hw,hh));
		_polygon.points.push_back(glTiledTexturedImage::point(0,ch));
		_polygon.points.push_back(glTiledTexturedImage::point(-hw,hh));
		_polygon.points.push_back(glTiledTexturedImage::point(-cw,0));
	}
	else
	{
		_polygon.points.push_back(glTiledTexturedImage::point(-hw,-hh));
		_polygon.points.push_back(glTiledTexturedImage::point(hw,-hh));
		_polygon.points.push_back(glTiledTexturedImage::point(hw,hh));
		_polygon.points.push_back(glTiledTexturedImage::point(-hw,hh));
	}

	_polygon.reset();

	_folding = _foldingBack = false;
	_changed = true;
	_selectingRegion = false;
	_markingFeedback = 0;

	if (false && _window->getPseudoSwallowedBy() != 0)
	{
		// recussion
		MetisseWindow *win =
			dynamic_cast<MetisseWindow *>
			(_window->getPseudoSwallowedBy());
		win->getRenderer()->simpleReset();
	}
}

void WindowRenderer::simpleReset(void)
{
	_reset();
}

void WindowRenderer::_shapeDisplay(bool on)
{
	if ((_nShapeRects == 0 || !_canUseStencilBuffer) &&
	    (_nCutShapeRects == 0))
	{
		return;
	}

	if (on)
	{
		// this slow down things
		//glClearStencil(0);
		//glClear(GL_STENCIL_BUFFER_BIT);

		glEnable(GL_STENCIL_TEST);

		// test always fail     ref, mask
		glStencilFunc(GL_NEVER, 0x1, 0x1);
		// replace by ref if the test fail
		glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);

		float hw,hh;
		_window->getSize(&hw, &hh);
		hw=hw/2.0; hh=hh/2.0;
		int i;
		for (i = 0; i < _nShapeRects; i++)
		{
			float x = _shape[i*4];
			float y = _shape[i*4 + 1];
			float w = _shape[i*4 + 2];
			float h = _shape[i*4 + 3];

			GLdouble x1, y1, x2, y2;
			x1 = -hw + x; y1 = hh - y;
			x2 = x1 + w; y2 = y1 - h;
			glRectd(x1,y1,x2,y2);
		}
		if (_nCutShapeRects > 0)
		{
			glStencilOp(GL_INVERT, GL_INVERT, GL_INVERT);
			if (_nShapeRects == 0)
			{
				glRectd(-hw,hh,hw,-hh);
			}
		}
		for (i = 0; i < _nCutShapeRects; i++)
		{
			float x = _cutShape[i*4];
			float y = _cutShape[i*4 + 1];
			float w = _cutShape[i*4 + 2];
			float h = _cutShape[i*4 + 3];
			
			GLdouble x1, y1, x2, y2;
			x1 = -hw + x; y1 = hh - y;
			x2 = x1 + w; y2 = y1 - h;
			glRectd(x1,y1,x2,y2);
		}

		glStencilFunc(GL_EQUAL, 0x1, 0x1);
		glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
	}
	else
	{
		glDisable(GL_STENCIL_TEST);
	}
}

void WindowRenderer::display(dlPolicy policy)
{
	if (!_window->doDraw())
	  return;

	float width,height;
	_window->getSize(&width,&height);

	float red, green, blue, alpha;
	_window->getBgcolor(&red, &green, &blue, &alpha);

	std::list<glTiledTexturedImage::point>::iterator i;
	std::list<glTiledTexturedImage::point>::iterator j;

	ShaderRenderer *shadRend = _getCurrentShaderRenderer();
	glShader *shad = 0;
	if (shadRend)
	{
		shad = shadRend->shader;
	}

	glDisable(GL_DEPTH_TEST);

	// ----------------------------------
	_shapeDisplay(true);

	// --------------------------------
	glTiledTexturedImage *texture = 0;

	int texCoordsSize = 0;
	float *texCoords = 0;
	_window->getTextureMapping(&texture, &texCoordsSize, &texCoords);

	glColor4f(red,green,blue,alpha);

	if (texture && texCoordsSize==8 && texCoords)
	{
		float left = texCoords[0];
		float bottom = texCoords[1];
		float right = texCoords[2];
		float top = texCoords[5];
		float Ox = left+(right-left)/2.0;
		float Oy = top+(bottom-top)/2.0;

		texture->enable();

		if (shadRend && shad)
		{
			shad->activate();
			if (!shad->setUniformParam(
				    "width", (GLfloat)(1/width)) ||
			    !shad->setUniformParam(
				    "height", (GLfloat)(1/height)))
			{
			}
			if (!shad->setUniformParam(
				    "alpha", (GLfloat)alpha))
			{
				//std::cerr << "setUniformParam Fail (alpha)\n";
			}
			if (shadRend->opt->type & SHADER_RENDERER_ANIMATE)
			{
				if (!shad->setUniformParam(
					    "nbr_anim_step",
					    (GLfloat)(shadRend->opt->nbr_anim_step))
				    ||
				    !shad->setUniformParam(
					    "current_step",
					    (GLfloat)(shadRend->current_step)))
				{
					//std::cerr << "setUniformParam Fail\n";
				}
			}
		}

		//texture->enable();
		
		if (_img_points.size() > 0)
		{
			texture->display(&(_polygon.poly1), &_img_points);
		}
		else if (_testPolygon || _folding || _foldingBack ||
			 _cropToCircle)
		{
			// tessellator should be used and are used if the nbr of 
			// texture in the tile is > 1
			texture->tessDisplay(&(_polygon.poly1), _useTessCache);
			_useTessCache = true;
			texture->getLastTessellation(&_tiled_tessellation);
		}
		else
		{
			_useTessCache = false;
			texture->display();
		}

		//texture->disable();

		if (shad)
		{
			shad->deactivateAllShaders();
		}


		texture->disable();

		if (_nShapeRects == 0)
		{
			// Hum ...
			glColor4f(0.5,0.5,0.5,0);
			glBegin(GL_LINE_LOOP);
			for (i=_polygon.poly1.begin(); i!=_polygon.poly1.end();
			     ++i)
			{
				double xp=(*i).x, yp=(*i).y;
				double xt = Ox + xp*(right-left)/width;
				double yt = Oy + yp*(top-bottom)/height;
				//glTexCoord2d(xt, yt);
				glVertex2d(xp,yp);
			}
			glEnd();
		}
	}
	else
	{
		glBegin(GL_POLYGON);
		for (i=_polygon.poly1.begin(); i!=_polygon.poly1.end(); ++i)
		{
			glVertex2d((*i).x,(*i).y);
		}
		glEnd();
	}

	_shapeDisplay(false);
	
	if (_polygon.poly2.size())
	{
		glTranslatef(0,0,1);
		//glColor4f(0.6*red,0.6*green,0.6*blue,alpha*_foldAlpha);
		glColor4f(0.811,0.843, 0.874,alpha*_foldAlpha);
		glBegin(GL_POLYGON);
		for (i=_polygon.poly2.begin(); i!=_polygon.poly2.end(); ++i)
		{
			glVertex2d((*i).x, (*i).y);
		}
		glEnd();
		//glColor4f(0,0,0,alpha*_foldAlpha);
		glColor4f(0.71,0.764,0.811,alpha*_foldAlpha);
		glTranslatef(0,0,1);
		for (i=_polygon.poly2.begin(); i!=_polygon.poly2.end(); ++i)
		{
			j = i;
			j++;
			if (j == _polygon.poly2.end())
			{
				j=_polygon.poly2.begin();
			}
			glBegin(GL_POLYGON);
			glVertex2d((*i).x, (*i).y);
			glVertex2d((*j).x, (*j).y);
			glVertex2d((*j).x - 1, (*j).y - 1);
			glVertex2d((*i).x - 1, (*i).y - 1);
			glEnd();
			glBegin(GL_POLYGON);
			glVertex2d((*i).x, (*i).y);
			glVertex2d((*j).x, (*j).y);
			glVertex2d((*j).x + 1, (*j).y + 1);
			glVertex2d((*i).x + 1, (*i).y + 1);
			glEnd();
			glBegin(GL_POLYGON);
			glVertex2d((*i).x, (*i).y);
			glVertex2d((*j).x, (*j).y);
			glVertex2d((*j).x + 1, (*j).y - 1);
			glVertex2d((*i).x + 1, (*i).y - 1);
			glEnd();
			glBegin(GL_POLYGON);
			glVertex2d((*i).x, (*i).y);
			glVertex2d((*j).x, (*j).y);
			glVertex2d((*j).x - 1, (*j).y + 1);
			glVertex2d((*i).x - 1, (*i).y + 1);
			glEnd();
			glBegin(GL_POLYGON);
			glVertex2d((*i).x, (*i).y);
			glVertex2d((*j).x, (*j).y);
			glVertex2d((*j).x - 1, (*j).y + 1);
			glVertex2d((*i).x + 1, (*i).y - 1);
			glEnd();
			glBegin(GL_POLYGON);
			glVertex2d((*i).x, (*i).y);
			glVertex2d((*j).x, (*j).y);
			glVertex2d((*j).x + 1, (*j).y - 1);
			glVertex2d((*i).x - 1, (*i).y + 1);
			glEnd();
		}
	}

	

	// Window Selected Region
	std::list<WinRegions::xyregion > xycs =
		_winRegions->getNormalizedXYRegions();
	std::list<WinRegions::xyregion >::iterator iter;

	for (iter=xycs.begin(); iter!= xycs.end(); ++iter)
	{
		GLdouble x1,y1,x2,y2;
		x1 = (*iter).x1 -1; y1 = (*iter).y1 +1;
		x2 = (*iter).x2 +1; y2 = (*iter).y2 -1;

		glColor4f(0.1,0.1,0.1, 0.3 * getWindow()->getAlpha());
		glRectf(x1,y1,x2,y2);

		glColor4f(1,1,1, getWindow()->getAlpha());
		glBegin(GL_LINE_LOOP);
		glVertex2d(x1,y1); glVertex2d(x2,y1);
		glVertex2d(x2,y2); glVertex2d(x1,y2);
		glEnd();
	}

	// region set
	for (iter=_regionSet.begin(); iter!= _regionSet.end(); ++iter)
	{
		GLdouble x1,y1,x2,y2;
		x1 = (*iter).x1 -1; y1 = (*iter).y1 +1;
		x2 = (*iter).x2 +1; y2 = (*iter).y2 -1;

		glColor4f(0.1,0.1,0.1, 0.2 * getWindow()->getAlpha());
		glRectf(x1,y1,x2,y2);

		glColor4f(1,1,1, getWindow()->getAlpha());
		glBegin(GL_LINE_LOOP);
		glVertex2d(x1,y1); glVertex2d(x2,y1);
		glVertex2d(x2,y2); glVertex2d(x1,y2);
		glEnd();
	}

	if (_markingFeedback)
	{
		//glTranslatef(0,0,1);
		goodies_marcking_feedback(
			_mf_x, _mf_y, _mf_dist, _mf_draw_dist, _mf_dirs,
			getWindow()->getAlpha());
	}

	double cross_x, cross_y;
	if (_window->getCrossPosition(&cross_x, &cross_y))
	{
		glBegin(GL_LINES);
		glColor4f(1,1,1, getWindow()->getAlpha());
		glVertex2d(cross_x-20,cross_y); glVertex2d(cross_x+20,cross_y);
		glVertex2d(cross_x,cross_y-20); glVertex2d(cross_x,cross_y+20);
		glColor4f(0,0,0, getWindow()->getAlpha());
		glVertex2d(cross_x-20,cross_y-1);
		glVertex2d(cross_x+20,cross_y-1);
		glVertex2d(cross_x+1,cross_y-20);
		glVertex2d(cross_x+1,cross_y+20);
		glEnd();
	}

	/* add shadow */
	if (_shadow && !getWindow()->isRootWindow() && 
	    !getWindow()->isEwmhDesktop() &&_nShapeRects == 0)
	{
		float hw, hh;
		_window->getSize(&hw, &hh);
		float startAlpha = 0.3 * getWindow()->getAlpha();
		GLfloat shborder = 5;
		if (getWindow()->isUnmanaged() || getWindow()->isToolTips())
		{
			shborder = 3;
		}
		//goodies_shadow(hw, hh, startAlpha, shborder);
		goodies_shadow(
			_polygon, startAlpha, 0.5*alpha*_foldAlpha, shborder);
	}

	if (_lines_points.size() > 0)
	{
		for (i=_lines_points.begin(); i!=_lines_points.end(); ++i)
		{
			j = i;
			j++;
			shadow_line((*i).x, (*i).y, (*j).x, (*j).y, 0.5, 2);
			i++;
		}
	}

	if (texCoords)
	{
		delete [] texCoords;
	}
}

// ----------------------------------------------------------------------
//
  
void WindowRenderer::select(void)
{
	glDisable(GL_DEPTH_TEST);

	std::list<glTiledTexturedImage::point>::iterator i;
	bool folded = _polygon.poly2.size()>0;

	if (getWindow() != 0 && getWindow()->isToolTips())
	{
		return;
	}

	if (getWindow() != 0 && getWindow()->doPseudoSelection())
	{
		pseudoSelect();
		return;
	}

	if (_useTessCache && _tiled_tessellation.size() > 0)
	{
		glTiledTexturedImage::tiledTessellation:: iterator itt;
		glTiledTexturedImage::tessellation:: iterator it;
		std::list<glTiledTexturedImage::point >:: iterator ip;

		for (itt = _tiled_tessellation.begin();
		     itt != _tiled_tessellation.end(); ++itt)
		{
			for (it = (*itt).begin(); it != (*itt).end(); ++it)
			{
				glBegin((*it).type);
				for (ip = (*it).points.begin();
				     ip != (*it).points.end(); ++ip)
				{
					glVertex2d((*ip).x, (*ip).y);
				}
				glEnd();     
			}
		}
	}
	else
	{
		glBegin(GL_POLYGON);
		for (i=_polygon.poly1.begin(); i!=_polygon.poly1.end(); ++i)
		{
			float xx = (*i).x;
			float yy = (*i).y;
			glVertex2d(xx,yy);
		}
		glEnd();
	}

	if (folded)
	{
		glTranslatef(0,0,1);
		glBegin(GL_POLYGON);
		for (i=_polygon.poly2.begin(); i!=_polygon.poly2.end(); ++i)
		{
			glVertex2d((*i).x, (*i).y);
		}
		glEnd();
	}

}

void WindowRenderer::pseudoSelect()
{
	double hw,hh;
	_window->getSize(&hw,&hh);

	hw= (hw/2.0)*10000;
	hh= (hh/2.0)*10000;

	glBegin(GL_POLYGON);
	glVertex2d(-hw, -hh);
	glVertex2d(hw, -hh);
	glVertex2d(hw, hh);
	glVertex2d(-hw, hh);
	glEnd();
}

// ----------------------------------------------------------------------
//

#define M_FW_BACK_STEP 7
#define M_FW_BACK_STEP_ICONIFICATION 20

void WindowRenderer::react(Observable *obs)
{
	if (_foldingBack && obs==_foldTk)
	{
		//double dx = (_x_to-_x_from);
		//double dy = (_y_to-_y_from);
		//double d = dx*dx+dy*dy;

		_foldAnimStep++;
		double dx = _x_to + _foldAnimStep*(- _x_to + _x_from)/15;
		double dy = _y_to + _foldAnimStep*(- _y_to + _y_from)/15;

		_useTessCache = false;
		//_x_to -= dx*0.2;
		//_y_to -= dy*0.2;
		//int timer = (int)(d/1000.0);
		if (_foldAnimStep >= 15)
		{
			_foldingBack = false;
			unsubscribeFrom(_foldTk);
			delete _foldTk;
			_foldTk = 0;
			_reset();
		}
		else
		{
			//_polygon.fold(_x_from,_y_from,_x_to,_y_to);
			_polygon.fold(_x_from,_y_from, dx, dy,_foldReduction);
			_foldTk->arm(10);
			postRedisplay();
		}
	}

	if (_foldingOn && obs==_foldTk)
	{
		_foldAnimStep++;
		double dx = _x_from + _foldAnimStep*(_x_to - _x_from)/15;
		double dy = _y_from + _foldAnimStep*(_y_to - _y_from)/15;

		_useTessCache = false;
		if (_foldAnimStep >= 15)
		{
			_foldingOn = false;
			unsubscribeFrom(_foldTk);
			delete _foldTk;
			_foldTk = 0;
			_foldingOn = false;
			_polygon.fold(
				_x_from,_y_from,_x_to,_y_to,_foldReduction);
			postRedisplay();
		}
		else
		{
			_polygon.fold(_x_from,_y_from, dx, dy,_foldReduction);
			_foldTk->arm(10);
			postRedisplay();
		}
	}

	if (obs == _geneTk && _scaleBack)
	{
		float x,y,z;

		_window->getAbsoluteScale(&x, &y, &z);
		if (x < 1.0)
		{
			x += _scaleBackStep;
			if (x >= 1.0)
			{
				x = 1.0;
			}
		}
		else if (x != 1.0)
		{
			x -= _scaleBackStep;
			if (x <= 1.0)
			{
				x = 1.0;
			}
		}
		if (y < 1.0)
		{
			y += _scaleBackStep;
			if (y >= 1.0)
			{
				y = 1.0;
			}
		}
		else if (y != 1.0)
		{
			y -= _scaleBackStep;
			if (y <= 1.0)
			{
				y = 1.0;
			}
		}

		_root->scaleWindow(_window, x,y,1, _scaleBackGravity, true);

		if (x == 1.0 && y == 1.0)
		{
			_scaleBack = false;
			unsubscribeFrom(_geneTk);
			delete _geneTk;
			_geneTk = 0;
			//_window->getAbsoluteScale(&x, &y, &z);
			_window->sendMetisseTransform(
				(_inResetOrSet)? true:false);
			if (_inResetOrSet)
			{
				// send stop
			}
			_inResetOrSet = false;
		}
		else
		{
			_geneTk->arm(15);
			_window->sendMetisseTransform(false);
		}
	}

	if (obs == _geneTk && _scheduleHide)
	{
		_window->setRendererFullTransformation(_beforeIconTrans);
		hide();
		_fvwmModule->sendUnlockNotification();
		_scheduleHide = false;
		unsubscribeFrom(_geneTk);
		delete _geneTk;
		_geneTk = 0;
		_inIconification = false;
	}
	
	if (obs == _geneTk && _transBack)
	{
		float curTrans[18];
		_window->getRendererFullTransformation(curTrans);
		bool ok = true;

		int i;
		for (i = 0; i < 18; i++)
		{
			curTrans[i] = curTrans[i] + _transBackSteps[i];
			if (_transBackSteps[i] >= 0 &&
			    _transBackTrans[i] <= curTrans[i])
			{
				curTrans[i] = _transBackTrans[i];
			}
			else if (_transBackSteps[i] <= 0 &&
				 _transBackTrans[i] >= curTrans[i])
			{
				curTrans[i] = _transBackTrans[i];
			}
			else
			{
				ok = false;
			}
		}
		_backStep++;
		if ((!_inIconification && _backStep > M_FW_BACK_STEP) ||
		    (_backStep > M_FW_BACK_STEP_ICONIFICATION))
		{
			// safe
			ok = true;
		}
		_window->setRendererFullTransformation(curTrans);
		if (ok)
		{
			// not enough, what about absolute rotation?
			_window->resetAbsoluteScale();
			_transBack = false;
			_backStep = 0;
			_window->sendMetisseTransform(
				(_inResetOrSet)? true:false);
			_inResetOrSet = false;
			if (_inIconification)
			{
				if (_iconify)
				{
					_scheduleHide = true;
					_geneTk->arm(50);
				}
			}
			if (!(_inIconification && _iconify))
			{
				unsubscribeFrom(_geneTk);
				delete _geneTk;
				_geneTk = 0;
				_inIconification = false;
				if (_doFixCursor)
				{
					_ascr->fixCursorPos(false);
					_doFixCursor = false;
				}
			}
		}
		else
		{
			if (_inIconification)
			{
				_geneTk->arm(7);
			}
			else
			{
				_geneTk->arm(15);
			}
			_window->sendMetisseTransform(false);
		}
	}

	if (obs==_animTk)
	{
		rotate_rel(-1,0,0,1);
		_animTk->arm(20);
		// _window->sendMetisseTransform(false);
	}

	if (obs == _shaderTk)
	{
		ShaderRenderer *sr = _getCurrentShaderRenderer();
		if (sr && !(sr->opt->type & SHADER_RENDERER_ANIMATE))
		{
			_shaderTk->disarm();
			unsubscribeFrom(_shaderTk);
		}
		else if (sr)
		{
			if (sr->back && sr->current_step > 0)
			{
				sr->current_step--;	
			}
			else if (!sr->back &&
				 sr->current_step < sr->opt->nbr_anim_step)
			{
				sr->current_step++;
			}
			if ((!sr->back &&
			     sr->current_step == sr->opt->nbr_anim_step)
			    ||
			    (sr->back && sr->current_step == 0))
			{
				if (sr->opt->type & SHADER_RENDERER_PERIODIC)
				{
					std::cerr << "Anim Shader Period\n";
					sr->back = !sr->back;
					_shaderTk->arm(
						sr->opt->anim_total_time/
						sr->opt->nbr_anim_step);
				}
				else
				{
					_shaderTk->disarm();
					unsubscribeFrom(_shaderTk);
				}
			}
			else
			{
				//std::cerr << "Anim Shader Step\n";
				_shaderTk->arm(
					sr->opt->anim_total_time/
					sr->opt->nbr_anim_step);	
			}
			postRedisplay();
		}
	}
	notifyObservers();
}

void WindowRenderer::fold(
	int px, int py, bool fold, bool anim, float alpha, float reduction)
{
	
	if (fold)
	{
		if (anim)
		{
			_foldReduction = 4;
		}
		else
		{
			_foldReduction = 1;
		}
		if (reduction != -1.0f)
		{
			_foldReduction = reduction;
		}
		if (_folding)
		{
			if (anim)
			{
				_foldingOn = true;
				_foldingBack = false;
				_foldAnimStep = 0;
				_x_to = px;
				_y_to = py;
				//_foldReduction = 1; // for ss
				if (_foldTk == 0)
				{
					_foldTk = TimeKeeper::create();
					subscribeTo(_foldTk);
				}
				_foldTk->arm(0);
			}
			else
			{
				_x_to = px;
				_y_to = py;
				_polygon.fold(
					_x_from, _y_from, _x_to, _y_to,
					_foldReduction); 
				postRedisplay();
			}
		}
		else
		{
			_x_from = px;
			_y_from = py;
			_folding = true;
		}
		_foldAlpha = alpha;
	}
	else
	{
		if (_folding)
		{
			_foldingBack = true;
		}
		else
		{
			return;
		}
		_foldAnimStep = 0;
		_foldingOn = false;
		_folding = false;
		if (_foldTk == 0)
		{
			_foldTk = TimeKeeper::create();
			subscribeTo(_foldTk);
		}
		_foldTk->arm(20);
	}
	_useTessCache = false;
}

void WindowRenderer::foldOff(void)
{
	_reset();
	_folding = false;
}

void WindowRenderer::scaleBack(LayerManager *root, int gravity)
{
	_root = root;
	_scaleBackGravity = gravity;
	_scaleBack = true;
	_inIconification = false;
	_geneTk = TimeKeeper::create();
	subscribeTo(_geneTk);
	_geneTk->arm(1);

	float x,y,z;
	_window->getAbsoluteScale(&x, &y, &z);
	// FIXME: do a better interpolation!
	_scaleBackStep = max(fabs(x-1), fabs(y-1))/10.0;
	if (_scaleBackStep < 0.001)
	{
		_scaleBackStep = 0.001;
	}
}

void WindowRenderer::transformBack(GLfloat *trans)
{
	int step = M_FW_BACK_STEP;
	if (_inIconification)
	{
		step = M_FW_BACK_STEP_ICONIFICATION;
	}
	_transBack = true;
	_backStep = 0;
	_geneTk = TimeKeeper::create();
	subscribeTo(_geneTk);
	if (_inIconification && !_iconify)
	{
		_geneTk->arm(50);
	}
	else
	{
		_geneTk->arm(1);
	}
	  
	GLfloat curTrans[18];
	_window->getRendererFullTransformation(curTrans);
	int i;
	for (i = 0; i < 18; i++)
	{
		_transBackTrans[i] = trans[i];
		_transBackSteps[i] = (trans[i] - curTrans[i])/step;
	}
}

void WindowRenderer::resetOrSet(bool fixCursor)
{
	GLfloat curTrans[18];
	bool isCurId = True;
	int i;

	_inResetOrSet = true;
	_inIconification = false;
	_doFixCursor = fixCursor;
	_window->getRendererFullTransformation(curTrans);
	for (i = 0; i < 16; i++)
	{
		if (fabsf(curTrans[i]-Identity[i]) > 0.01)
		{
			isCurId = False;
			break;
		}
	}
	if (fixCursor)
	{
		_ascr->fixCursorPos(true);
	}
	if (isCurId)
	{
		transformBack(_transMark);
	}
	else
	{
#if 0
		for (i = 0; i < 18; i++)
		{
			fprintf(stderr,"%f ", curTrans[i]);
		}
		fprintf(stderr,"\n");
#endif
		memcpy(_transMark, curTrans, sizeof(GLfloat)*18);
		transformBack(Identity);
	}
}

void WindowRenderer::reset(bool store, bool animate, bool fixCursor)
{
	GLfloat curTrans[18];
	bool isCurId = True;
	int i;

	// FIXME: transmark!
	_window->getRendererFullTransformation(curTrans);
	for (i = 0; i < 16; i++)
	{
		if (fabsf(curTrans[i]-Identity[i]) > 0.01)
		{
			isCurId = False;
			break;
		}
	}
	if (isCurId)
	{
		if (!store)
		{
			memcpy(_transMark, Identity, sizeof(GLfloat)*18);
		}
		return;
	}
	else
	{
		if (store)
		{
			memcpy(_transMark, curTrans, sizeof(GLfloat)*18);
		}
		else
		{
			memcpy(_transMark, Identity, sizeof(GLfloat)*18);
		}
		if (fixCursor)
		{
			_ascr->fixCursorPos(true);
		}
		if (animate)
		{
			_inResetOrSet = true;
			_inIconification = false;
			_doFixCursor = fixCursor;
			transformBack(Identity);
		}
		else
		{
			_window->setRendererFullTransformation(Identity);
			if (fixCursor)
			{
				_ascr->fixCursorPos(false);
			}
		}
	}
}


void WindowRenderer::iconify(
	LayerManager *root, FvwmModule *fvwmModule, bool on,
	float icon_x, float icon_y, float icon_w, float icon_h,
	float frame_x, float frame_y, float frame_w,float frame_h)
{
	// WARNING: this function should leads to a SendUnlockNotification
	// at some point (if on)

	_root = root;
	_fvwmModule  = fvwmModule;

	if (_inIconification)
	{
		_window->setRendererFullTransformation(_beforeIconTrans);
		_fvwmModule->sendUnlockNotification();
		_scheduleHide = false;
		_inIconification = false;
		if (_geneTk)
		{
			unsubscribeFrom(_geneTk);
			delete _geneTk;
			_geneTk = 0;
		}
		
		if (!on)
		{
			unHide();
		}
		else
		{
			hide();
		}
		return;
	}

	if (frame_w <= 0 || frame_h <= 0 || icon_w <= 0 || icon_h <= 0 ||
	    _window->neverMapped())
	{
		if (!on)
		{
			unHide();
		}
		else
		{
			hide();
		}
		_fvwmModule->sendUnlockNotification();
		return;
	}

	// we get absolute X11 corrdinates
	GLfloat curTrans[18];

	_window->getRendererFullTransformation(_beforeIconTrans);

	// compute final or start coordinates
	GLfloat iconTrans[18];

	// translate
	float oldX, oldY;
	_window->getRealPosition(&oldX, &oldY);
	float sx = 1, sy = 1;
	float tx,ty;
	_root->getScale(&sx,&sy); // FIXME: should (un)project over root
	if (0 /*_portrait*/)
	{
		ty = ((float)(-frame_x + icon_x))*(1/sx);
		tx = -((float)(frame_y - icon_y))*(1/sy);
	}
	else
	{
		tx = ((float)( - (frame_x+(frame_w/2)) +
			      (icon_x + (icon_w/2))) )*(1/sx);
		ty = ((float)( (frame_y+(frame_h/2)) -
			       (icon_y + (icon_h/2)) ))*(1/sy);
	}

	_window->setRendererTransformation(Identity);
	_window->internalTranslation(tx, ty, 1);
	_root->scaleWindow(
		_window, icon_w/frame_w, icon_h/frame_h, 1,
		CenterGravity /* gravity: 0 guess */, 1 /* absolute? */);
	_window->getRendererFullTransformation(iconTrans);

	_inIconification = True;
	if (on)
	{
		_iconify = true;
		_window->setRendererFullTransformation(_beforeIconTrans);
		transformBack(iconTrans);
	}
	else
	{
		_iconify = false;
		transformBack(_beforeIconTrans);
	}
}

void WindowRenderer::unsnap(void)
{
	_do_x_snapping = false;
	_do_y_snapping = false;
	_useTessCache = false;
	_reset();
}

void WindowRenderer::snap(
	bool do_x, MetisseWindow *win_x, int snap_x, bool right,
	bool do_y, MetisseWindow *win_y, int snap_y, bool top)
{
	if (do_x)
	{
		_do_x_snapping = true;
		_snap_right = right;
		_snap_xoff = snap_x;
	}
	else
	{
		_do_x_snapping = false;
	}
	if (do_y)
	{
		_do_y_snapping = true;
		_snap_top = top;
		_snap_yoff = snap_y;
		//fprintf(stderr,"MSY OFF: %f\n", _snap_yoff);
	}
	else
	{
		_do_y_snapping = false;
	}
	_useTessCache = false;
	_reset();
}

void WindowRenderer::unlens(void)
{
	bool do_reset = (_do_lens || _do_blens);
	_do_lens = false;
	_do_blens = false;
	_in_pm_lens = false;
	_useTessCache = false;
	if (do_reset)
		_reset();
}

void WindowRenderer::lens(
	GLfloat ax, GLfloat ay, GLfloat x, GLfloat y,
	GLfloat width, GLfloat height, GLfloat scale_x, GLfloat scale_y)
{
	_lens_ax = ax;
	_lens_ay = ay;
	_lens_x = x;
	_lens_y = y;
	_lens_w = width;
	_lens_h = height;
	_lens_sx = scale_x;
	_lens_sy = scale_y;
	_do_lens = true;
	_do_blens = false;

	_useTessCache = false;
	_reset();
}
void WindowRenderer::borderLens(
	GLfloat xax, GLfloat xay, GLfloat xx, GLfloat xy,
	GLfloat xwidth, GLfloat xheight, GLfloat xscale_x, GLfloat xscale_y,
	GLfloat yax, GLfloat yay, GLfloat yx, GLfloat yy,
	GLfloat ywidth, GLfloat yheight, GLfloat yscale_x, GLfloat yscale_y)
{
	_lens_ax = xax;
	_lens_ay = xay;
	_lens_x = xx;
	_lens_y = xy;
	_lens_w = xwidth;
	_lens_h = xheight;
	_lens_sx = xscale_x;
	_lens_sy = xscale_y;

	_blens_ax = yax;
	_blens_ay = yay;
	_blens_x = yx;
	_blens_y = yy;
	_blens_w = ywidth;
	_blens_h = yheight;
	_blens_sx = yscale_x;
	_blens_sy = yscale_y;

	_do_blens = true;
	_do_lens = true;

	_useTessCache = false;
	_reset();
}

void WindowRenderer::lensSnapAndGo(
	bool do_x, GLfloat offx, bool right, bool left_to_right,
	bool do_y, GLfloat offy, bool top, bool bottom_to_top)
{
	GLfloat ax, ay, x, y, width, height, scale_x, scale_y;
	GLfloat bax, bay, bx, by, bwidth, bheight, bscale_x, bscale_y;
	bool blens = false;
	GLfloat sad = _ascr->root->getSnapAttractDist();
	
	width = bwidth = 0;
	height = bheight = 0;
	GLfloat win_width, win_height;
	_window->getSize(&win_width, &win_height);

	if (do_x && do_y)
	{
		blens = true;	
	}
	if (do_x && !right)
	{
		x = - win_width/2.0;
		if (left_to_right)
		{
			ax = - win_width/2.0 - offx;
		}
		else
		{
			ax = - win_width/2.0 - (sad + offx);
		}
		ay = y = - win_height/2.0;
		width = 1;
		height = win_height;
		scale_x = sad;
		scale_y = 1;
	}
	else if (do_x && right)
	{
		x = win_width/2.0 - 1;
		if (left_to_right)
		{
			ax = win_width/2.0 - 1 - offx;
		}
		else
		{
			ax = win_width/2.0 - 1 - (sad + offx);
		}
		ay = y = - win_height/2.0;
		width = 1;
		height = win_height;
		scale_x = sad;
		scale_y = 1;
	}
	if (do_y && !top)
	{

		bax = bx = - win_width/2.0;
		by = - win_height/2.0;
		if (bottom_to_top)
		{
			bay = - win_height/2.0 + offy;
		}
		else
		{
			bay = - win_height/2.0 - (sad - offy);
		}
		bwidth = win_width;
		bheight = 1;
		bscale_x = 1;
		bscale_y = sad;
	}
	else if (do_y && top)
	{
		bax = bx = - win_width/2.0;
		by = win_height/2.0 - 1;
		if (bottom_to_top)
		{
			bay = win_height/2.0 -1 + offy;
		}
		else
		{
			bay = win_height/2.0 -1 - (sad - offy);
		}
		bwidth = win_width;
		bheight = 1;
		bscale_x = 1;
		bscale_y = sad;
	}

	if ((width == 0 && bwidth == 0) || (height == 0 && bheight == 0))
	{
		unlens();
		return;
	}

	if (do_x && !do_y)
	{
		lens(ax, ay, x, y, width, height, scale_x, scale_y);
	}
	else if (!do_x && do_y)
	{
		lens(bax, bay, bx, by, bwidth, bheight, bscale_x, bscale_y);
	}
	else if (do_x && do_y)
	{
		borderLens(
			ax, ay, x, y, width, height, scale_x, scale_y,
			bax, bay, bx, by, bwidth, bheight, bscale_x, bscale_y);
	}
}

void WindowRenderer::lensRegions(float sx, float sy)
{
	std::list<WinRegions::xyregion > xycs =
		_winRegions->getNormalizedXYRegions();

	if (xycs.size() <= 0)
	{
		return;
	}
	
	GLfloat ax, ay, x, y, width, height, scale_x, scale_y;
	std::list<WinRegions::xyregion >::iterator iter;
	iter=xycs.begin();
	GLdouble x1,y1,x2,y2;
	x1 = (*iter).x1 -1; y1 = (*iter).y1 +1;
	x2 = (*iter).x2 +1; y2 = (*iter).y2 -1;

	
	x = (*iter).x1;
	y = (*iter).y2;
	width = fabs((*iter).x2 - (*iter).x1);
	height = fabs((*iter).y2 - (*iter).y1);
	scale_x = sx;
	scale_y = sy;

	ax = x + width/2.0 - (width*sx/2.0);
	ay = y + height/2.0 - (height*sy/2.0);

	std::cerr << ax << " " << ay << " " << x << " " << y << " " << width << " " << height << " " << sx << " " << sy << "\n";
	lens(ax, ay, x, y, width, height, scale_x, scale_y);

	rmSelectedRegions();
}

void WindowRenderer::setAutoLensType(int lt)
{
	_auto_lens_type = lt;
	unlens();
	//reset();
}

// ---------------------------------------------------------------

void WindowRenderer::cropToCircule(void)
{
	_cropToCircle = !_cropToCircle;
	_testPolygon = false;
	_useTessCache = false;
	_reset();
}

void WindowRenderer::testPolygon(void)
{
	_testPolygon = !_testPolygon;
	_cropToCircle = false;
	_useTessCache = false;
	_reset();
}

void WindowRenderer::animate(void)
{
	if (_animTk)
	{
		unsubscribeFrom(_animTk);
		delete _animTk;
		_animTk = 0;
	}
	else
	{
		_animTk = TimeKeeper::create();
		_animTk->arm(1);
		subscribeTo(_animTk);
	}
}


// ----------------------------------------------------------------------
//

bool WindowRenderer::_checkCurrentMotorPointing(float x, float y)
{

	if (!_mp_in)
	{
		return false;
	}

	if (!_ascr->inInteractiveResize() &&
	    _mp_wix <= _csc_ix && _csc_ix < _mp_wix + _mp_wiw &&
	    _mp_wiy <= _csc_iy && _csc_iy < _mp_wiy + _mp_wih)
	{
		return true;
	}

	return false;
}

bool WindowRenderer::_getNewMotorPointingWidget(float x, float y)
{
	GLfloat w,h,realx,realy;
	
	_window->getSize(&w,&h);
	_window->getRealPosition(&realx, &realy);

	if (WindowRenderer::borderVisLens != 1.0)
	{
		// borders
		int bw = _window->getBorderWidth();
		_mp_wix = (int)(w - bw + realx);
		_mp_wiy = (int)(realy);
		_mp_wiw = bw;
		_mp_wih = (int)h;
		if (!_ascr->inInteractiveResize() &&
		    _mp_wix <= _csc_ix && _csc_ix < _mp_wix+_mp_wiw &&
		    _mp_wiy <= _csc_iy && _csc_iy < _mp_wiy + _mp_wih)
		{
			_mp_scale_x = WindowRenderer::borderVisLens;
			_mp_scale_y = 1;
			#if 0
			std::cerr << "New motor Pointing widget " <<
			  " x: " << _mp_wix << " y: " << _mp_wiy <<
			  " w: " << _mp_wiw << " h: " << _mp_wih << std::endl;
			#endif
			return true;
		}
		_mp_wix = (int)(realx);
		_mp_wiy = (int)(realy);
		_mp_wiw = bw;
		_mp_wih = (int)h;
		if (!_ascr->inInteractiveResize() &&
		    _mp_wix <= _csc_ix && _csc_ix < _mp_wix+_mp_wiw &&
		    _mp_wiy <= _csc_iy && _csc_iy < _mp_wiy + _mp_wih)
		{
			_mp_scale_x = WindowRenderer::borderVisLens;
			_mp_scale_y = 1;
			#if 0
			std::cerr << "New motor Pointing widget " <<
			  " x: " << _mp_wix << " y: " << _mp_wiy <<
			  " w: " << _mp_wiw << " h: " << _mp_wih << std::endl;
			#endif
			return true;
		}
		_mp_wix = (int)(realx);
		_mp_wiy = (int)(realy);
		_mp_wiw = (int)w;
		_mp_wih = (int)bw;
		if (!_ascr->inInteractiveResize() &&
		    _mp_wix <= _csc_ix && _csc_ix < _mp_wix+_mp_wiw &&
		    _mp_wiy <= _csc_iy && _csc_iy < _mp_wiy + _mp_wih)
		{
			_mp_scale_y = WindowRenderer::borderVisLens;
			_mp_scale_x = 1;
			#if 0
			std::cerr << "New motor Pointing widget " <<
			  " x: " << _mp_wix << " y: " << _mp_wiy <<
			  " w: " << _mp_wiw << " h: " << _mp_wih << std::endl;
			#endif
			return true;
		}
		_mp_wix = (int)(realx);
		_mp_wiy = (int)(h - bw + realy);
		_mp_wiw = (int)w;
		_mp_wih = (int)bw;
		if (!_ascr->inInteractiveResize() &&
		    _mp_wix <= _csc_ix && _csc_ix < _mp_wix+_mp_wiw &&
		    _mp_wiy <= _csc_iy && _csc_iy < _mp_wiy + _mp_wih)
		{
			_mp_scale_y = WindowRenderer::borderVisLens;
			_mp_scale_x = 1;
			#if 0
			std::cerr << "New motor Pointing widget " <<
			  " x: " << _mp_wix << " y: " << _mp_wiy <<
			  " w: " << _mp_wiw << " h: " << _mp_wih << std::endl;
			#endif
			return true;
		}
	}

	return false;
}

void WindowRenderer::_doMotorPointing(float x, float y, float *rx, float *ry)
{
	*rx = x;
	*ry = y;

	GLfloat w,h,hw,hh,realx,realy;
	
	_window->getSize(&w,&h);
	hw= w/2.0; hh= h/2.0;
	_window->getRealPosition(&realx, &realy);

	int ppx, ppy, cpx, cpy;
	_ascr->getPrevPointerPosition(&ppx,&ppy);
	_ascr->getPointerPosition(&cpx, &cpy);

	#if 0
	std::cerr << "wx: " << realx << " wy: " << realy << " hw: " << hw
		  << " ww: " << hw << " x: " << x << " y: " << y <<"\n";
	std::cerr << "ix: " << _csc_ix << "/" << (float)(x+hw+realx) << " iy: "
		  << _csc_iy << " ppx: " << ppx
		  << " ppy: " << ppy << " cpx: " << cpx << " cpy: " << cpy
		  << "\n";
	#endif
	
	if (!_mp_in)
	{
		GLfloat d;

		// entring the widget
		if (_mp_scale_x == 1.0)
		{
			_mp_mx = _csc_ix;	
		}
		else if (ppx >= _mp_wix + _mp_wiw)
		{
			// from the right
		
			d = _csc_ix - (_mp_wix + _mp_wiw - 1);
			d = d/_mp_scale_x;
			_mp_mx = (GLfloat)(_mp_wix + _mp_wiw - 1) + d;
			#if 0
			std::cerr << "from the right mx:" << _mp_mx
				  << "/" << (int)rint(_mp_mx)
				  << std::endl;
			#endif
		}
		else if (ppx < _mp_wix)
		{
			// from the left
			d = _csc_ix - _mp_wix;
			d = d/_mp_scale_x;
			_mp_mx = (GLfloat)(_mp_wix) + d;
			#if 0
			std::cerr << "from the left mx:" << _mp_mx
				  << "/" << (int)rint(_mp_mx)
				  << std::endl;
			#endif
		}
		else
		{
			// from nowhere
			_mp_mx = (GLfloat)_csc_ix;
			#if 0
			std::cerr << "from the nowhere mx:" << _mp_mx
				  << std::endl;
			#endif
		}

		if (_mp_scale_y == 1.0)
		{
			_mp_my = _csc_iy;	
		}
		else if (ppy >= _mp_wiy + _mp_wih)
		{
			// from the top
			d = _csc_iy - (_mp_wiy + _mp_wih - 1);
			d = d/_mp_scale_y;
			_mp_my = (GLfloat)(_mp_wiy + _mp_wih - 1) + d;
			#if 0
			std::cerr << "from the right my:" << _mp_my
				  << "/" << (int)rint(_mp_my)
				  << std::endl;
			#endif
		}
		else if (ppy < _mp_wiy)
		{
			// from the bottom
			d = _csc_iy - _mp_wiy;
			d = d/_mp_scale_y;
			_mp_my = (GLfloat)(_mp_wiy) + d;
			#if 0
			std::cerr << "from the left my:" << _mp_my
				  << "/" << (int)rint(_mp_my)
				  << std::endl;
			#endif
		}
		else
		{
			// from nowhere
			_mp_my = (GLfloat)_csc_iy;
			#if 0
			std::cerr << "from the nowhere my:" << _mp_my
				  << std::endl;
			#endif
		}
	}
	else
	{
		if (_mp_scale_x == 1.0)
		{
			_mp_mx = _csc_ix;	
		}
		else
		{
			_mp_mx = _mp_mx + ((float)(cpx - ppx))/_mp_scale_x;
		}
		if (_mp_scale_y == 1.0)
		{
			_mp_my = _csc_iy;	
		}
		else
		{
			_mp_my = _mp_my + ((float)(cpy - ppy))/_mp_scale_y;
			#if 0
			std::cerr << "in mp:" << _mp_my << "/"
				  << (int)rint(_mp_my)
				  << std::endl;
			#endif
		}
	}

	int imx = (int)rint(_mp_mx);
	int imy = (int)rint(_mp_my);
	if (_csc_ix != imx || _csc_iy != imy)
	{
		_ascr->warpCursor(imx, imy);
		//std::cerr << "warp " << imx << " " << imy << std::endl;
		_mp_warp = true;
	}
	*rx = x + (imx - _csc_ix);
	*ry = y - (imy - _csc_iy);
	//std::cerr << "new x y:" << *rx << " "  << *ry << std::endl;

	GLfloat ax, ay, lx, ly, width, height, scale_x, scale_y;
	
	if (_mp_scale_x == 1.0)
	{
		lx = -hw;
		ax = -hw;
	}
	else
	{
		lx = _mp_wix - hw - realx; //hw - _mp_wiw;
		ax = lx - (int)(_mp_scale_x*(x + (_mp_mx - _csc_ix) - lx));
	}
	if (_mp_scale_y == 1.0)
	{
		ly = -hh;
		ay = -hh;
	}
	else
	{
		// FIXME
		ly = _mp_wiy - hh - realy;
		ay = ly - (int)(_mp_scale_y*(y - (_mp_my - _csc_iy) - ly));
	}

	width = _mp_wiw;
	height = _mp_wih;
	scale_x = _mp_scale_x;
	scale_y = _mp_scale_y;

	if (_mp_scale_y == 1.0)
	{
		lens(ax, ay, lx, ly, width, height, scale_x, scale_y);
	}
}

// ----------------------------------------------------------------------
//
#if 0
void WindowRenderer::_setupLense(void)
{
}
#endif

// ----------------------------------------------------------------------
//
void WindowRenderer::keyEvent(unsigned long key, bool down_flag)
{
	_window->keyEvent(key, down_flag);
}

void WindowRenderer::pointerLeave(void)
{
	if (_auto_lens_type != LENS_TYPE_NONE || 
	    WindowRenderer::borderVisLens != 1.0 ||
	    _in_pm_lens)
	{
		unlens();
	}
	_mp_warp = _mp_in = false;
}

void WindowRenderer::pointerEvent(float x, float y, unsigned long button_mask)
{
  // std::cerr << "WindowRenderer::pointerEvent: " << x << "," << y << ":" << button_mask << std::endl ;
	Bool do_motor_pointing = false;
	
	if (_mp_warp)
	{
		//std::cerr << "get warped " << x << " " << y << std::endl;
		_mp_warp = false;
		return;
	}

	GLfloat w,h,hw,hh,realx,realy;
	_window->getSize(&w,&h);
	hw= w/2.0; hh= h/2.0;
	_window->getRealPosition(&realx, &realy);

	_csc_ix = _ascr->obc2x11scX(x, hw, realx);
	_csc_iy = _ascr->obc2x11scY(y, hh, realy);

	float pmsx,pmsy;
	bool do_pm_lens = false;

#if 0
	// pager mode lenses
	_ascr->root->getPagerModeScale(&pmsx,&pmsy);
	if (pmsx != 1.0 || pmsy != 1.0)
	{
		do_pm_lens = true;
	}
#endif

#if 1
	float new_x = x, new_y = y;

	if (_mp_in)
	{
		//fprintf(stderr,"IN Motor Pointing\n");
		_doMotorPointing(x, y, &new_x, &new_y);
		x = new_x;
		y = new_y;
	}
	if (_checkCurrentMotorPointing(x, y))
	{

		//fprintf(stderr,"Current Motor Pointing %i\n", _mp_in);
		if (!_mp_in)
		{
			do_motor_pointing = true;
		}
	}
	else if (_getNewMotorPointingWidget(x, y))
	{

		//fprintf(stderr,"New motor Pointing widget\n");
		do_motor_pointing = true;
		_mp_warp = _mp_in = false;
		unlens();
		//_mp_in = true;
	}
	else
	{
		_mp_in = false;
		//unlens();
	}
	
	if (do_motor_pointing)
	{
		_doMotorPointing(x, y, &new_x, &new_y);
		_mp_in = true;
	}

	x = new_x;
	y = new_y;
#endif

	if (!_mp_in && !do_motor_pointing &&
	    (_auto_lens_type != LENS_TYPE_NONE ||
	     (_window->isUnmanaged() && _menu_lens) ||
	     do_pm_lens))
	{
		GLfloat ax, ay, lx, ly, width, height, scale_x, scale_y;

		width = 0; height = 0;
		if (_window->isRootWindow() || _ascr->inInternalMove(_window))
		{
			// ok
		}
		else if (_auto_lens_type == LENS_TYPE_HORIZ ||
			 (_window->isUnmanaged()))// & !do_pm_lens))
		{
			// horizontal lens
			lx = 0;
			ly = fmax(y-(_lens_horiz_h/2.0),-hh);
			width = w;
			height = fmin(_lens_horiz_h,h);
			scale_x = 1;
			scale_y = 3;
		}
		else if (_auto_lens_type == LENS_TYPE_VERT)
		{
			// vertical lens
			lx = fmax(x-(_lens_vert_w/2),-hw);
			ly = -hh;
			width = fmin(_lens_vert_w,w);
			height = h;
			scale_x = 2;
			scale_y = 1;
		}
		else if (_auto_lens_type == LENS_TYPE_CROSS)
		{
			lx = fmax(x-(_lens_vert_w/2),-hw);
			ly = fmax(y-(_lens_horiz_h/2),-hh);
			width = fmin(_lens_vert_w, w);
			height = fmin(_lens_horiz_h, h);
			scale_x = 2;
			scale_y = 2;
		}
		else if (_auto_lens_type == LENS_TYPE_CENTER)
		{
			lx = fmax(x-(_lens_wh_w/2),-hw);
			ly = fmax(y-(_lens_wh_h/2),-hh);
			width = fmin(_lens_wh_w,w);
			height = fmin(_lens_wh_h,h);
			scale_x = 2;
			scale_y = 2;
			width = w/(4.0*scale_x);
			height =h/(4.0*scale_y);
			lx = fmax(x-(width/2.0),-hw);
			ly = fmax(y-(height/2.0),-hh);
		}
		else if (do_pm_lens)
		{
			float wsx,wsy,wsz;
			getWindow()->getAbsoluteScale(&wsx,&wsy,&wsz);
			_in_pm_lens = true;
			scale_x = wsx/pmsx;
			scale_y = wsy/pmsy;
			width = w/(1.0*scale_x);
			height =h/(1.0*scale_y);
			lx = fmax(x-(width/2.0),-hw);
			ly = fmax(y-(height/2.0),-hh);
		}
		if (width != 0 && height != 0)
		{
			// FIXME: extreme cases ... redirecte the input !
			if (lx + width > hw)
			{
				lx = lx - (lx + width - hw);
			}
			if (ly + height > hh)
			{
				ly = ly - (ly + height - hh);
			}
			ax = lx + width/2.0 - (width*scale_x/2.0);
			ay = ly + height/2.0 - (height*scale_y/2.0);
			if (ay + height*scale_y > hh)
			{
				ay = hh - height*scale_y;
			}
			if (ay < -hh)
			{
				ay = -hh;
			}
			if (ax + width*scale_x > hw)
			{
				ax = hw - width*scale_x;
			}
			if (ax < -hw)
			{
				ax = -hw;
			}
			lens(ax, ay, lx, ly, width, height, scale_x, scale_y);
		}
	}

	_window->pointerEvent(x, y, button_mask);
}

// ---------------------------------------------------------------------------
// Shaders

ShaderRenderer *WindowRenderer::_getCurrentShaderRenderer(void)
{
	std::list<ShaderRenderer *>::iterator it_shader = _shaders.begin();
	if (it_shader != _shaders.end())
	{
		return (*it_shader);
	}
	return 0;
}

bool WindowRenderer::removeShader(glShader *shader)
{
	std::list<ShaderRenderer *>::iterator iter;
	for (iter = _shaders.begin(); iter != _shaders.end(); iter++)
	{
		if (shader == (*iter)->shader)
		{
			ShaderRenderer *sr = (*iter);
			_shaders.remove(sr);
			//std::cerr << "Shader RM " << sr->shader << "\n";
			delete (sr);
			postRedisplay();
			return true;
		}
	}
	return false;
}

void WindowRenderer::removeAllShaders(void)
{
	std::list<ShaderRenderer *>::iterator iter;
	for (iter = _shaders.begin(); iter != _shaders.end(); iter++)
	{
		ShaderRenderer *sr = (*iter);
		delete (sr);
	}
	_shaders.clear();
	postRedisplay();
}

// FIXME: what to do if a "proressive shader is in work"?
// for now nothing!

void WindowRenderer::addShader(
	glShader *shader, ShaderRendererOpt *sro, bool back)
{
	std::list<ShaderRenderer *>::iterator iter;
	for (iter = _shaders.begin(); iter != _shaders.end(); iter++)
	{
		if (shader == (*iter)->shader)
		{
			return;
		}
	}
	ShaderRenderer *nsr = new ShaderRenderer(shader, sro);
	if (back)
	{
		_shaders.push_back(nsr);
	}
	else
	{
		_shaders.push_front(nsr);
	}
	#if 0
	std::cerr << "Shader Added Anim Shader " << nsr->shader << " "
		  << nsr->opt->nbr_anim_step << " " << nsr->opt->anim_total_time << " "
		  << nsr->opt->type << " " << nsr->opt->life_time << " "
		  << "\n";
	#endif
	if (nsr->opt->type & SHADER_RENDERER_ANIMATE)
	{
		if (_shaderTk == 0)
		{
			_shaderTk = TimeKeeper::create();
		}
		else
		{
			_shaderTk->disarm();
		}
		_shaderTk->arm(
			nsr->opt->anim_total_time/nsr->opt->nbr_anim_step);
		nsr->current_step = 0;
		subscribeTo(_shaderTk);
	}
	postRedisplay();
}


void WindowRenderer::replaceShader(glShader *rmshader, glShader *addshader)
{
	bool found = false;
	std::list<ShaderRenderer *>::iterator iter;

	for (iter = _shaders.begin(); iter != _shaders.end(); iter++)
	{
		if (rmshader == (*iter)->shader)
		{
			found = true;
			break;
		}
	}
	if (found)
	{
		(*iter)->shader = addshader;
		postRedisplay();
	}
}

void WindowRenderer::animShader(bool back)
{
	ShaderRenderer *sr = _getCurrentShaderRenderer();
	if (sr == 0 || !(sr->opt->type & SHADER_RENDERER_ANIMATE))
	{
		return;
	}

	sr->back = back;
	if (_shaderTk == 0)
	{
		_shaderTk = TimeKeeper::create();
	}
	else
	{
		_shaderTk->disarm();
		unsubscribeFrom(_shaderTk);
	}
	_shaderTk->arm(sr->opt->anim_total_time/sr->opt->nbr_anim_step);
	subscribeTo(_shaderTk);
	postRedisplay();
}


// ---------------------------------------------------------------------------

WindowRenderer::WindowRenderer(
	AScreen *ascr, MetisseWindow *window) : sgNode("WindowRenderer")
{
	_ascr = ascr;
	_window = window;
	_shadow = shadowByDefault;

	_shape = NULL;
	_nShapeRects = 0;
	_cutShape = NULL;
	_nCutShapeRects = 0;
	_canUseStencilBuffer = false;

	_winRegions = new WinRegions();
	_selectingRegion = false;

	_markingFeedback = false;

	_cropToCircle = false;
	_testPolygon = false;
	_animTk = 0;
	_foldTk = 0;
	_geneTk = 0;
	_shaderTk = 0;
	_root = 0;
	_scaleBack = false;
	_transBack = false;
	_backStep = 0;
	_inResetOrSet = false;
	_inIconification = false;
	_scheduleHide = false;
	_doFixCursor = false;

	_do_x_snapping = _do_y_snapping = false;
	_do_lens = false;
	_do_blens = false;
	_in_pm_lens = false;

	_auto_lens_type = LENS_TYPE_NONE;
	_lens_vert_w = 60;
	_lens_horiz_h = 30;
	_lens_wh_w = 80;
	_lens_wh_h = 30;
	_menu_lens = 0;
	
	_mp_in = false;
	_mp_warp = false;

	memcpy(_transMark, Identity, sizeof(GLfloat)*18);

	_reset();
}

WindowRenderer::~WindowRenderer(void)
{
	if (_shape)
	{
		free(_shape);
	}
	if (_cutShape)
	{
		free(_cutShape);
	}

	delete _winRegions;
	_regionSet.clear();

	if (_animTk)
	{
		delete _animTk;
	}
	if (_foldTk)
	{
		delete _foldTk;
	}
	if (_geneTk)
	{
		delete _geneTk;
	}
	if (_shaderTk)
	{
		delete _shaderTk;
	}
  }
