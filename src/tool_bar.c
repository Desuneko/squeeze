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
#include "archive_store.h"
#include "navigation_bar.h"
#include "tool_bar.h"


static void
xa_tool_bar_class_init(XAToolBarClass *archive_class);

static void
xa_tool_bar_init(XAToolBar *archive);

static void
xa_tool_bar_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void
xa_tool_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation);

static GType
xa_tool_bar_child_type(GtkContainer *container);
static void
xa_tool_bar_add(GtkContainer *container, GtkWidget *child);
static void
xa_tool_bar_remove(GtkContainer *container, GtkWidget *child);
static void
xa_tool_bar_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data);

static void
cb_xa_tool_bar_pwd_changed(XAArchiveStore *store, XANavigationBar *bar);
static void
cb_xa_tool_bar_new_archive(XAArchiveStore *store, XANavigationBar *bar);
static void
cb_xa_tool_bar_store_set(XANavigationBar *bar);

static void
cb_xa_tool_bar_history_back(GtkWidget *button, XAToolBar *nav_bar);
static void
cb_xa_tool_bar_history_forward(GtkWidget *forward_button, XAToolBar *nav_bar);

static void
cb_xa_tool_bar_up(GtkWidget *, XAToolBar *tool_bar);
static void
cb_xa_tool_bar_home(GtkWidget *, XAToolBar *tool_bar);

static void
cb_xa_tool_bar_path_field_activated(GtkWidget *entry, XAToolBar *tool_bar);

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
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	widget_class = (GtkWidgetClass*)tool_bar_class;
	container_class = (GtkContainerClass*)tool_bar_class;

	widget_class->size_request = xa_tool_bar_size_request;
	widget_class->size_allocate = xa_tool_bar_size_allocate;

	container_class->add = xa_tool_bar_add;
	container_class->remove = xa_tool_bar_remove;
	container_class->forall = xa_tool_bar_forall;
	container_class->child_type = xa_tool_bar_child_type;
}

static void
xa_tool_bar_init(XAToolBar *tool_bar)
{
	GtkToolItem *button = NULL;
	XA_NAVIGATION_BAR(tool_bar)->_cb_pwd_changed = cb_xa_tool_bar_pwd_changed;
	XA_NAVIGATION_BAR(tool_bar)->_cb_new_archive = cb_xa_tool_bar_new_archive;
	XA_NAVIGATION_BAR(tool_bar)->_cb_store_set   = cb_xa_tool_bar_store_set;

	GTK_WIDGET_SET_FLAGS(tool_bar, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate(GTK_WIDGET(tool_bar), FALSE);

	tool_bar->bar = GTK_TOOLBAR(gtk_toolbar_new());
	gtk_toolbar_set_style(GTK_TOOLBAR(tool_bar->bar), GTK_TOOLBAR_ICONS);
	gtk_container_add(GTK_CONTAINER(tool_bar), GTK_WIDGET(tool_bar->bar));
	gtk_widget_show(GTK_WIDGET(tool_bar->bar));

	tool_bar->back_button = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), tool_bar->back_button, 0);
	g_signal_connect(G_OBJECT(tool_bar->back_button), "clicked", (GCallback)cb_xa_tool_bar_history_back, tool_bar);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), 0);

	tool_bar->forward_button = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), tool_bar->forward_button, 1);
	g_signal_connect(G_OBJECT(tool_bar->forward_button), "clicked", (GCallback)cb_xa_tool_bar_history_forward, tool_bar);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), 0);

	tool_bar->up_button = gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), tool_bar->up_button, 2);
	g_signal_connect(G_OBJECT(tool_bar->up_button), "clicked", (GCallback)cb_xa_tool_bar_up, tool_bar);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->up_button), 0);

	tool_bar->home_button = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), tool_bar->home_button, 3);
	g_signal_connect(G_OBJECT(tool_bar->home_button), "clicked", (GCallback)cb_xa_tool_bar_home, tool_bar);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->home_button), 0);

	button = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), button, 4);

	button = gtk_tool_item_new();
	tool_bar->path_field = gtk_entry_new();
	gtk_tool_item_set_expand(button, TRUE);
	tool_bar->hbox = gtk_hbox_new(FALSE, 0);

	gtk_container_add(GTK_CONTAINER(button), tool_bar->hbox);
	gtk_box_pack_start(GTK_BOX(tool_bar->hbox), gtk_label_new(_("Location:")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tool_bar->hbox), tool_bar->path_field, TRUE, TRUE, 5);
	g_signal_connect(G_OBJECT(tool_bar->path_field), "activate", (GCallback)cb_xa_tool_bar_path_field_activated, tool_bar);
	gtk_tool_item_set_visible_horizontal(button, TRUE);
	gtk_tool_item_set_homogeneous(button, FALSE);

	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), button, 5);
	gtk_widget_show_all(GTK_WIDGET(button));
	gtk_widget_show(GTK_WIDGET(tool_bar->path_field));
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), 0);

	gtk_widget_show_all(GTK_WIDGET(tool_bar->bar));

	g_signal_connect(G_OBJECT(tool_bar), "xa-store-set", (GCallback)cb_xa_tool_bar_store_set, NULL);
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
xa_tool_bar_refresh(XAToolBar *tool_bar, gchar *path)
{
	gtk_entry_set_text(GTK_ENTRY(tool_bar->path_field), path);
	gtk_editable_set_position(GTK_EDITABLE(tool_bar->path_field), -1);

	/* FIXME: the part about path[0] '/' could be bugged */
	if(strlen(path) < 1 || (strlen(path) == 1 && path[0] == '/'))
	{
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->up_button), 0);
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->home_button), 0);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->up_button), 1);
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->home_button), 1);
	}
	if(xa_archive_store_has_future(XA_NAVIGATION_BAR(tool_bar)->store))
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), 1);
	else
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), 0);

	if(xa_archive_store_has_history(XA_NAVIGATION_BAR(tool_bar)->store))
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), 1);
	else
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), 0);
}

