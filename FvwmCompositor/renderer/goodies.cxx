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

#include "goodies.H"

#include <math.h>

#include "fvwmmodule/AFvwm.H"
#include "FoldablePolygon.H"
#include "desktop/MetisseWindow.H"

void goodies_marcking_feedback(
	GLfloat x, GLfloat y, int dist, int draw_dist, int dirs, GLfloat alpha)
{
	glColor4f(1,1,1, 1);

	glBegin(GL_LINES);
	GLfloat md = draw_dist;
	GLfloat rd = dist;
	bool draw_rect = false;

	if (dirs & MULTI_DIR_W)
	{
		glColor4f(1,1,1,1);
		glVertex2d(x - md, y); glVertex2d(x, y);
		glColor4f(0,0,0,1);
		glVertex2d(x - md, y + 1);
		glVertex2d(x, y + 1);
		glVertex2d(x - md, y - 1);
		glVertex2d(x, y - 1);
		draw_rect = true;
	}
	if (dirs & MULTI_DIR_NW)
	{
		glColor4f(1,1,1,1);
		glVertex2d(x - md, y + md); glVertex2d(x, y);
		glColor4f(0,0,0,1);
		glVertex2d(x - md, y + md + 1);
		glVertex2d(x, y + 1);
		glVertex2d(x - md, y + md - 1);
		glVertex2d(x, y - 1);
		draw_rect = true;
	}
	if (dirs & MULTI_DIR_N)
	{
		glColor4f(1,1,1,1);
		glVertex2d(x, y + md); glVertex2d(x, y);
		glColor4f(0,0,0,1);
		glVertex2d(x-1, y + md); glVertex2d(x-1, y);
		glVertex2d(x+1, y + md); glVertex2d(x+1, y);
		draw_rect = true;
	}
	if (dirs & MULTI_DIR_NE)
	{
		glColor4f(1,1,1,1);
		glVertex2d(x + md, y + md); glVertex2d(x, y);
		glColor4f(0,0,0,1);
		glVertex2d(x + md, y + md - 1);
		glVertex2d(x, y - 1);
		glVertex2d(x + md, y + md + 1);
		glVertex2d(x, y + 1);
		draw_rect = true;
	}
	if (dirs & MULTI_DIR_E)
	{
		glColor4f(1,1,1,1);
		glVertex2d(x + md, y); glVertex2d(x, y);
		glColor4f(0,0,0,1);
		glVertex2d(x + md, y + 1);
		glVertex2d(x, y + 1);
		glVertex2d(x + md, y - 1);
		glVertex2d(x, y - 1);
		draw_rect = true;
	}
	if (dirs & MULTI_DIR_SE)
	{
		glColor4f(1,1,1,1);
		glVertex2d(x + md, y - md); glVertex2d(x, y);
		glColor4f(0,0,0,1);
		glVertex2d(x + md, y - md - 1); 
		glVertex2d(x, y - 1);
		glVertex2d(x + md, y - md + 1);
		glVertex2d(x, y + 1);
		draw_rect = true;
	}
	if (dirs & MULTI_DIR_S)
	{
		glColor4f(1,1,1,1);
		glVertex2d(x, y - md); glVertex2d(x, y);
		glColor4f(0,0,0,1);
		glVertex2d(x + 1, y - md);
		glVertex2d(x + 1, y);
		glVertex2d(x - 1, y - md);
		glVertex2d(x - 1, y);
		draw_rect = true;
	}
	if (dirs & MULTI_DIR_SE)
	{
		glColor4f(1,1,1,1);
		glVertex2d(x - md, y - md); glVertex2d(x, y);
		glColor4f(0,0,0,1);
		glVertex2d(x - md, y - md - 1);
		glVertex2d(x, y - 1);
		glVertex2d(x - md, y - md + 1);
		glVertex2d(x, y + 1);
		draw_rect = true;
	}
	glEnd();

	if (draw_rect)
	{
		GLdouble x1,y1,x2,y2;
		x1 = x - rd + 1;
		y1 = y + rd - 1;
		x2 = x + rd - 1;
		y2 = y - rd+ 1;
		
		glColor4f(1,0,0, 0.7);
		glRectf(x - rd + 1, y + rd - 1, x + rd - 1, y - rd+ 1);
		glColor4f(1,1,1, alpha);
		glBegin(GL_LINE_LOOP);
		glVertex2d(x1,y1);
		glVertex2d(x2,y1);
		glVertex2d(x2,y2);
		glVertex2d(x1,y2);
		glEnd();
	}
}

void goodies_shadow(
	FoldablePolygon fpoly, float startAlpha, float foldAlpha, 
	GLfloat shborder)
{
	std::list<glTiledTexturedImage::point>::iterator i;
	std::list<glTiledTexturedImage::point>::iterator j;
	std::list<glTiledTexturedImage::point>::iterator k;
	std::list<glTiledTexturedImage::point>::iterator l;
	std::list<glTiledTexturedImage::point>::iterator m;

	double sx,sy,cx,cy,rx,ry;
	double rborder = shborder/sqrt(2);
	double ralpha = startAlpha;

	for (i=fpoly.poly1.begin(); i!=fpoly.poly1.end(); ++i)
	{
		j = i;
		j++;
		if (j == fpoly.poly1.end())
		{
			j=fpoly.poly1.begin();
		}
		k = j;
		k++;
		if (k == fpoly.poly1.end())
		{
			k=fpoly.poly1.begin();
		}
			
		sx = sy = cx = cy = rx = ry = 0;

		if ((*i).x < (*j).x)
		{
			sy = -shborder;
		}
		else if ((*i).x > (*j).x)
		{
			sy = shborder;
		}
		if ((*i).y < (*j).y)
		{
			sx = shborder;
		}
		else if ((*i).y > (*j).y)
		{
			sx = - shborder;
		}

		if (sx != 0 && sy != 0)
		{
			sx = sx/sqrt(2);
			sy = sy/sqrt(2);
		}

		if ((*j).x < (*k).x)
		{
			cy = -shborder;
			ry = -rborder;
		}
		else if ((*j).x > (*k).x)
		{
			cy = shborder;
			ry = rborder;
		}
		if ((*j).y < (*k).y)
		{
			cx = shborder;
			rx = rborder;
		}
		else if ((*j).y > (*k).y)
		{
			cx = - shborder;
			rx = - rborder;
		}

		if (cx != 0 && cy != 0)
		{
			cx = cx/sqrt(2);
			cy = cy/sqrt(2);
		}

		double ux = ((sx>0)? 1:0)+((cx>0)? 1:0);
		double uy = ((sy>0)? 1:0)+((cy>0)? 1:0);

		ux = (ux > 0)? ux:1;
		uy = (uy > 0)? uy:1;
		rx = (sx+cx)/(ux*sqrt(2));
		ry = (sy+cy)/(uy*sqrt(2));

		glBegin(GL_POLYGON);
		glColor4f(0.1,0.1,0.1,startAlpha);
		glVertex2f((*i).x, (*i).y);
		glVertex2f((*j).x, (*j).y);
		glColor4f(0.1,0.1,0.1,0);
		glVertex2f((*j).x+sx, (*j).y+sy);
		glVertex2f((*i).x+sx, (*i).y+sy);
		glEnd();

		glBegin(GL_POLYGON);
		glColor4f(0.1,0.1,0.1,ralpha);
		glVertex2f((*j).x, (*j).y);
		glColor4f(0.1,0.1,0.1,0);
		glVertex2f((*j).x+sx, (*j).y+sy);
		glVertex2f((*j).x+rx, (*j).y+ry);
		glVertex2f((*j).x+cx, (*j).y+cy);
		glEnd();
	}

	glTranslatef(0,0,1);
	// FIXME: not perfect yet!!
	for (i = fpoly.shadow.begin(), l = fpoly.poly2.begin();
	     i != fpoly.shadow.end() && l != fpoly.poly2.end(); i++,l++)
	{
		j = i;
		j++;
		if (j == fpoly.shadow.end())
		{
			j = fpoly.shadow.begin();
		}
		
		m = l;
		m++;
		if (m == fpoly.poly2.end())
		{
			m = fpoly.poly2.begin();
		}

		glBegin(GL_POLYGON);
		glColor4f(0.1,0.1,0.1,foldAlpha);
		glVertex2f((*l).x, (*l).y);
		glVertex2f((*m).x, (*m).y);
		glColor4f(0.1,0.1,0.1,0);
		if ((*j).x != (*m).x ||  (*j).y != (*m).y)
		{
			glVertex2f((*j).x, (*j).y);
		}
		if ((*i).x != (*l).x ||  (*i).y != (*l).y)
		{
			glVertex2f((*i).x, (*i).y);
		}
		glEnd();
#if 0
		glBegin(GL_LINE_LOOP);
		glColor4f(0.1,0.1,0.1,1);
		glVertex2f((*l).x, (*l).y);
		glVertex2f((*m).x, (*m).y);
		glColor4f(0.1,0.1,0.1,1);
		if ((*j).x != (*m).x ||  (*j).y != (*m).y)
		{
			glVertex2f((*j).x, (*j).y);
		}
		if ((*i).x != (*l).x ||  (*i).y != (*l).y)
		{
			glVertex2f((*i).x, (*i).y);
		}
		glEnd();
#endif
	}
}

