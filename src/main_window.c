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
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>
#include "main_window_menu_bar.h"
#include "main_window_tool_bar.h"
#include "main_window_status_bar.h"
#include "main_window.h"
#include "new_dialog.h"

#include "main.h"

static void
xa_main_window_class_init(XAMainWindowClass *);

static void
xa_main_window_init(XAMainWindow *);

GType
xa_main_window_get_type ()
{
	static GType xa_main_window_type = 0;

 	if (!xa_main_window_type)
	{
 		static const GTypeInfo xa_main_window_info = 
		{
			sizeof (XAMainWindowClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_main_window_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAMainWindow),
			0,
			(GInstanceInitFunc) xa_main_window_init,
			NULL
		};

		xa_main_window_type = g_type_register_static (GTK_TYPE_WINDOW, "XAMainWindow", &xa_main_window_info, 0);
	}
	return xa_main_window_type;
}

static void
xa_main_window_class_init(XAMainWindowClass *window_class)
{
}

static void
xa_main_window_init(XAMainWindow *window)
{
	gtk_window_set_default_icon_from_file(DATADIR "/pixmaps/xarchiver.png", NULL);
	window->main_vbox = gtk_vbox_new(FALSE, 0);
	window->menubar = xa_main_window_menu_bar_new();
	window->toolbar = xa_main_window_tool_bar_new();
	window->statusbar = xa_main_window_status_bar_new();

	gtk_box_pack_start(GTK_BOX(window->main_vbox), window->menubar, 0, FALSE, FALSE);
	gtk_box_pack_start(GTK_BOX(window->main_vbox), window->toolbar, 0, FALSE, FALSE);
	gtk_box_pack_end(GTK_BOX(window->main_vbox), window->statusbar, 0, FALSE, FALSE);

	gtk_widget_show(window->main_vbox);
	gtk_widget_show_all(window->menubar);
	gtk_widget_show_all(window->toolbar);
	gtk_widget_show_all(window->statusbar);

	gtk_container_add(GTK_CONTAINER(window), window->main_vbox);
}

GtkWidget *
xa_main_window_new()
{
	GtkWidget *window;

	window = g_object_new(xa_main_window_get_type(),
			"title", "Xarchiver " PACKAGE_VERSION,
			NULL);

	return window;
}

GtkWidget *
xa_main_window_find_image(gchar *filename, GtkIconSize size)
{
	GError *error = NULL;
	GtkWidget *file_image;
	gchar *path;
	path = g_strconcat(DATADIR, "/xarchiver/pixmaps/", filename, NULL);
	GdkPixbuf *file_pixbuf = gdk_pixbuf_new_from_file(path, &error);
	if(error)
	{
		/*
		* perhaps xarchiver has not been installed and is being executed from source dir
		*/
		g_free(error);
		error = NULL;
		path = g_strconcat("./pixmaps/", filename, NULL);
		file_pixbuf = gdk_pixbuf_new_from_file(path, &error);
	}
	if(file_pixbuf)
	{
		file_image = gtk_image_new_from_pixbuf(file_pixbuf);
		g_object_unref(file_pixbuf);
	}
	else
	{
		g_free(error);
		file_image = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, size);
	}
	g_free(path);
	return file_image;
}

void
cb_xa_main_new_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	gchar *new_archive_path = NULL;
	gint result = 0;
	
	dialog = xa_new_archive_dialog_new();
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog) );
		return;
	}
	if(result == GTK_RESPONSE_OK)
	{
		new_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if(lp_xa_archive)
		{
			g_object_unref(lp_xa_archive);
			lp_xa_archive = NULL;
		}
		if(!lxa_new_archive(new_archive_path, TRUE, NULL, &lp_xa_archive))
		{
			g_debug("Archive opened");
		}
		gtk_widget_destroy (GTK_WIDGET (dialog) );
	}
}

void
cb_xa_main_open_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	gchar *open_archive_path = NULL;
	gint result = 0;
	
	dialog = gtk_file_chooser_dialog_new(_("Open archive"), 
	                                     GTK_WINDOW(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)),
																			 GTK_FILE_CHOOSER_ACTION_OPEN,
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
																			 GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog) );
		return;
	}
	if(result == GTK_RESPONSE_OK)
	{
		open_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if(lp_xa_archive)
		{
			g_object_unref(lp_xa_archive);
			lp_xa_archive = NULL;
		}
		if(!lxa_open_archive(open_archive_path, &lp_xa_archive))
		{
			g_debug("Archive opened");
		}
		gtk_widget_destroy (GTK_WIDGET (dialog) );
	}

}