static void
xa_tool_bar_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	XAToolBar *tool_bar = XA_TOOL_BAR(widget);

	if(tool_bar->bar && GTK_WIDGET_VISIBLE(tool_bar->bar))
		gtk_widget_size_request(GTK_WIDGET(tool_bar->bar), requisition);

	if(requisition->width < 400)
		requisition->width = 400;

	widget->requisition = *requisition;
}

static void
xa_tool_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	XAToolBar *tool_bar = XA_TOOL_BAR(widget);

	widget->allocation = *allocation;

	if(tool_bar->bar && GTK_WIDGET_VISIBLE(tool_bar->bar))
		gtk_widget_size_allocate(GTK_WIDGET(tool_bar->bar), allocation);
}

static GType
xa_tool_bar_child_type(GtkContainer *container)
{
	if(!XA_TOOL_BAR(container)->bar)
		return GTK_TYPE_WIDGET;
	else
		return G_TYPE_NONE;
}

static void
xa_tool_bar_add(GtkContainer *container, GtkWidget *child)
{
	XAToolBar *tool_bar = XA_TOOL_BAR(container);

	g_return_if_fail(GTK_IS_WIDGET(child));

	if(((GtkWidget*)tool_bar->bar) != child)
	{
		g_warning("DON'T set another child as toolbar");
		return;
	}

	gtk_widget_set_parent(child, GTK_WIDGET(tool_bar));
}

static void
xa_tool_bar_remove(GtkContainer *container, GtkWidget *child)
{
	XAToolBar *tool_bar = XA_TOOL_BAR(container);
	gboolean widget_was_visible;

	g_return_if_fail(GTK_IS_WIDGET(child));
	g_return_if_fail(((GtkWidget*)tool_bar->bar) == child);

	widget_was_visible = GTK_WIDGET_VISIBLE(child);

	gtk_widget_unparent(child);
	tool_bar->bar = NULL;

	if(widget_was_visible)
		gtk_widget_queue_resize(GTK_WIDGET(container));
}

static void
xa_tool_bar_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
	XAToolBar *tool_bar = XA_TOOL_BAR(container);

	g_return_if_fail(callback != NULL);

	if(tool_bar->bar)
		(* callback)(GTK_WIDGET(tool_bar->bar), callback_data);
}

static void
cb_xa_tool_bar_pwd_changed(XAArchiveStore *store, XANavigationBar *bar)
{
	XAToolBar *tool_bar = XA_TOOL_BAR(bar);
	gchar *path= xa_archive_store_get_pwd(store);

	xa_tool_bar_refresh(tool_bar, path);
	g_free(path);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), TRUE);
}

static void
cb_xa_tool_bar_new_archive(XAArchiveStore *store, XANavigationBar *bar)
{
	XAToolBar *tool_bar = XA_TOOL_BAR(bar);

	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->up_button), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->home_button), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), FALSE);
}

static void
cb_xa_tool_bar_history_back(GtkWidget *back_button, XAToolBar *tool_bar)
{
	xa_archive_store_go_back(XA_NAVIGATION_BAR(tool_bar)->store);
	gchar *path= xa_archive_store_get_pwd(XA_NAVIGATION_BAR(tool_bar)->store);
	xa_tool_bar_refresh(tool_bar, path);
	g_free(path);
}

static void
cb_xa_tool_bar_history_forward(GtkWidget *forward_button, XAToolBar *tool_bar)
{
	xa_archive_store_go_forward(XA_NAVIGATION_BAR(tool_bar)->store);
	gchar *path= xa_archive_store_get_pwd(XA_NAVIGATION_BAR(tool_bar)->store);
	xa_tool_bar_refresh(tool_bar, path);
	g_free(path);
}

static void
cb_xa_tool_bar_up(GtkWidget *forward_button, XAToolBar *tool_bar)
{
	xa_archive_store_go_up(XA_NAVIGATION_BAR(tool_bar)->store);
}

static void
cb_xa_tool_bar_home(GtkWidget *forward_button, XAToolBar *tool_bar)
{
	/* FIXME: the part about "/" could be bugged */
	xa_archive_store_set_pwd(XA_NAVIGATION_BAR(tool_bar)->store, "/");
}

static void
cb_xa_tool_bar_path_field_activated(GtkWidget *entry, XAToolBar *tool_bar)
{
	const gchar *path = gtk_entry_get_text(GTK_ENTRY(entry));
	xa_archive_store_set_pwd(XA_ARCHIVE_STORE(XA_NAVIGATION_BAR(tool_bar)->store), path);
}

static void
cb_xa_tool_bar_store_set(XANavigationBar *bar)
{
	XAToolBar *tool_bar = XA_TOOL_BAR(bar);
	if(bar->store)
	{
		gchar *path= xa_archive_store_get_pwd(bar->store);
		xa_tool_bar_refresh(tool_bar, path);
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), TRUE);
		g_free(path);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), FALSE);
		xa_tool_bar_refresh(tool_bar, "");
	}
}
