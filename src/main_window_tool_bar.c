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
#include "main_window_tool_bar.h"
#include "main_window.h"

#include "add_dialog.h"
#include "new_dialog.h"
#include "extract_dialog.h"

static void
xa_main_window_tool_bar_class_init(XAMainWindowToolBarClass *);

static void
xa_main_window_tool_bar_init(XAMainWindowToolBar *);

GType
xa_main_window_tool_bar_get_type ()
{
	static GType xa_main_window_tool_bar_type = 0;

 	if (!xa_main_window_tool_bar_type)
	{
 		static const GTypeInfo xa_main_window_tool_bar_info = 
		{
			sizeof (XAMainWindowToolBarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_main_window_tool_bar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAMainWindowToolBar),
			0,
			(GInstanceInitFunc) xa_main_window_tool_bar_init,
			NULL
		};

		xa_main_window_tool_bar_type = g_type_register_static (GTK_TYPE_TOOLBAR, "XAMainWindowToolBar", &xa_main_window_tool_bar_info, 0);
	}
	return xa_main_window_tool_bar_type;
}

static void
xa_main_window_tool_bar_class_init(XAMainWindowToolBarClass *toolbar_class)
{
}

static void
xa_main_window_tool_bar_init(XAMainWindowToolBar *toolbar)
{
	GtkToolItem *separator;
	GtkWidget *tmp_image;
	
	/* contents of 'archive' pane */
	toolbar->tool_item_new = gtk_tool_button_new_from_stock("gtk-new");
	toolbar->tool_item_open = gtk_tool_button_new_from_stock("gtk-open");
	separator = gtk_separator_tool_item_new ();

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(toolbar->tool_item_new));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(toolbar->tool_item_open));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(separator));

	/* contents of 'action' pane */
	tmp_image = xa_main_window_find_image("add.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
	toolbar->tool_item_add = gtk_tool_button_new(tmp_image, _("Add"));

	tmp_image = xa_main_window_find_image("extract.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
	toolbar->tool_item_extract = gtk_tool_button_new(tmp_image, _("Extract"));

	tmp_image = gtk_image_new_from_stock("gtk-delete", GTK_ICON_SIZE_LARGE_TOOLBAR);
	toolbar->tool_item_remove = gtk_tool_button_new(tmp_image, _("Delete"));

	separator = gtk_separator_tool_item_new ();

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(toolbar->tool_item_add));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(toolbar->tool_item_extract));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(toolbar->tool_item_remove));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(separator));
}

GtkWidget *
xa_main_window_tool_bar_new()
{
	GtkWidget *toolbar;

	toolbar = g_object_new(xa_main_window_tool_bar_get_type(), NULL);

	return toolbar;
}
