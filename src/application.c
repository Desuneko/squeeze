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
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>

#include "new_dialog.h"
#include "add_dialog.h"
#include "extract_dialog.h"

#include "settings.h"
#include "archive_store.h"
#include "navigation_bar.h"
#include "application.h"
#include "widget_factory.h"
#include "main_window.h"

static void
sq_application_class_init(SQApplicationClass *archive_class);

static void
sq_application_init(SQApplication *);
static void
sq_application_finalize(GObject *);
static void
sq_application_dispose(GObject *object);

enum
{
	SQ_APPLICATION_SIGNAL_DESTROY = 0,
	SQ_APPLICATION_SIGNAL_COUNT
};

static gint sq_application_signals[SQ_APPLICATION_SIGNAL_COUNT];

GType
sq_application_get_type ()
{
	static GType sq_application_type = 0;

 	if (!sq_application_type)
	{
 		static const GTypeInfo sq_application_info = 
		{
			sizeof (SQApplicationClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_application_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQApplication),
			0,
			(GInstanceInitFunc) sq_application_init,
			NULL
		};

		sq_application_type = g_type_register_static (G_TYPE_OBJECT, "SQApplication", &sq_application_info, 0);
	}
	return sq_application_type;
}

static void
sq_application_class_init(SQApplicationClass *application_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (application_class);
	object_class->finalize     = sq_application_finalize;
	object_class->dispose      = sq_application_dispose;

	sq_application_signals[SQ_APPLICATION_SIGNAL_DESTROY] = g_signal_new("destroy",
			G_TYPE_FROM_CLASS(application_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
}

static void
sq_application_init(SQApplication *application)
{
	application->settings = sq_settings_new();
	sq_settings_set_group(application->settings, "Global");

	application->props._tabs = sq_settings_read_bool_entry(application->settings, "UseTabs", TRUE);

}

static void
sq_application_dispose(GObject *object)
{
	g_signal_emit(object, sq_application_signals[SQ_APPLICATION_SIGNAL_DESTROY], 0, object);
}

static void
sq_application_finalize(GObject *object )
{
	SQApplication *application = SQ_APPLICATION(object);

	sq_settings_set_group(application->settings, "Global");

	sq_settings_write_bool_entry(application->settings, "UseTabs", application->props._tabs);

	sq_settings_save(application->settings);

	g_object_unref(G_OBJECT(application->settings));
}

SQApplication *
sq_application_new(GtkIconTheme *icon_theme)
{
	SQApplication *app;

	app = g_object_new(SQ_TYPE_APPLICATION, NULL);

	app->icon_theme = icon_theme;

	return app;
}

GtkWidget *
sq_application_new_window(SQApplication *app)
{
	GtkWidget *window = sq_main_window_new(app, app->icon_theme);
	gtk_widget_set_size_request(window, 500, 300);
	return window;
}

void
cb_sq_application_archive_status_changed(LSQArchive *archive, gpointer data)
{
	SQApplication *app = SQ_APPLICATION(data);

	switch(archive->status)
	{
		case LSQ_ARCHIVESTATUS_IDLE:
		case LSQ_ARCHIVESTATUS_ERROR:
			lsq_close_archive(archive);
		case LSQ_ARCHIVESTATUS_USERBREAK:
			g_object_unref(app);
			break;
		default:
			break;
	}
}

gint
sq_application_extract_archive(SQApplication *app, gchar *archive_path, gchar *dest_path)
{
	GtkWidget *dialog = NULL;
	gint result = 0;
	LSQArchive *lp_archive = NULL;
	LSQArchiveSupport *lp_support = NULL;

	if(!lsq_open_archive(archive_path, &lp_archive))
	{
		g_signal_connect(G_OBJECT(lp_archive), "lsq_status_changed", G_CALLBACK(cb_sq_application_archive_status_changed), app);
		lp_support = lsq_get_support_for_mime(lp_archive->mime_info);
		if(!dest_path)
		{
			dialog = sq_extract_archive_dialog_new(lp_support, lp_archive, FALSE);
			result = gtk_dialog_run (GTK_DIALOG (dialog) );
			if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
			{
				gtk_widget_destroy (GTK_WIDGET (dialog) );
				lsq_close_archive(lp_archive);
			}
			if(result == GTK_RESPONSE_OK)
			{
				dest_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
				lsq_archive_support_extract(lp_support, lp_archive, dest_path, NULL);
				g_free(dest_path);
				dest_path = NULL;
			}
		}
		else
			lsq_archive_support_extract(lp_support, lp_archive, dest_path, NULL);
	}
	g_object_ref(app);
	return 0;
}

gint
sq_application_new_archive(SQApplication *app, gchar *archive_path, GSList *files)
{
	GtkWidget *dialog = NULL;
	gint result = 0;
	LSQArchive *lp_archive = NULL;
	LSQArchiveSupport *lp_support = NULL;

	if(!archive_path)
	{
		dialog = sq_new_archive_dialog_new();
		/* FIXME, does not work correctly when there are more dots in a filename then the one identifying the extention */
		gchar **filename_components = g_strsplit(files->data, ".", 2);
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename_components[0]);
		g_strfreev(filename_components);
		result = gtk_dialog_run (GTK_DIALOG (dialog) );
		if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
		{
			gtk_widget_destroy (GTK_WIDGET (dialog) );
			return 2;
		}
		if(result == GTK_RESPONSE_OK)
		{
			archive_path = sq_new_archive_dialog_get_filename(SQ_NEW_ARCHIVE_DIALOG(dialog));
			gtk_widget_destroy (GTK_WIDGET (dialog) );
		}
		if(lsq_new_archive(archive_path, TRUE, NULL, &lp_archive))
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
		g_free(archive_path);
		archive_path = NULL;
	}
	else
	{
		if(lsq_open_archive(archive_path, &lp_archive))
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
	}
	g_signal_connect(G_OBJECT(lp_archive), "lsq_status_changed", G_CALLBACK(cb_sq_application_archive_status_changed), app);
	lp_support = lsq_get_support_for_mime(lp_archive->mime_info);
	lsq_archive_support_add(lp_support, lp_archive, files);
	g_object_ref(app);
	return 0;
}

gint
sq_application_open_archive(SQApplication *app, GtkWidget *window, gchar *path)
{
	gint retval = 0;

	if(!window)
	{
		window = sq_application_new_window(app);
	}
	if(app->props._tabs)
	{
		retval = sq_main_window_open_archive(SQ_MAIN_WINDOW(window), path, -1);
	}
	else
	{
		retval = sq_main_window_open_archive(SQ_MAIN_WINDOW(window), path, 0);
	}
	gtk_widget_show(window);
	return retval;
}
