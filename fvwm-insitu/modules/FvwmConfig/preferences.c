#include "preferences.h"

#include <gconf/gconf-client.h>

#define METISSE_GCONF_BASE "/apps/metisse"
#define METISSE_GCONF_THEME METISSE_GCONF_BASE "/theme"

struct _Prefs
{
	GConfClient *gconf_client;
	const gchar *homedir;
	const gchar *config_dir;
	GtkListStore *themes_store;
};

static Prefs *prefs = NULL;



static void
_prefs_fill_themes_store ()
{
	GError *error = NULL;
	gchar *themes_dir;
	const gchar *theme_file;
	GDir *dir;

	themes_dir = g_strjoin (NULL, prefs->config_dir, "/themes", NULL);

	/* setup a list of available themes */
	prefs->themes_store = gtk_list_store_new (N_COLUMNS,
					      G_TYPE_STRING,
					      G_TYPE_STRING);

	dir = g_dir_open (themes_dir, 0, &error);
	while ((theme_file = g_dir_read_name (dir)))
	{
		GtkTreeIter iter;
		const gchar *ext = NULL;
		gchar *theme_name = NULL;

		ext = g_strrstr (theme_file, ".theme");
		if (!ext)
			continue;

		theme_name = g_strndup (theme_file, ext - theme_file);

		gtk_list_store_append (prefs->themes_store, &iter);
		gtk_list_store_set (prefs->themes_store, &iter,
			NAME_COLUMN, theme_name,
			FILENAME_COLUMN, theme_file,
			-1);

		//g_print ("theme: %s\n", theme);
		g_free (theme_name);

	}
	g_dir_close (dir);

	g_free (themes_dir);
}



void
prefs_init ()
{
	if (prefs)
		return;

	prefs = g_new (Prefs, 1);

	prefs->gconf_client = gconf_client_get_default ();

	/* find fvwm's pref dir */
	prefs->homedir = g_getenv ("HOME");
	if (prefs->homedir == NULL) prefs->homedir = g_get_home_dir ();
	prefs->config_dir = g_strjoin (NULL, prefs->homedir, "/.fvwm-metisse", NULL);

	_prefs_fill_themes_store ();

	//g_print ("pouet, %s\n", themes_dir);
}



GtkListStore *
prefs_get_themes_store ()
{
	g_assert (prefs != NULL);
	return prefs->themes_store;
}



void
prefs_set_theme (const char* theme)
{
	g_debug ("prefs_set_theme : %s", theme);
	gconf_client_set_string (prefs->gconf_client,
	                         METISSE_GCONF_THEME, theme, NULL);
}



char*
prefs_get_theme ()
{
	return gconf_client_get_string (prefs->gconf_client,
	                         METISSE_GCONF_THEME, NULL);
}
#if 0
Preferences::Preferences()
{
	std::auto_ptr<Glib::Error> error(new Glib::Error);

	// gconf foo
	mClient = Gnome::Conf::Client::get_default_client();
	mClient->add_dir(_GCONF_PATH, Gnome::Conf::CLIENT_PRELOAD_NONE);
	mClient->notify_add(_GCONF_PATH, sigc::mem_fun(*this, &Preferences::onGconfNotify));

	// create the tree model for the themes list
	mThemesModel = Gtk::ListStore::create(mThemesColumns);

	mConfigDir = Glib::get_home_dir();
	mConfigDir.append("/.fvwm-metisse");

	fillThemesModel();
}



void Preferences::themesSelection(Glib::RefPtr<Gtk::TreeSelection> selection)
{
	mThemesSelection = selection;
	mThemesSelection->signal_changed().connect(sigc::mem_fun(*this, &Preferences::onSelectionChanged));

	std::string theme = mClient->get_entry(_GCONF_PATH "/theme").get_value().get_string();
	updateThemesSelection(theme);
}



void Preferences::fillThemesModel()
{
	std::string uri = Gnome::Vfs::get_uri_from_local_path(mConfigDir);
	uri.append("/themes");

	Gnome::Vfs::DirectoryHandle::visit(uri, Gnome::Vfs::FILE_INFO_DEFAULT,
	                                   Gnome::Vfs::DIRECTORY_VISIT_LOOPCHECK,
	                                   sigc::mem_fun(*this, &Preferences::on_visit));
}



bool Preferences::on_visit(const Glib::ustring& rel_path, const Glib::RefPtr<const Gnome::Vfs::FileInfo>& info, bool recursing_will_loop, bool& recurse)
{
	recurse = false;

	const std::string extension(".theme");
	const std::string name(info->get_name());

	const size_t loc = name.find_last_of('.');
	if (loc == std::string::npos)
		return true;

	if (name.substr(loc) != extension)
		return true;

	Gtk::TreeModel::Row row;
	row = *(mThemesModel->append());
	row[mThemesColumns.filename] = name.substr(0, loc);

	return true;
}



void Preferences::onSelectionChanged()
{
	Gtk::TreeIter i = mThemesSelection->get_selected();
	if (i)
	{
		mClient->set(_GCONF_PATH "/theme", (*i)[mThemesColumns.filename]);
	}
}



void Preferences::onGconfNotify(guint context, Gnome::Conf::Entry entry)
{
	std::string key = Glib::path_get_basename (entry.get_key());
	if (key == "theme")
	{
		updateThemesSelection(entry.get_value().get_string());
	}
}



void Preferences::updateThemesSelection(const std::string & theme)
{
	Gtk::TreeIter i = mThemesModel->get_iter("0");

	if (!i)
		return;

	while (i)
	{
		if ((*i)[mThemesColumns.filename] == theme)
			break;
		i++;
	}
	mThemesSelection->unselect_all();
	mThemesSelection->select(*i);
}
#endif
