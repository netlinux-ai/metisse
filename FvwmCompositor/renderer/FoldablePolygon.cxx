/*
 *
 * wstyle/FoldablePolygon.H --
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

#include "FoldablePolygon.H"

#include <list>
#include <math.h>

#include <nucleo/gl/glUtils.H>

double FoldablePolygon::distanceFrom(double x, double y)
{
	std::list<glTiledTexturedImage::point>::iterator i = points.end();
	i--;
	double dmin=99999999.0, xA=(*i).x, yA=(*i).y;

	for (i=points.begin(); i!=points.end(); ++i)
	{
		double xB=(*i).x, yB=(*i).y;
		double norm = ( (xB-xA)*(xB-xA) + (yB-yA)*(yB-yA) );
		double lambda =  ( (x-xA)*(xB-xA) + (y-yA)*(yB-yA) ) / norm;
		double d = 0.0;

		if (lambda<=0.0)
			d = (x-xA)*(x-xA) + (y-yA)*(y-yA);
		else if (lambda>=1.0) 
			d = (x-xB)*(x-xB) + (y-yB)*(y-yB);
		else
			d = (x-xA)*(x-xA) + (y-yA)*(y-yA) - lambda*lambda*norm;

		if (d<dmin) dmin=d;
		xA = xB;
		yA = yB;
	}

	return dmin;
}

void FoldablePolygon::fold(
	double xi, double yi, double xj, double yj, double reduce)
{    
	double dx = xj-xi;
	double dy = yj-yi;
	double xM = (xi+xj)/2.0;
	double yM = (yi+yj)/2.0;
	double red = 0.5 + 1/(2*reduce);
	double shade = 0.5 + 1/(2*(reduce-0.1*reduce));
	double shadeM = 0.5 + 1/(2*(reduce+0.1*reduce));

	double normIJ2 = (dx*dx + dy*dy);
	if (!normIJ2)
	{
		return;
	}
	double ax = -2.0 * dx / normIJ2;
	double ay = -2.0 * dy / normIJ2;

	std::list<glTiledTexturedImage::point>::iterator i = points.end();
	i--;
	double xA = (*i).x, yA = (*i).y;
	double sA = (xM-xA)*dx + (yM-yA)*dy;

	poly1.clear();
	poly2.clear();
	shadow.clear();

	for (i=points.begin(); i!=points.end(); ++i)
	{
		double xB = (*i).x, yB = (*i).y;
		double sB = (xB-xM)*dx + (yB-yM)*dy;

		if (sA == 0 || sB == 0 || sA*sB < 0)
		{
			// the whole segment is on the same side of D
			if (sA<=0)
			{
				poly1.push_back(
					glTiledTexturedImage::point(xB,yB));
			}
			else
			{
				double xBs = xB + sB*ax*red;
				double yBs = yB + sB*ay*red;
				double xBss = xB + sB*ax*shade;
				double yBss = yB + sB*ay*shade;
				poly2.push_front(
					glTiledTexturedImage::point(xBs,yBs));
				shadow.push_front(
					glTiledTexturedImage::point(xBss,yBss));
			}
		}
		else
		{
			double lambda = sA / (sA+sB);
			double xP = xA + lambda*(xB-xA);
			double yP = yA + lambda*(yB-yA);
			// sym(B)
			double xBs = xB + sB*ax*red;
			double yBs = yB + sB*ay*red;
			double xBss = xB + sB*ax*shade;
			double yBss = yB + sB*ay*shade;
			if (sA<0)
			{
				
				poly1.push_back(
					glTiledTexturedImage::point(xP,yP));
				poly2.push_front(
					glTiledTexturedImage::point(xP,yP));
				poly2.push_front(
					glTiledTexturedImage::point(xBs,yBs));
				shadow.push_front(
					glTiledTexturedImage::point(xP,yP));
				shadow.push_front(
					glTiledTexturedImage::point(xBss,yBss));
			}
			else
			{
				poly1.push_back(
					glTiledTexturedImage::point(xP,yP));
				poly1.push_back(
					glTiledTexturedImage::point(xB,yB));
				poly2.push_front(
					glTiledTexturedImage::point(xP,yP));
				shadow.push_front(
					glTiledTexturedImage::point(xP,yP));
			}
		}
		// Iteration
		xA = xB;
		yA = yB;
		sA = -sB;
	}

}

void FoldablePolygon::reset(void)
{
	poly1.clear();
	poly1 = points;
	poly2.clear();
	shadow.clear();
}

