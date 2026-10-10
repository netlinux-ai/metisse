#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include <gtk/gtk.h>

enum
{
	NAME_COLUMN,
	FILENAME_COLUMN,
	N_COLUMNS
};

typedef struct _Prefs Prefs;

void 		prefs_init ();
GtkListStore*	prefs_get_themes_store ();
void		prefs_set_theme (const char *theme_name);
char*		prefs_get_theme ();



#endif
