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
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>

#include <gettext.h>

#include "new_dialog.h"
#include "add_dialog.h"
#include "extract_dialog.h"

#include "settings.h"
#include "archive_store.h"
#include "navigation_bar.h"
#include "application.h"
#include "main_window.h"

static void
xa_application_class_init(XAApplicationClass *archive_class);

static void
xa_application_init(XAApplication *archive);
static void
xa_application_finalize(GObject *object);
static void
xa_application_dispose(GObject *object);

static gint xa_application_signals[1];

GType
xa_application_get_type ()
{
	static GType xa_application_type = 0;

 	if (!xa_application_type)
	{
 		static const GTypeInfo xa_application_info = 
		{
			sizeof (XAApplicationClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_application_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAApplication),
			0,
			(GInstanceInitFunc) xa_application_init,
			NULL
		};

		xa_application_type = g_type_register_static (G_TYPE_OBJECT, "XAApplication", &xa_application_info, 0);
	}
	return xa_application_type;
}

/* Destroy code was copied from gtk+-2.8.16 */
static void
xa_application_class_init(XAApplicationClass *application_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (application_class);
	object_class->finalize     = xa_application_finalize;
	object_class->dispose      = xa_application_dispose;

	xa_application_signals[0] = g_signal_new("destroy",
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
xa_application_init(XAApplication *application)
{
	application->settings = xa_settings_new();
	xa_settings_set_group(application->settings, "Global");

	application->props._tabs = xa_settings_read_bool_entry(application->settings, "UseTabs", TRUE);
}

static void
xa_application_dispose(GObject *object)
{
	g_signal_emit(object, xa_application_signals[0], 0, object);
}

static void
xa_application_finalize(GObject *object)
{
#ifdef DEBUG
	g_debug("Application Destroyed");
#endif
}

XAApplication *
xa_application_new(GtkIconTheme *icon_theme)
{
	XAApplication *app;

	app = g_object_new(XA_TYPE_APPLICATION, NULL);

	app->icon_theme = icon_theme;

	return app;
}

GtkWidget *
xa_application_new_window(XAApplication *app)
{
	GtkWidget *window = xa_main_window_new(app, app->icon_theme);
	gtk_widget_set_size_request(window, 500, 300);
	return window;
}

void
cb_xa_application_archive_status_changed(LXAArchive *archive, gpointer data)
{
	XAApplication *app = XA_APPLICATION(data);

	switch(archive->status)
	{
		case LXA_ARCHIVESTATUS_IDLE:
			lxa_close_archive(archive);
		case LXA_ARCHIVESTATUS_ERROR:
		case LXA_ARCHIVESTATUS_USERBREAK:
			g_object_unref(app);
			break;
		default:
			break;
	}
}

gint
xa_application_extract_archive(XAApplication *app, gchar *archive_path, gchar *dest_path)
{
	GtkWidget *dialog = NULL;
	gint result = 0;
	LXAArchive *lp_archive = NULL;
	LXAArchiveSupport *lp_support = NULL;

	if(!lxa_open_archive(archive_path, &lp_archive))
	{
		g_signal_connect(G_OBJECT(lp_archive), "lxa_status_changed", G_CALLBACK(cb_xa_application_archive_status_changed), app);
		lp_support = lxa_get_support_for_mime(lxa_mime_info_get_name(lp_archive->mime_info));
		if(!dest_path)
		{
			dialog = xa_extract_archive_dialog_new(lp_support, lp_archive, FALSE);
			result = gtk_dialog_run (GTK_DIALOG (dialog) );
			if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
			{
				gtk_widget_destroy (GTK_WIDGET (dialog) );
				lxa_close_archive(lp_archive);
			}
			if(result == GTK_RESPONSE_OK)
			{
				dest_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
				lxa_archive_support_extract(lp_support, lp_archive, dest_path, NULL);
				g_free(dest_path);
				dest_path = NULL;
			}
		}
		else
			lxa_archive_support_extract(lp_support, lp_archive, dest_path, NULL);
	}
	g_object_ref(app);
	return 0;
}

gint
xa_application_new_archive(XAApplication *app, gchar *archive_path, GSList *files)
{
	GtkWidget *dialog = NULL;
	gint result = 0;
	LXAArchive *lp_archive = NULL;
	LXAArchiveSupport *lp_support = NULL;

	if(!archive_path)
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
			archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gtk_widget_destroy (GTK_WIDGET (dialog) );
		}
		if(lxa_new_archive(archive_path, TRUE, NULL, &lp_archive))
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
		if(lxa_open_archive(archive_path, &lp_archive))
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
	g_signal_connect(G_OBJECT(lp_archive), "lxa_status_changed", G_CALLBACK(cb_xa_application_archive_status_changed), NULL);
	lp_support = lxa_get_support_for_mime(lxa_mime_info_get_name(lp_archive->mime_info));
	lxa_archive_support_add(lp_support, lp_archive, files);
	g_object_ref(app);
	return 0;
}

gint
xa_application_open_archive(XAApplication *app, GtkWidget *window, gchar *path)
{
	if(!window)
	{
		window = xa_application_new_window(app);
	}
	if(app->props._tabs)
		xa_main_window_open_archive(XA_MAIN_WINDOW(window), path, -1);
	else
		xa_main_window_open_archive(XA_MAIN_WINDOW(window), path, 0);
	gtk_widget_show_all(window);
	return 0;
}
