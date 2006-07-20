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
#include <gettext.h>
#include <glib.h>
#include <glib-object.h>
#include <libxarchiver/libxarchiver.h>
#include <gtk/gtk.h>

#include "new_dialog.h"
#include "main.h"

gboolean no_gui = FALSE;
gboolean new_archive  = FALSE;
gboolean silent = FALSE;

gboolean extract_archive  = FALSE;
gchar *extract_archive_path = NULL;

gboolean add_archive  = FALSE;

gchar *add_archive_path = NULL;

gchar **_argv;
gint _argc;

gpointer command;

gint opened_archives = 0;

/*
 * Using roughly the same interface as File-roller.
 */
static GOptionEntry entries[] =
{
	{	"extract-to", 'x', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &extract_archive_path,
		N_("Extract the archive to the specified folder and quits."),
		N_("[destination path]")
	},
	{	"extract", 'e', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &extract_archive,
		N_("Extract the archive by asking the destination folder and quits."),
		NULL
	},
	{	"add-to", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &add_archive_path,
		N_("Add files to the specified archive and quits."),
		N_("[archive path] [file1] [file2] ... [fileN]")
	},
	/*
	{	"add", 'a', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &add_archive,
		N_("Add files to the specified archive by asking their filenames and quits."),
		NULL
	},
	*/
	{	"new", 'n', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &new_archive,
		N_("Ask for the archive to be created, add files to it and quits."),
		N_("[file1] [file2] ... [fileN]")
	},
	{ NULL }
};

void
archive_status_changed(LXAArchive *archive, gpointer data)
{
	GtkWidget *dialog;
	gchar *msg_string;
	switch(archive->status)
	{
		case(LXA_ARCHIVESTATUS_IDLE):
			if(add_archive || extract_archive || extract_archive_path || add_archive_path)
				opened_archives--;
			break;
		case(LXA_ARCHIVESTATUS_ERROR):
			if(add_archive || extract_archive || extract_archive_path || add_archive_path)
				opened_archives--;
			break;
		case(LXA_ARCHIVESTATUS_ADD):
			break;
		case(LXA_ARCHIVESTATUS_USERBREAK):
		case(LXA_ARCHIVESTATUS_REMOVE):
		case(LXA_ARCHIVESTATUS_EXTRACT):
		case(LXA_ARCHIVESTATUS_VIEW):
			break;
	}
	if(opened_archives <= 0)
	{
		gtk_main_quit();
	}
}

void
archive_operation_failed(LXAArchive *archive, gpointer data)
{
	GtkWidget *dialog;
	gchar *msg_string;
	switch(archive->status)
	{
		case(LXA_ARCHIVESTATUS_ADD):
			msg_string = _("Could not add file(s) to archive '%s'");
			break;
		case(LXA_ARCHIVESTATUS_EXTRACT):
			msg_string = _("Could not extract archive '%s'");
			break;
		case(LXA_ARCHIVESTATUS_REMOVE):
			msg_string = _("Could not remove file(s) from archive '%s'");
			break;
	}
	dialog = gtk_message_dialog_new (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,msg_string, archive->path);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
	gtk_dialog_run (GTK_DIALOG (dialog) );
	gtk_widget_destroy (GTK_WIDGET (dialog) );
}

void
archive_initialized(LXAArchive *archive, gpointer data)
{
	GtkWidget *dialog;
	GSList *files = NULL;
	gint i = 0;
	if(add_archive_path)
	{
		for(i = 1; i < _argc; i++)
		{
			if(g_file_test(_argv[i], G_FILE_TEST_EXISTS))
				files = g_slist_prepend(files, _argv[i]);
			else
			{
				dialog = gtk_message_dialog_new (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("File '%s' does not exist: ABORTING"), _argv[i]);
				gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
				gtk_dialog_run (GTK_DIALOG (dialog) );
				gtk_widget_destroy (GTK_WIDGET (dialog) );
				g_slist_free(files);
				lxa_close_archive(archive);
				gtk_main_quit();
			}
		}
		lxa_archive_add(archive, files);
	}
	if(extract_archive_path)
		lxa_archive_extract(archive, files, extract_archive_path);
}

int main(int argc, char **argv)
{
	GError *cli_error = NULL;
	GtkWidget *dialog;
	LXAArchive *xa_archive = NULL;
	gint result = 0;
	gint i = 0;
	GSList *files = NULL;
	
  #ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
  #endif


	gtk_init_with_args(&argc, &argv, _("[archive name]"), entries, PACKAGE, &cli_error);

	lxa_init();

	_argc = argc;
	_argv = argv;
	if(extract_archive_path || extract_archive)
	{
		if(argc == 1)
		{
			return 1;
		}
		if(!extract_archive_path)
		{
			/* 
			 * TODO: Show extract-dialog.
			 */
		}
		for(i = 1; i < argc; i++)
		{
			if(!lxa_open_archive(argv[i], &xa_archive, G_CALLBACK(archive_initialized)))
			{
				opened_archives++;
				g_signal_connect(G_OBJECT(xa_archive), "lxa_status_changed", G_CALLBACK(archive_status_changed), NULL);
				g_signal_connect(G_OBJECT(xa_archive), "lxa_init_complete", G_CALLBACK(archive_initialized), NULL);
				g_signal_connect(G_OBJECT(xa_archive), "lxa_operation_failure",  G_CALLBACK(archive_operation_failed), NULL);
			}
			else
			{
				dialog = gtk_message_dialog_new (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Could not extract archive '%s'"), argv[i]);
				gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
				gtk_dialog_run (GTK_DIALOG (dialog) );
				gtk_widget_destroy (GTK_WIDGET (dialog) );
			}
		}
	}

	if(add_archive_path || new_archive)
	{
		if(argc < 2)
		{
			dialog = gtk_message_dialog_new (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("You should provide at least one file to add\n") );
			gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
			gtk_dialog_run (GTK_DIALOG (dialog) );
			gtk_widget_destroy (GTK_WIDGET (dialog) );
			return 1;
		}
		if(add_archive_path && !lxa_open_archive(add_archive_path, &xa_archive, G_CALLBACK(archive_initialized)))
		{
			g_signal_connect(G_OBJECT(xa_archive), "lxa_status_changed", G_CALLBACK(archive_status_changed), NULL);
			g_signal_connect(G_OBJECT(xa_archive), "lxa_init_complete", G_CALLBACK(archive_initialized), NULL);
			g_signal_connect(G_OBJECT(xa_archive), "lxa_operation_failure",  G_CALLBACK(archive_operation_failed), NULL);
			opened_archives++;
		}
		else /* No file-name provided, a new archive has to be created: */
		{
			/* TODO: Show new archive dialog */
			dialog = xa_new_archive_dialog_new();
			result = gtk_dialog_run (GTK_DIALOG (dialog) );
			gtk_widget_destroy (GTK_WIDGET (dialog) );
			if(result == GTK_RESPONSE_CANCEL)
				return 2;
			if(result == GTK_RESPONSE_OK)
			{
				/* do crazy stuff */
				return 0;
			}
		}
	}

	if(!add_archive && !extract_archive && !extract_archive_path && !add_archive_path)
		return 0;

	gtk_main();
	
	return 0;
}
