/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>

#include "settings.h"
#include "archive_store.h"
#include "navigation_bar.h"
#include "notebook.h"
#include "application.h"
#include "widget_factory.h"
#include "main_window.h"

#include "main.h"

gboolean version = FALSE;

gboolean extract_archive  = FALSE;
gchar *extract_archive_path = NULL;

gchar *new_archive  = NULL;
gchar *add_archive_path = NULL;

gpointer command;

gint opened_archives = 0;

static GOptionEntry entries[] =
{
	{	"extract-to", 'x', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &extract_archive_path,
		NULL,
		N_("[destination path]")
	},
	{	"extract", 'e', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &extract_archive,
		NULL,
		NULL
	},
	{	"add-to", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &add_archive_path,
		NULL,
		N_("[archive path] [file1] [file2] ... [fileN]")
	},
	{	"new", 'n', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &new_archive,
		NULL,
		N_("[file1] [file2] ... [fileN]")
	},
	{ "version", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
		N_("Version information"),
		NULL
	},
	{ NULL }
};

void
cb_main_window_destroy(SQMainWindow *window, gpointer data)
{
	gtk_main_quit();
}

int main(int argc, char **argv)
{
	GtkWidget *main_window = NULL;
	GtkIconTheme *sq_icon_theme;
	SQApplication *sq_app = NULL;
	GError *cli_error = NULL;
	gint i = 0;

	#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
 	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
 	textdomain (GETTEXT_PACKAGE);
	#endif

	if(!gtk_init_with_args(&argc, &argv, _("[archive name]"), entries, PACKAGE, &cli_error))
	{
		if ( cli_error != NULL )
		{
			g_print (_("%s: %s\nTry %s --help to see a full list of available command line options.\n"), PACKAGE, cli_error->message, PACKAGE_NAME);
			g_error_free (cli_error);
			return 1;
		}
	}
	gchar *current_dir = g_get_current_dir();

	thunar_vfs_init();
	lsq_init();

	sq_icon_theme = gtk_icon_theme_get_default();
	sq_app = sq_application_new(sq_icon_theme);

	gtk_window_set_default_icon_name("squeeze");

	g_signal_connect(G_OBJECT(sq_app), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	if(version)
	{
		g_print("%s\n", PACKAGE_STRING);
		return 0;
	}


	if(extract_archive_path || extract_archive)
	{
		gint err = 0;
		if(argc == 1)
		{
			return 1;
		}
		for(i = 1; i < argc; i++)
		{
			if(sq_application_extract_archive(sq_app, argv[i], extract_archive_path))
				err++;
		}
		if(err)
			return 1;
	}
	if(new_archive || add_archive_path)
	{
		GSList *files = NULL;

		/*
		 * Remove prefix if it is the pwd
		 */
		for(i = 1; i < argc; i++)
		{
			gchar *filename;
			if(g_str_has_prefix(argv[i], current_dir))
			{
				filename = g_strdup(&(argv[i][strlen(current_dir)+1]));
			}
			else
				filename = g_strdup(argv[i]);
			files = g_slist_prepend(files, filename);
		}

		/*
		 * Remove prefix if it is the pwd
		 */
		if(new_archive)
		{
			gchar *filename;
			if(g_str_has_prefix(new_archive, current_dir))
			{
				filename = g_strdup(&(new_archive[strlen(current_dir)+1]));
			}
			else
				filename = g_strdup(new_archive);
			files = g_slist_prepend(files, filename);
		}

		if(sq_application_new_archive(sq_app, add_archive_path, files))
			return 1;
	}

	if(!new_archive && !add_archive_path && !extract_archive && !extract_archive_path)
	{
		if(argc > 1)
		{
			if(sq_app->props._tabs)
				main_window = sq_application_new_window(sq_app);
			for(i = 1; i < argc; i++)
			{
				sq_application_open_archive(sq_app, main_window, argv[i]);
			}
		} else
		{
			main_window = sq_application_new_window(sq_app);
			gtk_widget_show(GTK_WIDGET(main_window));
		}
	}


	g_object_unref(sq_app);
	gtk_main();
	lsq_shutdown();
	thunar_vfs_shutdown();

	return 0;
}
