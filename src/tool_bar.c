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
#include "archive_store.h"
#include "navigation_bar.h"
#include "tool_bar.h"


static void
xa_tool_bar_class_init(XAToolBarClass *archive_class);

static void
xa_tool_bar_init(XAToolBar *archive);

static void
cb_xa_tool_bar_pwd_changed(XAArchiveStore *store, XAToolBar *bar);

GType
xa_tool_bar_get_type ()
{
	static GType xa_tool_bar_type = 0;

 	if (!xa_tool_bar_type)
	{
 		static const GTypeInfo xa_tool_bar_info = 
		{
			sizeof (XAToolBarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_tool_bar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAToolBar),
			0,
			(GInstanceInitFunc) xa_tool_bar_init,
			NULL
		};

		xa_tool_bar_type = g_type_register_static (XA_TYPE_NAVIGATION_BAR, "XAToolBar", &xa_tool_bar_info, 0);
	}
	return xa_tool_bar_type;
}

static void
xa_tool_bar_class_init(XAToolBarClass *tool_bar_class)
{
}

static void
xa_tool_bar_init(XAToolBar *tool_bar)
{
	GtkToolItem *button = NULL;
	XA_NAVIGATION_BAR(tool_bar)->_cb_pwd_changed = (GCallback)cb_xa_tool_bar_pwd_changed;
	gtk_toolbar_set_style(GTK_TOOLBAR(tool_bar), GTK_TOOLBAR_ICONS);

	tool_bar->back_button = gtk_tool_button_new_from_stock("gtk-go-back");
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), tool_bar->back_button, 0);

	tool_bar->forward_button = gtk_tool_button_new_from_stock("gtk-go-forward");
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), tool_bar->forward_button, 1);

	tool_bar->up_button = gtk_tool_button_new_from_stock("gtk-go-up");
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), tool_bar->up_button, 2);

	button = gtk_tool_button_new_from_stock("gtk-home");
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), button, 3);

	button = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), button, 4);

	button = gtk_tool_item_new();
	tool_bar->path_field = gtk_entry_new();

	gtk_container_add(GTK_CONTAINER(button), tool_bar->path_field);
	gtk_tool_item_set_visible_horizontal(button, TRUE);
	gtk_tool_item_set_homogeneous(button, TRUE);

	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), button, 5);
	gtk_widget_show_all(GTK_WIDGET(button));
	gtk_widget_show(GTK_WIDGET(tool_bar->path_field));

}

XANavigationBar *
xa_tool_bar_new(XAArchiveStore *store)
{
	XANavigationBar *bar;

	bar = g_object_new(XA_TYPE_TOOL_BAR, NULL);

	if(store)
		xa_navigation_bar_set_store(XA_NAVIGATION_BAR(bar), store);

	return bar;
}

static void
cb_xa_tool_bar_pwd_changed(XAArchiveStore *store, XAToolBar *tool_bar)
{
	gchar *path= xa_archive_store_get_pwd(store);
	xa_navigation_bar_history_push(XA_NAVIGATION_BAR(tool_bar), path);
	g_free(path);

	GSList *path_list = xa_archive_store_get_pwd_list(store);
	if(g_slist_length(path_list) == 1)
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->up_button), 0);
	else
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->up_button), 1);
	if(xa_navigation_bar_history_get_length(XA_NAVIGATION_BAR(tool_bar)) <= 1)
	{
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), 0);
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), 0);
	}
	else
	{

	}
}

