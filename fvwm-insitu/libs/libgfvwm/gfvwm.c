#include "gfvwm.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib/gprintf.h>

/** private data */
static gboolean                 _gfvwm_is_init = FALSE;
static const char             * _gfvwm_name;
static int                      _gfvwm_fds[2];
static GIOChannel             * _gfvwm_channel_in;
static GIOChannel             * _gfvwm_channel_out;
static guint                    _gfvwm_watch_in;
static guint                    _gfvwm_log;
static GIOChannel             * _gfvwm_channel_log;
static GfvwmNewPacketCallback   _gfvwm_cb;


static gboolean
_fd_read_cb (GIOChannel * source, GIOCondition condition, gpointer data)
{
	if (condition & G_IO_IN)
	{
		if (_gfvwm_cb)
		{
			FvwmPacket *packet = ReadFvwmPacket (g_io_channel_unix_get_fd (source));
			_gfvwm_cb (packet);
			free (packet);
		}
		else
		{
			gchar message[FvwmPacketMaxSize_byte];
			gsize length;
			GError *error = NULL;

			g_io_channel_read_chars (source, message, FvwmPacketMaxSize_byte, &length, &error);
			//g_debug ("received bytes : %lu bytes long\n", length);
		}
	}
	else if (condition & G_IO_HUP)
	{
		g_source_remove (_gfvwm_watch_in);
		exit (0);
	}
	return TRUE;
}



static void
_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
             const gchar *message, gpointer user_data)
{
	GError *error = NULL;
	gchar  *output;
	gsize  bytes_written;

	if (log_domain)
		output = g_strconcat (log_domain, " - ", message, "\n", NULL);
	else
		output = g_strconcat (message, "\n", NULL);

	g_io_channel_write_chars (_gfvwm_channel_log, output, strlen(output),
	                         &bytes_written, &error);
	g_io_channel_flush (_gfvwm_channel_log, &error);

	g_free(output);
}



static void
_init_log ()
{
	GError *error = NULL;
	char   *template = NULL;
	char   *logname = NULL;

	template = g_strconcat ("gfvwm-", _gfvwm_name, "-XXXXXX", NULL);

	_gfvwm_log = g_file_open_tmp (template, &logname, &error);
	g_printf ("logname : %s\n", logname);
	g_free (template);
	g_free (logname);
	g_free (error);

	_gfvwm_channel_log = g_io_channel_unix_new (_gfvwm_log);
	g_io_channel_set_encoding (_gfvwm_channel_log, NULL, &error);
	g_io_channel_set_flags (_gfvwm_channel_log, G_IO_FLAG_NONBLOCK, &error);
	g_io_channel_set_buffered (_gfvwm_channel_log, FALSE);
	g_free (error);

	g_log_set_default_handler (_log_handler, NULL);
}



void
gfvwm_init (int *argc, char ***argv, const char *name)
{
	GError *error = NULL;

	if (_gfvwm_is_init)
		return;

	_gfvwm_name = g_strdup (name);

	_init_log ();

	_gfvwm_fds[0] = atoi((*argv)[1]);
	_gfvwm_fds[1] = atoi((*argv)[2]);

	_gfvwm_channel_out = g_io_channel_unix_new(_gfvwm_fds[0]);
	_gfvwm_channel_in  = g_io_channel_unix_new(_gfvwm_fds[1]);

	if (!_gfvwm_channel_out || !_gfvwm_channel_in)
		return;

	g_io_channel_set_encoding (_gfvwm_channel_out, NULL, &error);
	g_io_channel_set_encoding (_gfvwm_channel_in, NULL, &error);
	g_io_channel_set_flags (_gfvwm_channel_out, G_IO_FLAG_NONBLOCK, &error);
	g_io_channel_set_flags (_gfvwm_channel_in, G_IO_FLAG_NONBLOCK, &error);
	g_io_channel_set_buffered (_gfvwm_channel_out, FALSE);
	g_io_channel_set_buffered (_gfvwm_channel_in, FALSE);

	_gfvwm_watch_in = g_io_add_watch(
			_gfvwm_channel_in,
			G_IO_IN|G_IO_HUP,
			_fd_read_cb,
			NULL);

	_gfvwm_is_init = TRUE;

	g_free (error);

	return;
}



void
gfvwm_send_command (const gchar * command)
{
	gsize  len, bytes_written;
	gchar  *message, *p;
	GError *error = NULL;

	/* Get enough memory to store the entire message. */
	len = strlen(command);
	p = message = g_malloc(len + 3*sizeof(gulong));

	/* Put the message in the buffer, and... */
	*((unsigned long *)p) = 0;
	p += sizeof(unsigned long);

	*((unsigned long *)p) = len;
	p += sizeof(unsigned long);

	strcpy(p, command);
	p += len;

	*((unsigned long *)p) = 1;
	p += sizeof(unsigned long);

	g_io_channel_write_chars (_gfvwm_channel_out, message, len + 3*sizeof(gulong),
	                         &bytes_written, &error);
	g_io_channel_flush (_gfvwm_channel_out, &error);

	g_free (message);
}



void
gfvwm_register_callback (GfvwmNewPacketCallback cb)
{
	_gfvwm_cb = cb;
}