void goodies_shadow(
	GLfloat hw, GLfloat hh, float startAlpha, GLfloat shborder)
{

	hw=(int)(hw/2.0); hh=(int)(hh/2.0);
	GLboolean isBlendEnabled;

	glGetBooleanv(GL_BLEND, &isBlendEnabled);
	if (!isBlendEnabled)
	{
		glEnable(GL_BLEND);
	}

	static GLUquadricObj *_qobj =  gluNewQuadric();

	gluQuadricDrawStyle(_qobj, GLU_FILL);
	gluQuadricNormals(_qobj, GLU_NONE);

#if 0
	glColor4f(0.1,0.1,0.1, startAlpha);
	glPushMatrix();
	glTranslatef(0,0,-0.05);
	glRectf(-hw+shborder,-hh-shborder, hw+shborder,hh-shborder);
	glPopMatrix();

#else

      //#define LEFTRIGHT_SHADOW

#ifdef LEFTRIGHT_SHADOW
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(-hw+shborder, -hh);
      glVertex2f(hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw, -hh-shborder);
      glVertex2f(-hw+shborder, -hh-shborder);
      glEnd();

      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw+shborder, -hh);
      glVertex2f(hw+shborder, -hh-shborder);
      glVertex2f(hw, -hh-shborder);
      glEnd();

      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(hw, -hh);
      glVertex2f(hw, hh-shborder);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw+shborder, hh-shborder);
      glVertex2f(hw+shborder, -hh);
      glEnd();
