/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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
#include <libxarchiver/libxarchiver.h>
#include <gtk/gtk.h>

#include "new_dialog.h"
#include "extract_dialog.h"
#include "main.h"

gboolean new_archive  = FALSE;

gboolean extract_archive  = FALSE;
gchar *extract_archive_path = NULL;

gboolean add_archive  = FALSE;

gchar *add_archive_path = NULL;

gchar **_argv;
gint _argc;

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
	{ NULL }
};

void
archive_status_changed(LXAArchive *archive, gpointer data)
{
}

void
archive_operation_failed(LXAArchive *archive, gpointer data)
{
}

void
archive_initialized(LXAArchive *archive, gpointer data)
{
}

int main(int argc, char **argv)
{
	gint result = 0;
	GtkWidget *dialog = NULL;
	LXAArchive *lpArchive;
	LXAArchiveSupport *lpSupport;
	GError *cli_error = NULL;
	gint i = 0;

	#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
 	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
 	textdomain (GETTEXT_PACKAGE);
	#endif

	g_thread_init(NULL);


	gtk_init_with_args(&argc, &argv, _("[archive name]"), entries, PACKAGE, &cli_error);

	thunar_vfs_init();

	lxa_init();

	if(extract_archive_path || extract_archive)
	{
		if(argc == 1)
		{
			return 1;
		}
		if(!extract_archive_path)
  		{
			dialog = xa_extract_archive_dialog_new();
			result = gtk_dialog_run (GTK_DIALOG (dialog) );
			if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
			{
				gtk_widget_destroy (GTK_WIDGET (dialog) );
				return 2;
			}
			extract_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gtk_widget_destroy (GTK_WIDGET (dialog) );
		}
		for(i = 1; i < argc; i++)
		{
			if(!lxa_open_archive(argv[i], &lpArchive))
			{
				lpSupport = lxa_get_support_for_mime(lpArchive->mime);
				lxa_archive_support_extract(lpSupport, lpArchive, extract_archive_path, NULL);
			}
		}
	}
	if(new_archive)
	{
		dialog = xa_new_archive_dialog_new();
		result = gtk_dialog_run (GTK_DIALOG (dialog) );
		if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
		{
			gtk_widget_destroy (GTK_WIDGET (dialog) );
			return 2;
		}
		if(result == GTK_RESPONSE_OK)
		{
			add_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gtk_widget_destroy (GTK_WIDGET (dialog) );
			if(!lxa_new_archive(add_archive_path, TRUE, NULL, &lpArchive))
			{
				GSList *files = NULL;
				for(i = 1; i < argc; i++)
				{
					files = g_slist_prepend(files, argv[i]);
				}
				lpSupport = lxa_get_support_for_mime(lpArchive->mime);
				lxa_archive_support_add(lpSupport, lpArchive, files);
			}
		}

	}

	gtk_main();

	return 0;
}
