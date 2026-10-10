#ifndef GOODIES_H
#define GOODIES_H

/*
 *
 * wstyle/FoldableWindow.H --
 *
 * Copyright (C) Olivier Chapuis
 * Copyright (C) Nicolas Roussel
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <nucleo/gl/glUtils.H>
#include "FoldablePolygon.H"
#include "desktop/MetisseWindow.H"

typedef enum {
  STRANS_LENS,
  STRANS_LEFT_CUT_RIGHT,
  STRANS_TOP_CUT_BOTTOM,
  STRANS_TOP_COMPLR_BOTTOM,
  STRANS_LEFT_COMPTB_RIGHT, // no
  STRANS_SIMPLE_MULTIPLE,
  STRANS_TEST,
} strans_t;

void goodies_marcking_feedback(
	GLfloat x, GLfloat y, int dist, int draw_dist, int dirs, GLfloat alpha);
void goodies_shadow(
	GLfloat hw, GLfloat hh, float startAlpha, GLfloat shborder);
void goodies_shadow(
	FoldablePolygon fpoly, float startAlpha, float foldAlpha,
	GLfloat shborder);
void shadow_line(
	GLfloat start_x, GLfloat star_y, GLfloat end_x, GLfloat end_y,
	GLfloat startAlpha, GLfloat width);
void setup_circle_deformation(
	GLfloat hw, GLfloat hh,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points);
void setup_snapping_deformation(
	bool do_x_snapping, bool do_y_snapping,
	bool snap_top, bool snap_right, GLfloat snap_xoff, GLfloat snap_yoff,
	GLfloat d, GLfloat sad, GLfloat hw, GLfloat hh,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points);
void setup_lens_deformation(
	int autolens_type, int cnum, bool do_blens, GLfloat hw, GLfloat hh,
	GLfloat lens_ax, GLfloat lens_ay, GLfloat lens_x, GLfloat lens_y,
	GLfloat lens_w, GLfloat lens_h, GLfloat lens_sx, GLfloat lens_sy,
	GLfloat blens_ax, GLfloat blens_ay, GLfloat blens_x, GLfloat blens_y,
	GLfloat blens_w, GLfloat blens_h, GLfloat blens_sx, GLfloat blens_sy,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points);
static
void setup_swallowed_deformation_test(
	MetisseWindow *win,  MetisseWindow *s_win,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points);
void setup_swallowed_deformation(
	MetisseWindow *win,  MetisseWindow *s_win, strans_t trans_type,
	std::list<glTiledTexturedImage::point>  *polygon,
	std::list<glTiledTexturedImage::point>  *img_points,
	std::list<glTiledTexturedImage::point>  *lines_points);
#endif