#elif 0
      // top
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(-hw, hh);
      glVertex2f(hw, hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(+hw+shborder, hh+shborder);
      glVertex2f(-hw-shborder, hh+shborder);
      glEnd();

      // bottom
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(-hw, -hh);
      glVertex2f(hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw+shborder, -hh-shborder);
      glVertex2f(-hw-shborder, -hh-shborder);
      glEnd();

      // right
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(hw, -hh);
      glVertex2f(hw, hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw+shborder, hh+shborder);
      glVertex2f(hw+shborder, -hh-shborder);
      glEnd(); 

      // left 
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(-hw, hh);
      glVertex2f(-hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(-hw-shborder, -hh-shborder);
      glVertex2f(-hw-shborder, +hh+shborder);
      glEnd();
#elif 0
      // top
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(-hw, hh);
      glVertex2f(hw, hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(+hw, hh+shborder);
      glVertex2f(-hw, hh+shborder);
      glEnd();

      // top left 
      glTranslatef(-hw, hh, 0);
      glColor4f(0.1,0.1,0.1, startAlpha/2.2);
      gluPartialDisk(_qobj, 0.0, shborder/1.4, 20, 1, 270, 90);
      glTranslatef(hw, -hh, -0);

      // top right 
      glTranslatef(hw, hh, 0);
      glColor4f(0.1,0.1,0.1, startAlpha/2.2);
      gluPartialDisk(_qobj, 0.0, shborder/1.4, 20,  1, 0, 90);
      glTranslatef(-hw, -hh, -0);

      // bottom
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1, startAlpha);
      glVertex2f(-hw, -hh);
      glVertex2f(hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw, -hh-shborder);
      glVertex2f(-hw, -hh-shborder);
      glEnd();

      // bottom left 
      glTranslatef(-hw, -hh, 0);
      glColor4f(0.1,0.1,0.1, startAlpha/2.2);
      gluPartialDisk(_qobj, 0.0, shborder/1.4, 20,  1, 180, 90);
      glTranslatef(hw, hh, -0);

      // bottom right 
      glTranslatef(hw, -hh, 0);
      glColor4f(0.1,0.1,0.1, startAlpha/2.2);
      gluPartialDisk(_qobj, 0.0, shborder/1.4, 20,  1, 90, 90);
      glTranslatef(-hw, hh, -0);

      // right
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(hw, -hh);
      glVertex2f(hw, hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw+shborder, hh);
      glVertex2f(hw+shborder, -hh);
      glEnd(); 

      // left 
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(-hw, hh);
      glVertex2f(-hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(-hw-shborder, -hh);
      glVertex2f(-hw-shborder, +hh);
      glEnd(); 

#elif 1

      double rborder = shborder/sqrt(2);
      double ralpha = startAlpha; //startAlpha/sqrt(2);

      // top
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(-hw, hh);
      glVertex2f(hw, hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(+hw, hh+shborder);
      glVertex2f(-hw, hh+shborder);
      glEnd();

      // top left 
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,ralpha);
      glVertex2f(-hw, hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(-hw-shborder, hh);
      glVertex2f(-hw-rborder , hh+rborder);
      glVertex2f(-hw, hh+shborder);
      glEnd();

      // top right 
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,ralpha);
      glVertex2f(hw, hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw+shborder, hh);
      glVertex2f(hw+rborder, hh+rborder);
      glVertex2f(hw, hh+shborder);
      glEnd();

      // bottom
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1, startAlpha);
      glVertex2f(-hw, -hh);
      glVertex2f(hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw, -hh-shborder);
      glVertex2f(-hw, -hh-shborder);
      glEnd();

      // bottom left 
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,ralpha);
      glVertex2f(-hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(-hw-shborder, -hh);
      glVertex2f(-hw-rborder, -hh-rborder);
      glVertex2f(-hw, -hh-shborder);
      glEnd();

      // bottom right 
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,ralpha);
      glVertex2f(hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw+shborder, -hh);
      glVertex2f(hw+rborder, -hh-rborder);
      glVertex2f(hw, -hh-shborder);
      glEnd();


      // right
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(hw, -hh);
      glVertex2f(hw, hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(hw+shborder, hh);
      glVertex2f(hw+shborder, -hh);
      glEnd(); 

      // left 
      glBegin(GL_POLYGON);
      glColor4f(0.1,0.1,0.1,startAlpha);
      glVertex2f(-hw, hh);
      glVertex2f(-hw, -hh);
      glColor4f(0.1,0.1,0.1,0);
      glVertex2f(-hw-shborder, -hh);
      glVertex2f(-hw-shborder, +hh);
      glEnd();  
#endif

#endif
      if (!isBlendEnabled)
      {
	      glDisable(GL_BLEND);
      }
}

void shadow_line(
	GLfloat start_x, GLfloat start_y, GLfloat end_x, GLfloat end_y,
	GLfloat startAlpha, GLfloat width)
{
	glBegin(GL_POLYGON);
	glColor4f(0.1,0.1,0.1,startAlpha);
	glVertex2f(start_x, start_y);
	glVertex2f(end_x, end_y);
	glColor4f(0.1,0.1,0.1,0);
	glVertex2f(end_x, end_y-width);
	glVertex2f(start_x, end_y-width);
	glEnd();
	#if 1
	glBegin(GL_POLYGON);
	glColor4f(0.1,0.1,0.1,startAlpha);
	glVertex2f(start_x, start_y);
	glVertex2f(end_x, end_y);
	glColor4f(0.1,0.1,0.1,0);
	glVertex2f(end_x, end_y+width);
	glVertex2f(start_x, start_y+width);
	glEnd();
	#endif
}

void setup_circle_deformation(
	GLfloat hw, GLfloat hh,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points)
{
	double a,b;
	polygon->push_back(
		glTiledTexturedImage::point(0.01,0.01));
	img_points->push_back(
		glTiledTexturedImage::point(0.01,0.01));
	for (b=0; b <= 40; b++)
	{
		//double x=(hw-1.2)*cos(a), y=(hh-1.2)*sin(a);
		a = b*M_PI/20;
		double x=hw*cos(a), y=hh*sin(a);
		polygon->push_back(
			glTiledTexturedImage::point(x,y));
		double xi,yi;
		if (0 <= a && a <= M_PI/4)
		{
			xi = hw;
			yi = tan(a)*hh;
		}
		else if (M_PI/4 <= a && a <= 3*M_PI/4)
		{
			xi = (cos(a)/sin(a))*hw;
			yi = hh;
		}
		else if (3*M_PI/4 <= a && a <= 5*M_PI/4)
		{
			xi = - hw;
			yi = - tan(a)*hh;
		}
		else if (5*M_PI/4 <= a && a <= 7*M_PI/4)
		{
			xi = - (cos(a)/sin(a))*hw;
			yi = - hh;
		}
		else if (7*M_PI/4 <= a && a <= 2*M_PI)
		{
			xi = hw;
			yi = tan(a)*hh;
		}
		else
		{
			fprintf(stderr,"ERROR ");
		}
		img_points->push_back(
			glTiledTexturedImage::point(xi,yi));
	}
}
void setup_snapping_deformation(
	bool do_x_snapping, bool do_y_snapping,
	bool snap_top, bool snap_right, GLfloat snap_xoff, GLfloat snap_yoff,
	GLfloat d, GLfloat sad, GLfloat hw, GLfloat hh,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points)  
{
	if (d < 5) d=5;

	GLfloat inter_lr = 2*sad;
	GLfloat inter_tb = 2*sad;

	if (inter_lr > hw)
	{
		inter_lr = hw - 0.1;
	}
	if (inter_tb > hh)
	{
		inter_tb = hh - 0.1;
	}
	//d = fmin(fmin(d,hw-0.1), fmin(d,hh-0.1));
	d = fmin(inter_tb,inter_lr)/1.5;

	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, -hh + inter_tb));
	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, -hh + inter_tb));
	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, hh - inter_tb));
	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, -hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, -hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, hh - inter_tb));
	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));

	// snap y bottom
	GLfloat snap = 0;
	if (do_y_snapping && !snap_top)
	{
		snap = snap_yoff;
	}
			
		
	// 1
	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr,-hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr,-hh + inter_tb));
		
	polygon->push_back(glTiledTexturedImage::point(-hw,-hh));
	img_points->push_back(glTiledTexturedImage::point(-hw,-hh));

	polygon->push_back(
		glTiledTexturedImage::point(-hw+1.0*d, - hh - snap));
	img_points->push_back(
		glTiledTexturedImage::point(-hw+1.0*d, -hh));

	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));

	// 2
	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr,-hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr,-hh + inter_tb));

	polygon->push_back(
		glTiledTexturedImage::point(-hw+1.0*d, - hh - snap));
	img_points->push_back(
		glTiledTexturedImage::point(-hw+1.0*d, -hh));

	polygon->push_back(
		glTiledTexturedImage::point(hw-1.0*d, -hh - snap));
	img_points->push_back(
		glTiledTexturedImage::point(hw-1.0*d, -hh));
		
	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr,-hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr,-hh + inter_tb));

	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));

	// 3
	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr,-hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr,-hh + inter_tb));
		
	polygon->push_back(
		glTiledTexturedImage::point(hw-1.0*d, -hh - snap));
	img_points->push_back(
		glTiledTexturedImage::point(hw-1.0*d, -hh));

	polygon->push_back(glTiledTexturedImage::point(hw,-hh));
	img_points->push_back(glTiledTexturedImage::point(hw,-hh));

	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));

	// snap x right
	snap = 0;
	if (do_x_snapping && snap_right)
	{
		snap = snap_xoff;
	}

	// 1
	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, -hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, -hh + inter_tb));

	polygon->push_back(glTiledTexturedImage::point(hw, -hh));
	img_points->push_back(glTiledTexturedImage::point(hw, -hh));

	polygon->push_back(
		glTiledTexturedImage::point(hw + snap, -hh+1.0*d));
	img_points->push_back(
		glTiledTexturedImage::point(hw, -hh+1.0*d));

	polygon->push_back(
		glTiledTexturedImage::point(0,0));
	img_points->push_back(
		glTiledTexturedImage::point(0,0));
		
	// 2
	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, -hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, -hh + inter_tb));
		
	polygon->push_back(
		glTiledTexturedImage::point(hw + snap, -hh+1.0*d));
	img_points->push_back(
		glTiledTexturedImage::point(hw, -hh+1.0*d));

	polygon->push_back(
		glTiledTexturedImage::point(hw + snap, hh - 1.0*d));
	img_points->push_back(
		glTiledTexturedImage::point(hw, hh-1.0*d));

	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, hh - inter_tb));
			
	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));

	// 3
	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, hh - inter_tb));

	polygon->push_back(
		glTiledTexturedImage::point(hw + snap, hh - 1.0*d));
	img_points->push_back(
		glTiledTexturedImage::point(hw, hh-1.0*d));

		
	polygon->push_back(glTiledTexturedImage::point(hw, hh));
	img_points->push_back(glTiledTexturedImage::point(hw, hh));

	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));
		
	// snap y top
	snap = 0;
	if (do_y_snapping && snap_top)
	{
		snap = snap_yoff;
	}

	// 1
	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr, hh - inter_tb));
		
	polygon->push_back(glTiledTexturedImage::point(hw,hh));
	img_points->push_back(glTiledTexturedImage::point(hw,hh));

	polygon->push_back(
		glTiledTexturedImage::point(hw-1.0*d, hh - snap));
	img_points->push_back(
		glTiledTexturedImage::point(hw-1.0*d, hh));

	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));

	// 2
	polygon->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr,hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			hw - inter_lr,hh - inter_tb));

	polygon->push_back(
		glTiledTexturedImage::point(hw-1.0*d, hh - snap));
	img_points->push_back(
		glTiledTexturedImage::point(hw-1.0*d, hh));

	polygon->push_back(
		glTiledTexturedImage::point(-hw+1.0*d, hh - snap));
	img_points->push_back(
		glTiledTexturedImage::point(-hw+1.0*d, hh));
		
	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr,hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, hh - inter_tb));

	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));

	// 3
	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr,hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, hh - inter_tb));
		
	polygon->push_back(
		glTiledTexturedImage::point(-hw+1.0*d, hh - snap));
	img_points->push_back(
		glTiledTexturedImage::point(-hw+1.0*d, hh));

	polygon->push_back(glTiledTexturedImage::point(-hw, hh));
	img_points->push_back(glTiledTexturedImage::point(-hw, hh));

	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));

	// left snap
	snap = 0;
	if (do_x_snapping && !snap_right)
	{
		snap = snap_xoff;
	}

	// 1
	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, hh - inter_tb));

	polygon->push_back(glTiledTexturedImage::point(-hw, hh));
	img_points->push_back(glTiledTexturedImage::point(-hw, hh));

	polygon->push_back(
		glTiledTexturedImage::point(-hw + snap, +hh-1.0*d));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, hh-1.0*d));

	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));
		
	// 2
	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, hh - inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, hh - inter_tb));
		
	polygon->push_back(
		glTiledTexturedImage::point(-hw + snap, hh-1.0*d));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, hh-1.0*d));

	polygon->push_back(
		glTiledTexturedImage::point(-hw + snap, -hh + 1.0*d));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, -hh+1.0*d));

	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, -hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, -hh + inter_tb));
			
	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));

	// 3
	polygon->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, -hh + inter_tb));
	img_points->push_back(
		glTiledTexturedImage::point(
			-hw + inter_lr, -hh + inter_tb));

	polygon->push_back(
		glTiledTexturedImage::point(-hw + snap, -hh + 1.0*d));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, -hh+1.0*d));
		
	polygon->push_back(glTiledTexturedImage::point(-hw, -hh));
	img_points->push_back(glTiledTexturedImage::point(-hw, -hh));

	polygon->push_back(glTiledTexturedImage::point(0,0));
	img_points->push_back(glTiledTexturedImage::point(0,0));
}

