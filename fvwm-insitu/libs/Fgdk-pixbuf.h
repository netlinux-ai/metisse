/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */

#ifndef FGDKPIXBUF_H
#define FGDKPIXBUF_H

/* ---------------------------- included header files ---------------------- */

#include "PictureBase.h"

#if GdkPixbufSupport
#include <gdk-pixbuf/gdk-pixbuf.h>
#else

#endif


/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

#if GdkPixbufSupport
/* glib */
typedef gint fgint;
typedef GError FGError; 

/* gdk-pixbuf */
typedef GdkPixbufAlphaMode FGdkPixbufAlphaMode;
typedef GdkPixbuf FGdkPixbuf;
typedef GdkPixbufFormat FGdkPixbufFormat;

#define FGDK_INTERP_HYPER GDK_INTERP_HYPER
#define FGDK_INTERP_BILINEAR GDK_INTERP_BILINEAR

#else
typedef int fgint;
typedef struct
{
	char *message;
} FGError; 

typedef enum
{
        FGDK_PIXBUF_ALPHA_BILEVEL,
        FGDK_PIXBUF_ALPHA_FULL
} FGdkPixbufAlphaMode;

typedef void FGdkPixbuf;
typedef struct 
{
	char dummy;
} FGdkPixbufFormat; 

#define FGDK_INTERP_HYPER 0
#define FGDK_INTERP_BILINEAR 0

#endif

/* ---------------------------- global definitions ------------------------- */

#if GdkPixbufSupport
/* glib */
#define fg_type_init  g_type_init
#define fg_error_free g_error_free
/* gobject */
#define fg_object_unref g_object_unref
/* gdk-pixbuf */
#define fgdk_pixbuf_new_from_file gdk_pixbuf_new_from_file 
#define fgdk_pixbuf_new_from_file_at_scale gdk_pixbuf_new_from_file_at_scale
#define fgdk_pixbuf_new_from_file_at_size gdk_pixbuf_new_from_file_at_size

#define fgdk_pixbuf_get_bits_per_sample gdk_pixbuf_get_bits_per_sample
#define fgdk_pixbuf_get_pixels gdk_pixbuf_get_pixels
#define fgdk_pixbuf_get_width gdk_pixbuf_get_width
#define fgdk_pixbuf_get_height gdk_pixbuf_get_height
#define fgdk_pixbuf_get_rowstride gdk_pixbuf_get_rowstride
#define fgdk_pixbuf_get_n_channels gdk_pixbuf_get_n_channels
#define fgdk_pixbuf_get_has_alpha gdk_pixbuf_get_has_alpha

#define fgdk_pixbuf_new gdk_pixbuf_new
#define fgdk_pixbuf_scale_simple gdk_pixbuf_scale_simple

#else

#define fg_type_init()
#define fg_error_free(a)
#define fg_object_unref(a)
#define fgdk_pixbuf_new_from_file(a,b)  NULL
#define fgdk_pixbuf_new_from_file_at_scale(a,b,c,d,e) NULL
#define fgdk_pixbuf_new_from_file_at_size(a,b,c,d) NULL

#define fgdk_pixbuf_get_bits_per_sample(a) 0
#define fgdk_pixbuf_get_pixels(a) NULL
#define fgdk_pixbuf_get_width(a) 0
#define fgdk_pixbuf_get_height(a) 0
#define fgdk_pixbuf_get_rowstride(a) 0
#define fgdk_pixbuf_get_n_channels(a) 0
#define fgdk_pixbuf_get_has_alpha(a) 0

#define fgdk_pixbuf_new(a,b,c,d,e) NULL
#define fgdk_pixbuf_scale_simple(a,b,c,d) NULL
#endif


#endif /* FGDKPIXBUF_H */
