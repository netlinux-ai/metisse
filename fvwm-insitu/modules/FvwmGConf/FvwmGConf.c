#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <gconf/gconf-client.h>
#include <string.h>

#include "libs/libgfvwm/gfvwm.h"

#define _GCONF_PATH "/apps/metisse"

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
static void     destroy(GtkWidget *widget, gpointer data);
static void     change_theme(GtkWidget *widget, gpointer data);



static void
_gconf_notify (GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data)
{
	char * key = g_path_get_basename (gconf_entry_get_key (entry));
	if (strcmp (key, "theme") == 0)
	{
		const char * theme = gconf_value_get_string (gconf_entry_get_value (entry));
		g_debug ("theme changed : %s", theme);

		char * command = g_strconcat ("FuncTheme ", theme, NULL);
		gfvwm_send_command (command);
		g_free (command);
	}
	else if (strcmp (key, "mouse_focus") == 0)
	{
		/* do something here */
	}
	g_free (key);
}



int main(int argc, char **argv)
{
	int i;
	gfvwm_init (&argc, &argv, "FvwmGConf");

	gtk_init (&argc, &argv);

	GConfClient *client = gconf_client_get_default ();
	gconf_client_add_dir (client, _GCONF_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
	gconf_client_notify_add (client, _GCONF_PATH, _gconf_notify, NULL, NULL, NULL);

	// the window
	GtkWindow * window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_default_size(window, 400, 300);

	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "destroy",      G_CALLBACK(destroy), NULL);


	// the textview
	GtkWidget *textview;
	textview = gtk_text_view_new();

	char * text = g_strdup("");

	for (i = 0; i < argc; i++)
	{
		char * tmp = g_strdup_printf("%s%i \"%s\"\n", text, i, argv[i]);
		g_free(text);
		text = g_strdup(tmp);
		g_free(tmp);
	}

	GtkTextBuffer * textbuf;
	textbuf = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text (textbuf, text, -1);

	gtk_text_view_set_buffer (GTK_TEXT_VIEW(textview), textbuf);


	// the button
	GtkWidget *button;
	button = gtk_button_new_with_label("FuncTheme Metisse");
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (change_theme), NULL);

	// the vbox
	GtkVBox *vbox = GTK_VBOX(gtk_vbox_new(FALSE, 12));
	gtk_box_pack_start(GTK_BOX(vbox), textview, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);


	gtk_container_set_border_width(GTK_CONTAINER (window), 12);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));

	gtk_widget_show_all(GTK_WIDGET(window));
	gtk_main();

	g_free(text);

	g_object_unref (G_OBJECT (client));

	return 0;
}




static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    /* If you return FALSE in the "delete_event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */

    g_print ("delete event occurred\n");

    /* Change TRUE to FALSE and the main window will be destroyed with
     * a "delete_event". */

    return FALSE;
}

/* Another callback */
static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
}



static void change_theme(GtkWidget *widget, gpointer data)
{
	gfvwm_send_command("FuncTheme Metisse");
	//return FALSE; // idle callback is to be called only once
}