void setup_lens_deformation(
	int autolens_type, int cnum, bool do_blens, GLfloat hw, GLfloat hh,
	GLfloat lens_ax, GLfloat lens_ay, GLfloat lens_x, GLfloat lens_y,
	GLfloat lens_w, GLfloat lens_h, GLfloat lens_sx, GLfloat lens_sy,
	GLfloat blens_ax, GLfloat blens_ay, GLfloat blens_x, GLfloat blens_y,
	GLfloat blens_w, GLfloat blens_h, GLfloat blens_sx, GLfloat blens_sy,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points)
{
	GLfloat sw = lens_w * lens_sx;
	GLfloat sh = lens_h * lens_sy;

	GLfloat ax1 = lens_ax;
	GLfloat ay1 = lens_ay;
	GLfloat ax2 = ax1 + sw;
	GLfloat ay2 = ay1 + sh;

	GLfloat x1 = lens_x;
	GLfloat x2 = lens_x + lens_w;
	GLfloat y1 = lens_y;
	GLfloat y2 = lens_y + lens_h;

	GLfloat bsw = blens_w * blens_sx;
	GLfloat bsh = blens_h * blens_sy;

	GLfloat bax1 = blens_ax;
	GLfloat bay1 = blens_ay;
	GLfloat bax2 = bax1 + bsw;
	GLfloat bay2 = bay1 + bsh;

	GLfloat bx1 = blens_x;
	GLfloat bx2 = blens_x + blens_w;
	GLfloat by1 = blens_y;
	GLfloat by2 = blens_y + blens_h;
		
	int lens_type = autolens_type;
	if (lens_type == LENS_TYPE_NONE)
	{
		lens_type = LENS_TYPE_CENTER;
	}

#if 0
	fprintf(stderr, "%f/%f//%f/%f  %f/%f//%f/%f  %f/%f//%f/%f "
		"%f/%f//%f/%f (%f %f %i)\n",
		ax1,ay1,x1,y1, ax2,ay1,x2,y1,  ax2,ay2,x2,y2,
		ax1,ay2,x1,y2, hh,hw,do_blens);
#endif

	// first draw the lens rectangle
	polygon->push_back(
		glTiledTexturedImage::point(ax1, ay1));
	img_points->push_back(
		glTiledTexturedImage::point(x1, y1));

	polygon->push_back(
		glTiledTexturedImage::point(ax2, ay1));
	img_points->push_back(
		glTiledTexturedImage::point(x2, y1));

	polygon->push_back(
		glTiledTexturedImage::point(ax2, ay2));
	img_points->push_back(
		glTiledTexturedImage::point(x2, y2));

	polygon->push_back(
		glTiledTexturedImage::point(ax1, ay2));
	img_points->push_back(
		glTiledTexturedImage::point(x1, y2));

	polygon->push_back(glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(glTiledTexturedImage::point(0.0,0.0));
	if (do_blens)
	{
		polygon->push_back(
			glTiledTexturedImage::point(bax1, bay1));
		img_points->push_back(
			glTiledTexturedImage::point(bx1, by1));

		polygon->push_back(
			glTiledTexturedImage::point(bax2, bay1));
		img_points->push_back(
			glTiledTexturedImage::point(bx2, by1));

		polygon->push_back(
			glTiledTexturedImage::point(bax2, bay2));
		img_points->push_back(
			glTiledTexturedImage::point(bx2, by2));

		polygon->push_back(
			glTiledTexturedImage::point(bax1, bay2));
		img_points->push_back(
			glTiledTexturedImage::point(bx1, by2));
			
		polygon->push_back(
			glTiledTexturedImage::point(0.0,0.0));
		img_points->push_back(
			glTiledTexturedImage::point(0.0,0.0));
	}

	// left poly
	if (-hw < lens_x)
	{
		if (do_blens && by2 < hh)
		{
			// bottom lens too
			polygon->push_back(
				glTiledTexturedImage::point(bax1, bay2));
			img_points->push_back(
				glTiledTexturedImage::point(bx1, by2));
			polygon->push_back(
				glTiledTexturedImage::point(ax1, bay2));
			img_points->push_back(
				glTiledTexturedImage::point(x1, by2)); 
		}
		else if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(-hw, -hh));
			img_points->push_back(
				glTiledTexturedImage::point(-hw, -hh));
			polygon->push_back(
				glTiledTexturedImage::point(ax1, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(x1, y1));
		}

		if (lens_type ==  LENS_TYPE_CROSS)
		{
			polygon->push_back(
				glTiledTexturedImage::point(-hw, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(-hw, y1));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));

			polygon->push_back(
				glTiledTexturedImage::point(-hw, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(-hw, y1));
			polygon->push_back(
				glTiledTexturedImage::point(ax1, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(x1, y1));
		}
		else if (lens_type ==  LENS_TYPE_CENTER)
		{
			float bh = 2*hh/(float)cnum;
			float ah = sh/(float)cnum;
			float yh = lens_h/(float)cnum;
			//std::cerr << bw << " " << aw << " " <<
			//	xw << " " << "\n";
			for (int i = 0; i < cnum; i++)
			{
				polygon->push_back(
					glTiledTexturedImage::point(
						-hw, -hh + i*bh));
				img_points->push_back(
					glTiledTexturedImage::point(
						-hw, -hh + i*bh));
				polygon->push_back(
					glTiledTexturedImage::point(
						ax1, ay1 + i*ah));
				img_points->push_back(
					glTiledTexturedImage::point(
						x1, y1 + i*yh));

				// t
				polygon->push_back(
					glTiledTexturedImage::point(
						-hw, -hh + (i+1)*bh));
				img_points->push_back(
					glTiledTexturedImage::point(
						-hw, -hh + (i+1)*bh));
				polygon->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
				img_points->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
				polygon->push_back(
					glTiledTexturedImage::point(
						-hw, -hh + (i+1)*bh));
				img_points->push_back(
					glTiledTexturedImage::point(
						-hw, -hh + (i+1)*bh));
				polygon->push_back(
					glTiledTexturedImage::point(
						ax1, ay1 + i*ah));
				img_points->push_back(
					glTiledTexturedImage::point(
						x1, y1 + i*yh));
				//c
				polygon->push_back(
					glTiledTexturedImage::point(
						ax1, ay1 + (i+1)*ah));
				img_points->push_back(
					glTiledTexturedImage::point(
						x1, y1 + (i+1)*yh));
				//-t
				//polygon->push_back(
				//	glTiledTexturedImage::point(
				//		-hw, -hh + (i+1)*bh));
				//img_points->push_back(
				//	glTiledTexturedImage::point(
				//		-hw, -hh + (i+1)*bh));
				polygon->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
				img_points->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
			}
		}

		if (do_blens && -hh < by1)
		{
			// top lens too
			polygon->push_back(
				glTiledTexturedImage::point(ax1, bay1));
			img_points->push_back(
				glTiledTexturedImage::point(x1, by1));
		}
		else if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(ax1, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(x1, y2));
		}

		if (lens_type ==  LENS_TYPE_CROSS)
		{
			polygon->push_back(
				glTiledTexturedImage::point(-hw, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(-hw, y2));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));

			polygon->push_back(
				glTiledTexturedImage::point(-hw, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(-hw, y2));
			polygon->push_back(
				glTiledTexturedImage::point(ax1, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(x1, y2));
		}

		if (do_blens && -hh < by1)
		{
			// top lens too
			polygon->push_back(
				glTiledTexturedImage::point(bax1, bay1));
			img_points->push_back(
				glTiledTexturedImage::point(bx1, by1));
		}
		else  if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(-hw, hh));
			img_points->push_back(
				glTiledTexturedImage::point(-hw, hh));
		}
		if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));
		}
	}
		
	// bottom poly
	if (-hh < y1)
	{
		if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(-hw, -hh));
			img_points->push_back(
				glTiledTexturedImage::point(-hw, -hh));
			polygon->push_back(
				glTiledTexturedImage::point(ax1, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(x1, y1));
		}

		if (lens_type ==  LENS_TYPE_CROSS)
		{
			polygon->push_back(
				glTiledTexturedImage::point(ax1, -hh));
			img_points->push_back(
				glTiledTexturedImage::point(x1, -hh));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));


			polygon->push_back(
				glTiledTexturedImage::point(ax1, -hh));
			img_points->push_back(
				glTiledTexturedImage::point(x1, -hh));
				
			polygon->push_back(
				glTiledTexturedImage::point(ax2, -hh));
			img_points->push_back(
				glTiledTexturedImage::point(x2, -hh));
		}
		else if (lens_type ==  LENS_TYPE_CENTER)
		{
			float bw = 2*hw/(float)cnum;
			float aw = sw/(float)cnum;
			float xw = lens_w/(float)cnum;
			//std::cerr << bw << " " << aw << " " <<
			//	xw << " " << "\n";
			for (int i = 0; i < cnum; i++)
			{
				polygon->push_back(
					glTiledTexturedImage::point(
						-hw + i*bw, -hh));
				img_points->push_back(
					glTiledTexturedImage::point(
						-hw + i*bw, -hh));
				polygon->push_back(
					glTiledTexturedImage::point(
						ax1 + i*aw, ay1));
				img_points->push_back(
					glTiledTexturedImage::point(
						x1 + i*xw, y1));

				//t
				polygon->push_back(
					glTiledTexturedImage::point(
						-hw + (i+1)*bw, -hh));
				img_points->push_back(
					glTiledTexturedImage::point(
						-hw + (i+1)*bw, -hh));
				polygon->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
				img_points->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
				polygon->push_back(
					glTiledTexturedImage::point(
						-hw + (i+1)*bw, -hh));
				img_points->push_back(
					glTiledTexturedImage::point(
						-hw + (i+1)*bw, -hh));
				polygon->push_back(
					glTiledTexturedImage::point(
						ax1 + i*aw, ay1));
				img_points->push_back(
					glTiledTexturedImage::point(
						x1 + i*xw, y1));
				//c 
				polygon->push_back(
					glTiledTexturedImage::point(
						ax1 + (i+1)*aw, ay1));
				img_points->push_back(
					glTiledTexturedImage::point(
						x1 + (i+1)*xw, y1));
				//-t
				//polygon->push_back(
				//	glTiledTexturedImage::point(
				//		-hw + (i+1)*bw, -hh));
				//img_points->push_back(
				//glTiledTexturedImage::point(
				//		-hw + (i+1)*bw, -hh));

				polygon->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
				img_points->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));	
			} 
		}

		if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(ax2, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(x2, y1));
		}

		if (lens_type ==  LENS_TYPE_CROSS)
		{
			polygon->push_back(
				glTiledTexturedImage::point(ax1, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(x1, y1));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			polygon->push_back(
				glTiledTexturedImage::point(ax2, -hh));
			img_points->push_back(
				glTiledTexturedImage::point(x2, -hh));
			polygon->push_back(
				glTiledTexturedImage::point(ax2, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(x2, y1));
		}

		if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(hw, -hh));
			img_points->push_back(
				glTiledTexturedImage::point(hw, -hh));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));
		}
	}
		
	// right poly
	if (x2 < hw)
	{
		if (do_blens && by2 < hh)
		{
			// bottom lens too
			polygon->push_back(
				glTiledTexturedImage::point(
					ax2, bay2));
			img_points->push_back(
				glTiledTexturedImage::point(x2, by2));
		}
		else if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(ax2, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(x2, y1));
		}
		if (do_blens && by2 < hh)
		{
			// bottom lens too
			polygon->push_back(
				glTiledTexturedImage::point(bax2, bay2));
			img_points->push_back(
				glTiledTexturedImage::point(bx2, by2));
		}
		else if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(hw, -hh));
			img_points->push_back(
				glTiledTexturedImage::point(hw, -hh));
		}

		if (lens_type ==  LENS_TYPE_CROSS)
		{
			polygon->push_back(
				glTiledTexturedImage::point(hw, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(hw, y1));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));

			polygon->push_back(
				glTiledTexturedImage::point(hw, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(hw, y1));
			polygon->push_back(
				glTiledTexturedImage::point(ax2, ay1));
			img_points->push_back(
				glTiledTexturedImage::point(x2, y1));
			polygon->push_back(
				glTiledTexturedImage::point(ax2, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(x2, y2));
			polygon->push_back(
				glTiledTexturedImage::point(hw, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(hw, y2));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));
				
			polygon->push_back(
				glTiledTexturedImage::point(hw, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(hw, y2));
		}
		else if (lens_type ==  LENS_TYPE_CENTER)
		{
			float bh = 2*hh/(float)cnum;
			float ah = sh/(float)cnum;
			float yh = lens_h/(float)cnum;
			for (int i = 0; i < cnum; i++)
			{
				polygon->push_back(
					glTiledTexturedImage::point(
						hw, -hh + i*bh));
				img_points->push_back(
					glTiledTexturedImage::point(
						hw, -hh + i*bh));
				// FIXME
				polygon->push_back(
					glTiledTexturedImage::point(
						ax2, ay1 + i*ah));
				img_points->push_back(
					glTiledTexturedImage::point(
						x2, y1 + i*yh));
				polygon->push_back(
					glTiledTexturedImage::point(
						ax2, ay1 + (i+1)*ah));
				img_points->push_back(
					glTiledTexturedImage::point(
						x2, y1 + (i+1)*yh));
				polygon->push_back(
					glTiledTexturedImage::point(
						hw, -hh + (i+1)*bh));
				img_points->push_back(
					glTiledTexturedImage::point(
						hw, -hh + (i+1)*bh));
				polygon->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
				img_points->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
			}	
				
		}

		if (do_blens && -hh < by1)
		{
			// top lens too
			polygon->push_back(
				glTiledTexturedImage::point(bax2, bay1));
			img_points->push_back(
				glTiledTexturedImage::point(bx2, by1));
		}
		else if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(hw, hh));
			img_points->push_back(
				glTiledTexturedImage::point(hw, hh));
		}
		if (do_blens && -hh < by1)
		{
			// top lens too
			polygon->push_back(
				glTiledTexturedImage::point(
					ax2, bay1));
			img_points->push_back(
				glTiledTexturedImage::point(x2, by1));
		}
		else if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(ax2, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(x2, y2));
		}

		if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));
		}
	}

	// top
	if (y2 < hh)
	{
		if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(hw, hh));
			img_points->push_back(
				glTiledTexturedImage::point(hw, hh));
			polygon->push_back(
				glTiledTexturedImage::point(ax2, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(x2, y2));
		}

		if (lens_type ==  LENS_TYPE_CROSS)
		{
			polygon->push_back(
				glTiledTexturedImage::point(ax2, hh));
			img_points->push_back(
				glTiledTexturedImage::point(x2, hh));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));

			polygon->push_back(
				glTiledTexturedImage::point(ax2, hh));
			img_points->push_back(
				glTiledTexturedImage::point(x2, hh));
			polygon->push_back(
				glTiledTexturedImage::point(ax2, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(x2, y2));
		}
		else if (lens_type ==  LENS_TYPE_CENTER)
		{
			float bw = 2*hw/(float)cnum;
			float aw = sw/(float)cnum;
			float xw = lens_w/(float)cnum;
			for (int i = 0; i < cnum; i++)
			{
				polygon->push_back(
					glTiledTexturedImage::point(
						hw - i*bw, hh));
				img_points->push_back(
					glTiledTexturedImage::point(
						hw - i*bw, hh));
				polygon->push_back(
					glTiledTexturedImage::point(
						ax2 - i*aw, ay2));
				img_points->push_back(
					glTiledTexturedImage::point(
						x2 - i*xw, y2));
				polygon->push_back(
					glTiledTexturedImage::point(
						ax2 - (i+1)*aw,ay2));
				img_points->push_back(
					glTiledTexturedImage::point(
						x2 - (i+1)*xw, y2));
				polygon->push_back(
					glTiledTexturedImage::point(
						hw - (i+1)*bw, hh));
				img_points->push_back(
					glTiledTexturedImage::point(
						hw - (i+1)*bw, hh));
				polygon->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
				img_points->push_back(
					glTiledTexturedImage::point(
						0.0,0.0));
			}
		}

		if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(ax1, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(x1, y2));
		}

		if (lens_type ==  LENS_TYPE_CROSS)
		{
			polygon->push_back(
				glTiledTexturedImage::point(ax1, hh));
			img_points->push_back(
				glTiledTexturedImage::point(x1, hh));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));

			polygon->push_back(
				glTiledTexturedImage::point(ax1, hh));
			img_points->push_back(
				glTiledTexturedImage::point(x1, hh));
			polygon->push_back(
				glTiledTexturedImage::point(ax1, ay2));
			img_points->push_back(
				glTiledTexturedImage::point(x1, y2));
		}

		if (lens_type !=  LENS_TYPE_CENTER)
		{
			polygon->push_back(
				glTiledTexturedImage::point(-hw, hh));
			img_points->push_back(
				glTiledTexturedImage::point(-hw, hh));
			polygon->push_back(
				glTiledTexturedImage::point(0.0,0.0));
			img_points->push_back(
				glTiledTexturedImage::point(0.0,0.0));
		}
	}

}

