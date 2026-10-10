#ifndef FoldablePolygon_H
#define FoldablePolygon_H

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

#include <list>

#include "texture/glTiledTexturedImage.H"

struct FoldablePolygon
{

	// Use push_back for these two
	std::list<glTiledTexturedImage::point> points;
	std::list<glTiledTexturedImage::point> poly1;

	// Use push_front for this one
	std::list<glTiledTexturedImage::point> poly2;

	std::list<glTiledTexturedImage::point> shadow;

	double distanceFrom(double x, double y);
	void fold(
		double xi, double yi, double xj, double yj, double reduce = 1.0);
	void reset(void);
};

#endif
