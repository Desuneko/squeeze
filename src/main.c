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
#include "add_dialog.h"
#include "extract_dialog.h"
#include "main.h"
#include "archive_store.h"
#include "navigation_bar.h"
#include "main_window.h"

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
xa_archive_status_changed(LXAArchive *archive, gpointer data)
{
	if(archive->status == LXA_ARCHIVESTATUS_IDLE)
	{
		opened_archives--;
	}
	if(opened_archives <= 0)
		gtk_main_quit();
}

void
archive_operation_failed(LXAArchive *archive, gpointer data)
{
}

void
archive_initialized(LXAArchive *archive, gpointer data)
{
}

void
cb_main_window_destroy(XAMainWindow *window, gpointer data)
{
	if(window->lp_xa_archive)
	{
		lxa_close_archive(window->lp_xa_archive);
		opened_archives--;
	}
	if(opened_archives <= 0)
	{
		gtk_main_quit();
	}
}

int main(int argc, char **argv)
{
	gint result = 0;
	GtkWidget *dialog = NULL;
	GtkWidget *main_window = NULL;
	LXAArchive *lp_archive;
	LXAArchiveSupport *lpSupport;
	GtkIconTheme *xa_icon_theme;
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
			g_print (_("%s: %s\nTry xarchiver --help to see a full list of available command line options.\n"), PACKAGE, cli_error->message);
			g_error_free (cli_error);
			return 1;
		}
	}

	thunar_vfs_init();

	lxa_init();

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
			if(!lxa_open_archive(argv[i], &lp_archive))
			{
				g_signal_connect(G_OBJECT(lp_archive), "lxa_status_changed", G_CALLBACK(xa_archive_status_changed), NULL);
				opened_archives++;
				lpSupport = lxa_get_support_for_mime(lp_archive->mime);
				if(!extract_archive_path)
  			{
					dialog = xa_extract_archive_dialog_new(lpSupport, lp_archive, FALSE);
					result = gtk_dialog_run (GTK_DIALOG (dialog) );
					if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
					{
						gtk_widget_destroy (GTK_WIDGET (dialog) );
						lxa_close_archive(lp_archive);
						opened_archives--;
					}
					if(result == GTK_RESPONSE_OK)
					{
						extract_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
						lxa_archive_support_extract(lpSupport, lp_archive, extract_archive_path, NULL);
						g_free(extract_archive_path);
						extract_archive_path = NULL;
					}
				}
				else
					lxa_archive_support_extract(lpSupport, lp_archive, extract_archive_path, NULL);
			}
		}
	}
	if(new_archive || add_archive_path)
	{
		if(!add_archive_path)
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
			}
			if(lxa_new_archive(add_archive_path, TRUE, NULL, &lp_archive))
			{
				/* 
				 * Could not create archive (mime type unsupported) 
				 */
				dialog = gtk_message_dialog_new (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Could not create archive, MIME-type unsupported"));
				gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
				gtk_dialog_run (GTK_DIALOG (dialog) );
				gtk_widget_destroy (GTK_WIDGET (dialog) );
				return 1;
			}
			else
				opened_archives++;
		}
		else
		{
			if(lxa_open_archive(add_archive_path, &lp_archive))
			{
				/*
				 * Could not open archive (mime type not supported or file did not exist)
				 * Should be a more specific error message.
				 */ 
				dialog = gtk_message_dialog_new (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Could not open archive, MIME-type unsupported or file did not exist"));
				gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
				gtk_dialog_run (GTK_DIALOG (dialog) );
				gtk_widget_destroy (GTK_WIDGET (dialog) );
				return 1;
			}
			else
				opened_archives++;
		}
		g_signal_connect(G_OBJECT(lp_archive), "lxa_status_changed", G_CALLBACK(xa_archive_status_changed), NULL);
		GSList *files = NULL;
		for(i = 1; i < argc; i++)
		{
			files = g_slist_prepend(files, argv[i]);
		}
		lpSupport = lxa_get_support_for_mime(lp_archive->mime);
		lxa_archive_support_add(lpSupport, lp_archive, files);
	}

	if(!new_archive && !add_archive_path && !extract_archive && !extract_archive_path)
	{
		
		xa_icon_theme = gtk_icon_theme_get_default();
		if(argc > 1)
		{
			opened_archives++;
			for(i = 1; i < argc; i++)
			{
				/* Show main window */
				main_window = xa_main_window_new(xa_icon_theme);
				g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(cb_main_window_destroy), NULL);

				if(!xa_main_window_open_archive(XA_MAIN_WINDOW(main_window), argv[i]))
				{
					opened_archives++;
					gtk_widget_set_size_request(main_window, 500, 350);
					gtk_widget_show_all(main_window);
				} else
				{
					gtk_widget_destroy(main_window);
				}
			}
			opened_archives--;
			if(opened_archives <= 0)
				return 1;
		} else
		{
			/* Show main window */
			main_window = xa_main_window_new(xa_icon_theme);
			g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(cb_main_window_destroy), NULL);
			gtk_widget_set_size_request(main_window, 500, 350);
			gtk_widget_show_all(main_window);
		}
	} else
		if(!opened_archives)
			return 0;

	gtk_main();
	lxa_destroy();

	return 0;
}
