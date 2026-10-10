#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glade/glade-xml.h>
#include <glade/glade-init.h>

#include "libs/libgfvwm/gfvwm.h"

#include "preferences.h"

static gboolean delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	gtk_main_quit ();
	g_printf("quitting1\n");
	return TRUE;
}

static void on_close_clicked (GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
	g_printf ("quitting2\n");
}



static void
on_theme_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar *theme = NULL;

		gtk_tree_model_get (model, &iter, NAME_COLUMN, &theme, -1);
		prefs_set_theme (theme);

		g_free (theme);
	}
}



int main(int argc, char *argv[])
{
	GladeXML *xml = NULL;
	GtkWidget *win = NULL;
	GtkWidget *close = NULL;

	gfvwm_init (&argc, &argv, "FvwmConfig");
	gtk_init (&argc, &argv);
	glade_init ();

	prefs_init ();

	/* Preferences UI */
	xml = glade_xml_new (GLADEDIR "/FvwmConfig.glade", NULL, NULL);

	win = glade_xml_get_widget (xml, "main");
	g_signal_connect (G_OBJECT(win), "delete-event", G_CALLBACK(delete_event), NULL);
	gtk_widget_show_all (win);

	close = glade_xml_get_widget (xml, "close");
	g_signal_connect (G_OBJECT(close), "clicked", G_CALLBACK(on_close_clicked), NULL);

	/* theme list */
	{
		GtkTreeModel *model = NULL;
		GtkTreeView *view = NULL;
		GtkCellRenderer *renderer = NULL;
		GtkTreeSelection *select = NULL;
		char *theme = NULL;
		GtkTreeIter iter;

		model = GTK_TREE_MODEL (prefs_get_themes_store ());
		view = GTK_TREE_VIEW (glade_xml_get_widget (xml, "themes_view"));
		gtk_tree_view_set_model (view, model);

		renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_insert_column_with_attributes (view, 0, "Name", renderer,
							"text", NAME_COLUMN,
							NULL);

		select = gtk_tree_view_get_selection (view);
		gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
		g_signal_connect (G_OBJECT (select), "changed",
				  G_CALLBACK (on_theme_selection_changed), NULL);

		/* set the inital theme (read from gconf) */
		theme = prefs_get_theme ();
		if (theme && gtk_tree_model_get_iter_first (model, &iter))
		{
			do
			{
				GValue value = {0};
				g_value_init (&value, G_TYPE_STRING);
				gtk_tree_model_get_value (model, &iter, NAME_COLUMN, &value);
				if (strcmp (g_value_get_string (&value), theme) == 0)
				{
					gtk_tree_selection_select_iter (select, &iter);
					break;
				}
			}
			while (gtk_tree_model_iter_next (model, &iter));
		}
		g_free (theme);
	}

	gtk_main ();

	return 0;
}