static
void setup_swallowed_deformation_left_cut_right(
	MetisseWindow *win,  MetisseWindow *s_win,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points)
{
	GLfloat w,h,hw,hh,w_rx,w_ry;
	win->getSize(&w,&h);
	win->getRealPosition(&w_rx, &w_ry);
	hw = w/2.0;
	hh = h/2.0;

	float sw_w, sw_h, sw_hw, sw_hh, sw_rx, sw_ry;
	s_win->getRealPosition(&sw_rx, &sw_ry);
	s_win->getSize(&sw_w,&sw_h);

	GLfloat ix,iy,iw,ih;
	ix = fmax(sw_rx,w_rx);
	iy = fmax(sw_ry,w_ry);
	iw = fmin(sw_rx + sw_w,w_rx + w) - fmax(sw_rx,w_rx);
	ih = fmin(sw_ry + sw_h,w_ry + h) - fmax(sw_ry,w_ry);
	sw_w = iw;
	sw_h = ih;
	sw_hw = iw/2.0;
	sw_hh = ih/2.0;
	sw_rx = ix; 
	sw_ry = iy;

	sw_hw = sw_w/2.0;
	sw_hh = sw_h/2.0;

	GLfloat lax, laxx, layy, lay, lx, ly;

	// center
	lx = ((sw_rx+sw_hw)) - hw;
	ly = hh - (((sw_ry+sw_hh)));
	// top left of the win
	lax = lx - sw_hw;
	lay = ly + sw_hh;
	// bottom right of the win
	laxx = lx + sw_hw;
	layy = ly - sw_hh;

	GLfloat ret_x,ret_y;
	ret_x = sw_hw;
	ret_y = sw_hh;

	// left
	polygon->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax, hh));
	img_points->push_back(
		glTiledTexturedImage::point(lx+ret_x, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(lx+ret_x, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// right
	polygon->push_back(
		glTiledTexturedImage::point(laxx, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(lx-ret_x, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx, hh));
	img_points->push_back(
		glTiledTexturedImage::point(lx-ret_x, hh));
	polygon->push_back(
		glTiledTexturedImage::point(hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// FIXME: add sission lines
}

static
void setup_swallowed_deformation_top_cut_bottom(
	MetisseWindow *win,  MetisseWindow *s_win,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points)
{
	GLfloat w,h,hw,hh,w_rx,w_ry;
	win->getSize(&w,&h);
	win->getRealPosition(&w_rx, &w_ry);
	hw = w/2.0;
	hh = h/2.0;

	float sw_w, sw_h, sw_hw, sw_hh, sw_rx, sw_ry, sw_x, sw_y;
	s_win->getRealPosition(&sw_rx, &sw_ry);
	s_win->getSize(&sw_w,&sw_h);

	GLfloat ix,iy,iw,ih;
	ix = fmax(sw_rx,w_rx);
	iy = fmax(sw_ry,w_ry);
	iw = fmin(sw_rx + sw_w,w_rx + w) - fmax(sw_rx,w_rx);
	ih = fmin(sw_ry + sw_h,w_ry + h) - fmax(sw_ry,w_ry);
	sw_w = iw;
	sw_h = ih;
	sw_hw = iw/2.0;
	sw_hh = ih/2.0;
	sw_rx = ix;
	sw_ry = iy;

	GLfloat lax, laxx, layy, lay, lx, ly;

	// center
	lx = ((sw_rx+sw_hw)) - hw;
	ly = hh - (((sw_ry+sw_hh)));
	// top left of the win
	lax = lx - sw_hw;
	lay = ly + sw_hh;
	// bottom right of the win
	laxx = lx + sw_hw;
	layy = ly - sw_hh;

	GLfloat ret_x,ret_y;
	ret_x = sw_hw;
	ret_y = sw_hh;
	// FIXME: CONVEXITY
	GLfloat ret_xx,ret_yy;
	ret_xx = sw_hw;
	ret_yy = sw_hh;
	ret_xx = ret_yy = 0;

	// top
	polygon->push_back(
		glTiledTexturedImage::point(-hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(hw, lay-ret_yy));
	img_points->push_back(
		glTiledTexturedImage::point(hw, ly-ret_y));
	if (ret_yy > 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(laxx, lay));
		img_points->push_back(
			glTiledTexturedImage::point(laxx, ly-ret_y));
		polygon->push_back(
			glTiledTexturedImage::point(lax, lay));
		img_points->push_back(
			glTiledTexturedImage::point(lax, ly-ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(-hw, lay-ret_yy));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, ly-ret_y));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// bottom
	polygon->push_back(
		glTiledTexturedImage::point(-hw, layy+ret_yy));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, ly+ret_y));
	if (ret_yy > 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(lax, layy));
		img_points->push_back(
			glTiledTexturedImage::point(lax, ly+ret_y));
		polygon->push_back(
			glTiledTexturedImage::point(laxx, layy));
		img_points->push_back(
			glTiledTexturedImage::point(laxx, ly+ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(hw, layy+ret_yy));
	img_points->push_back(
		glTiledTexturedImage::point(hw, ly+ret_y));
	polygon->push_back(
		glTiledTexturedImage::point(hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// FIXME: add sission lines
}

static
void setup_swallowed_deformation_top_complr_bottom(
	MetisseWindow *win,  MetisseWindow *s_win,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points)
{
	GLfloat w,h,hw,hh,w_rx,w_ry;
	win->getSize(&w,&h);
	win->getRealPosition(&w_rx, &w_ry);
	hw = w/2.0;
	hh = h/2.0;

	float sw_w, sw_h, sw_hw, sw_hh, sw_rx, sw_ry, sw_x, sw_y;
	s_win->getRealPosition(&sw_rx, &sw_ry);
	s_win->getSize(&sw_w,&sw_h);

	GLfloat ix,iy,iw,ih;
	ix = fmax(sw_rx,w_rx);
	iy = fmax(sw_ry,w_ry);
	iw = fmin(sw_rx + sw_w,w_rx + w) - fmax(sw_rx,w_rx);
	ih = fmin(sw_ry + sw_h,w_ry + h) - fmax(sw_ry,w_ry);
	sw_w = iw;
	sw_h = ih;
	sw_hw = iw/2.0;
	sw_hh = ih/2.0;
	sw_rx = ix; 
	sw_ry = iy;

	GLfloat lax, laxx, layy, lay, lx, ly;

	// center
	lx = ((sw_rx+sw_hw)) - hw;
	ly = hh - (((sw_ry+sw_hh)));
	// top left of the win
	lax = lx - sw_hw;
	lay = ly + sw_hh;
	// bottom right of the win
	laxx = lx + sw_hw;
	layy = ly - sw_hh;

	// up
	polygon->push_back(
		glTiledTexturedImage::point(-hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(hw, lay));
	img_points->push_back(
		glTiledTexturedImage::point(hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, lay));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// left
	polygon->push_back(
		glTiledTexturedImage::point(-hw, lay));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(lax, lay));
	img_points->push_back(
		glTiledTexturedImage::point(lx, lay));
	polygon->push_back(
		glTiledTexturedImage::point(lax, layy));
	img_points->push_back(
		glTiledTexturedImage::point(lx, layy));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, layy));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// right
	polygon->push_back(
		glTiledTexturedImage::point(laxx, lay));
	img_points->push_back(
		glTiledTexturedImage::point(lx, lay));
	polygon->push_back(
		glTiledTexturedImage::point(hw, lay));
	img_points->push_back(
		glTiledTexturedImage::point(hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(hw, layy));
	img_points->push_back(
		glTiledTexturedImage::point(hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(laxx, layy));
	img_points->push_back(
		glTiledTexturedImage::point(lx, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// down
	polygon->push_back(
		glTiledTexturedImage::point(-hw, layy));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(hw, layy));
	img_points->push_back(
		glTiledTexturedImage::point(hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// lines
	lines_points->push_back(
		glTiledTexturedImage::point(-hw, lay));
	lines_points->push_back(
		glTiledTexturedImage::point(hw, lay));
	lines_points->push_back(
		glTiledTexturedImage::point(-hw, layy));
	lines_points->push_back(
		glTiledTexturedImage::point(hw, layy));
}

static
void setup_swallowed_deformation_multiple(
	MetisseWindow *win,  MetisseWindow *s_win,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points)
{
	GLfloat w,h,hw,hh,w_rx,w_ry;
	win->getSize(&w,&h);
	win->getRealPosition(&w_rx, &w_ry);
	hw = w/2.0;
	hh = h/2.0;

	float sw_w, sw_h, sw_hw, sw_hh, sw_rx, sw_ry, sw_x, sw_y;
	s_win->getRealPosition(&sw_rx, &sw_ry);
	s_win->getSize(&sw_w,&sw_h);

	GLfloat ix,iy,iw,ih;
	ix = fmax(sw_rx,w_rx);
	iy = fmax(sw_ry,w_ry);
	iw = fmin(sw_rx + sw_w,w_rx + w) - fmax(sw_rx,w_rx);
	ih = fmin(sw_ry + sw_h,w_ry + h) - fmax(sw_ry,w_ry);
	sw_w = iw;
	sw_h = ih;
	sw_hw = iw/2.0;
	sw_hh = ih/2.0;
	sw_rx = ix; 
	sw_ry = iy;

	GLfloat lax, laxx, layy, lay, lx, ly;

	// center
	lx = ((sw_rx+sw_hw)) - hw;
	ly = hh - (((sw_ry+sw_hh)));
	// top left of the win
	lax = lx - sw_hw;
	lay = ly + sw_hh;
	// bottom right of the win
	laxx = lx + sw_hw;
	layy = ly - sw_hh;

	GLfloat ret_x,ret_y;
	ret_x = sw_hw/2.0;
	ret_y = sw_hh/2.0;
	ret_x = ret_y = 0;

	// top-left
	polygon->push_back(
		glTiledTexturedImage::point(-hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, hh));
	img_points->push_back(
		glTiledTexturedImage::point(lax, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(lax, lay));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// top-middle
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, hh));
	img_points->push_back(
		glTiledTexturedImage::point(lax, hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, hh));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, lay));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, layy));
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, lay));
	img_points->push_back(
		glTiledTexturedImage::point(lax, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// top-right
	polygon->push_back(
		glTiledTexturedImage::point(hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, hh));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, lay));
	polygon->push_back(
		glTiledTexturedImage::point(hw, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// right
	polygon->push_back(
		glTiledTexturedImage::point(hw, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(hw, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(laxx, layy));
	img_points->push_back(
		glTiledTexturedImage::point(lax, layy));
	polygon->push_back(
		glTiledTexturedImage::point(laxx, lay));
	img_points->push_back(
		glTiledTexturedImage::point(lax, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// bottom-right
	polygon->push_back(
		glTiledTexturedImage::point(hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, layy));
	polygon->push_back(
		glTiledTexturedImage::point(hw, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// bottom-middle
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(lax, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax, layy));
	img_points->push_back(
		glTiledTexturedImage::point(lax, lay));
	polygon->push_back(
		glTiledTexturedImage::point(laxx, layy));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// bottom-left
	polygon->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(lax, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(lax, layy));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// left
	polygon->push_back(
		glTiledTexturedImage::point(-hw, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, lay));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(lax, lay));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, lay));
	polygon->push_back(
		glTiledTexturedImage::point(lax, layy));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// add lines
}

static
void setup_swallowed_deformation_test(
	MetisseWindow *win,  MetisseWindow *s_win,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points)
{
	GLfloat w,h,hw,hh,w_rx,w_ry;
	win->getSize(&w,&h);
	win->getRealPosition(&w_rx, &w_ry);
	hw = w/2.0;
	hh = h/2.0;

	float sw_w, sw_h, sw_hw, sw_hh, sw_rx, sw_ry, sw_x, sw_y;
	s_win->getRealPosition(&sw_rx, &sw_ry);
	s_win->getSize(&sw_w,&sw_h);

	GLfloat ix,iy,iw,ih;
	ix = fmax(sw_rx,w_rx);
	iy = fmax(sw_ry,w_ry);
	iw = fmin(sw_rx + sw_w,w_rx + w) - fmax(sw_rx,w_rx);
	ih = fmin(sw_ry + sw_h,w_ry + h) - fmax(sw_ry,w_ry);
	sw_w = iw;
	sw_h = ih;
	sw_hw = iw/2.0;
	sw_hh = ih/2.0;
	sw_rx = ix; 
	sw_ry = iy;

	GLfloat lax, laxx, layy, lay, lx, ly;

	// center
	lx = ((sw_rx+sw_hw)) - hw;
	ly = hh - (((sw_ry+sw_hh)));
	// top left of the win
	lax = lx - sw_hw;
	lay = ly + sw_hh;
	// bottom right of the win
	laxx = lx + sw_hw;
	layy = ly - sw_hh;

	GLfloat ret_x,ret_y;
	ret_x = sw_hw/2.0;
	ret_y = sw_hh/2.0;
	//ret_x = ret_y = 0;

	// top-left top-left
	polygon->push_back(
		glTiledTexturedImage::point(-hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, hh));
	img_points->push_back(
		glTiledTexturedImage::point(lax, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(lax, lay));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// top-left top
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, hh));
	img_points->push_back(
		glTiledTexturedImage::point(lax, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lx, hh));
	img_points->push_back(
		glTiledTexturedImage::point(lx, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lx, lay));
	img_points->push_back(
		glTiledTexturedImage::point(lx, ly));
	if (ret_x != 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(lax, lay));
		img_points->push_back(
			glTiledTexturedImage::point(lx-ret_x, ly+ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(lax, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// top-left left
	polygon->push_back(
		glTiledTexturedImage::point(-hw, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, ly));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, ly));
	polygon->push_back(
		glTiledTexturedImage::point(lax, ly));
	img_points->push_back(
		glTiledTexturedImage::point(lx, ly));
	if (ret_x != 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(lax, lay));
		img_points->push_back(
			glTiledTexturedImage::point(lx-ret_x, ly+ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(lax, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));

	// bottom-left bottom-left
	polygon->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(lax, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(lax, layy));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// bottom-left bottom
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(lax, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lx, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(lx, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lx, layy));
	img_points->push_back(
		glTiledTexturedImage::point(lx, ly));
	if (ret_x != 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(lax, layy));
		img_points->push_back(
			glTiledTexturedImage::point(lx-ret_x, ly-ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(lax, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// bottom-left left
	polygon->push_back(
		glTiledTexturedImage::point(-hw, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(-hw, ly));
	img_points->push_back(
		glTiledTexturedImage::point(-hw, ly));
	polygon->push_back(
		glTiledTexturedImage::point(lax, ly));
	img_points->push_back(
		glTiledTexturedImage::point(lx, ly));
	if (ret_x != 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(lax, layy));
		img_points->push_back(
			glTiledTexturedImage::point(lx-ret_x, ly-ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(lax-ret_x, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(lax, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));

	// top-right top-right
	polygon->push_back(
		glTiledTexturedImage::point(hw, hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, hh));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, lay));
	polygon->push_back(
		glTiledTexturedImage::point(hw, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// top-right top
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, hh));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lx, hh));
	img_points->push_back(
		glTiledTexturedImage::point(lx, hh));
	polygon->push_back(
		glTiledTexturedImage::point(lx, lay));
	img_points->push_back(
		glTiledTexturedImage::point(lx, ly));
	if (ret_x != 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(laxx, lay));
		img_points->push_back(
			glTiledTexturedImage::point(lx+ret_x, ly+ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// top-right right
	polygon->push_back(
		glTiledTexturedImage::point(hw, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(hw, lay));
	polygon->push_back(
		glTiledTexturedImage::point(hw, ly));
	img_points->push_back(
		glTiledTexturedImage::point(hw, ly));
	polygon->push_back(
		glTiledTexturedImage::point(laxx, ly));
	img_points->push_back(
		glTiledTexturedImage::point(lx, ly));
	if (ret_x != 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(laxx, lay));
		img_points->push_back(
			glTiledTexturedImage::point(lx+ret_x, ly+ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, lay+ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, lay));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));


	// bottom-right bottom-right
	polygon->push_back(
		glTiledTexturedImage::point(hw, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(hw, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, layy));
	polygon->push_back(
		glTiledTexturedImage::point(hw, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// bottom-right bottom
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lx, -hh));
	img_points->push_back(
		glTiledTexturedImage::point(lx, -hh));
	polygon->push_back(
		glTiledTexturedImage::point(lx, layy));
	img_points->push_back(
		glTiledTexturedImage::point(lx, ly));
	if (ret_x != 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(laxx, layy));
		img_points->push_back(
			glTiledTexturedImage::point(lx+ret_x, ly-ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	// bottom-right right
	polygon->push_back(
		glTiledTexturedImage::point(hw, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(hw, layy));
	polygon->push_back(
		glTiledTexturedImage::point(hw, ly));
	img_points->push_back(
		glTiledTexturedImage::point(hw, ly));
	polygon->push_back(
		glTiledTexturedImage::point(laxx, ly));
	img_points->push_back(
		glTiledTexturedImage::point(lx, ly));
	if (ret_x != 0.0)
	{
		polygon->push_back(
			glTiledTexturedImage::point(laxx, layy));
		img_points->push_back(
			glTiledTexturedImage::point(lx+ret_x, ly-ret_y));
	}
	polygon->push_back(
		glTiledTexturedImage::point(laxx+ret_x, layy-ret_y));
	img_points->push_back(
		glTiledTexturedImage::point(laxx, layy));
	polygon->push_back(
		glTiledTexturedImage::point(0.0,0.0));
	img_points->push_back(
		glTiledTexturedImage::point(0.0,0.0));
}

void setup_swallowed_deformation(
	MetisseWindow *win,  MetisseWindow *s_win, strans_t trans_type,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points)
{
	if (trans_type == STRANS_LEFT_CUT_RIGHT)
	{
		setup_swallowed_deformation_left_cut_right(
			win, s_win, polygon, img_points, lines_points);
	}
	else if (trans_type == STRANS_TOP_CUT_BOTTOM)
	{
		setup_swallowed_deformation_top_cut_bottom(
			win, s_win, polygon, img_points, lines_points);
	}
	else if (trans_type == STRANS_TOP_COMPLR_BOTTOM)
	{
		setup_swallowed_deformation_top_complr_bottom(
			win, s_win, polygon, img_points, lines_points);
	}
	else if (trans_type == STRANS_SIMPLE_MULTIPLE)
	{
		setup_swallowed_deformation_multiple(
			win, s_win, polygon, img_points, lines_points);
	}
	else
	{
		setup_swallowed_deformation_test(
			win, s_win, polygon, img_points, lines_points);
	}
}
