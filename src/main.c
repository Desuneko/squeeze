/*
 *  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
 *
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
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libsqueeze/libsqueeze.h>

#include <gettext.h>

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

gboolean new_archive  = FALSE;
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
	{	"new", 'n', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &new_archive,
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
sq_archive_status_changed(LSQArchive *archive, gpointer data)
{
	if(archive->status == LSQ_ARCHIVESTATUS_IDLE)
	{
		opened_archives--;
	}
	if(opened_archives <= 0)
		gtk_main_quit();
}

void
archive_operation_failed(LSQArchive *archive, gpointer data)
{
}

void
archive_initialized(LSQArchive *archive, gpointer data)
{
}

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

	//g_thread_init(NULL);
  //gdk_threads_init();
	gdk_threads_enter();

	if(!gtk_init_with_args(&argc, &argv, _("[archive name]"), entries, PACKAGE, &cli_error))
	{
		if ( cli_error != NULL )
		{
			g_print (_("%s: %s\nTry %s --help to see a full list of available command line options.\n"), PACKAGE, cli_error->message, PACKAGE_NAME);
			g_error_free (cli_error);
			return 1;
		}
	}

	lsq_init();

	sq_icon_theme = gtk_icon_theme_get_default();
	sq_app = sq_application_new(sq_icon_theme);

	g_signal_connect(G_OBJECT(sq_app), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	if(version)
	{
		g_print("%s\n", PACKAGE_STRING);
		return 0;
	}


	if(extract_archive_path || extract_archive)
	{
		if(argc == 1)
		{
			return 1;
		}
		for(i = 1; i < argc; i++)
		{
			sq_application_extract_archive(sq_app, argv[i], extract_archive_path);
		}
	}
	if(new_archive || add_archive_path)
	{
		GSList *files = NULL;
		for(i = 1; i < argc; i++)
		{
			files = g_slist_prepend(files, argv[i]);
		}
		sq_application_new_archive(sq_app, add_archive_path, files);
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
	gdk_threads_leave();
	lsq_destroy();

	return 0;
}
