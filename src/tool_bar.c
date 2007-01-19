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
#include "archive_store.h"
#include "navigation_bar.h"
#include "tool_bar.h"


static void
sq_tool_bar_class_init(SQToolBarClass *archive_class);

static void
sq_tool_bar_init(SQToolBar *archive);

static void
sq_tool_bar_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void
sq_tool_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation);

static GType
sq_tool_bar_child_type(GtkContainer *container);
static void
sq_tool_bar_add(GtkContainer *container, GtkWidget *child);
static void
sq_tool_bar_remove(GtkContainer *container, GtkWidget *child);
static void
sq_tool_bar_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data);

static void
cb_sq_tool_bar_pwd_changed(SQArchiveStore *store, SQNavigationBar *bar);
static void
cb_sq_tool_bar_new_archive(SQArchiveStore *store, SQNavigationBar *bar);
static void
cb_sq_tool_bar_store_set(SQNavigationBar *bar);

static void
cb_sq_tool_bar_history_back(GtkWidget *button, SQToolBar *nav_bar);
static void
cb_sq_tool_bar_history_forward(GtkWidget *forward_button, SQToolBar *nav_bar);

static void
cb_sq_tool_bar_up(GtkWidget *, SQToolBar *tool_bar);
static void
cb_sq_tool_bar_home(GtkWidget *, SQToolBar *tool_bar);

static void
cb_sq_tool_bar_path_field_activated(GtkWidget *entry, SQToolBar *tool_bar);

GType
sq_tool_bar_get_type ()
{
	static GType sq_tool_bar_type = 0;

 	if (!sq_tool_bar_type)
	{
 		static const GTypeInfo sq_tool_bar_info = 
		{
			sizeof (SQToolBarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_tool_bar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQToolBar),
			0,
			(GInstanceInitFunc) sq_tool_bar_init,
			NULL
		};

		sq_tool_bar_type = g_type_register_static (SQ_TYPE_NAVIGATION_BAR, "SQToolBar", &sq_tool_bar_info, 0);
	}
	return sq_tool_bar_type;
}

static void
sq_tool_bar_class_init(SQToolBarClass *tool_bar_class)
{
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	widget_class = (GtkWidgetClass*)tool_bar_class;
	container_class = (GtkContainerClass*)tool_bar_class;

	widget_class->size_request = sq_tool_bar_size_request;
	widget_class->size_allocate = sq_tool_bar_size_allocate;

	container_class->add = sq_tool_bar_add;
	container_class->remove = sq_tool_bar_remove;
	container_class->forall = sq_tool_bar_forall;
	container_class->child_type = sq_tool_bar_child_type;
}

static void
sq_tool_bar_init(SQToolBar *tool_bar)
{
	GtkToolItem *button = NULL;
	SQ_NAVIGATION_BAR(tool_bar)->_cb_pwd_changed = cb_sq_tool_bar_pwd_changed;
	SQ_NAVIGATION_BAR(tool_bar)->_cb_new_archive = cb_sq_tool_bar_new_archive;
	SQ_NAVIGATION_BAR(tool_bar)->_cb_store_set   = cb_sq_tool_bar_store_set;

	GTK_WIDGET_SET_FLAGS(tool_bar, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate(GTK_WIDGET(tool_bar), FALSE);

	tool_bar->bar = GTK_TOOLBAR(gtk_toolbar_new());
	gtk_toolbar_set_style(GTK_TOOLBAR(tool_bar->bar), GTK_TOOLBAR_ICONS);
	gtk_container_add(GTK_CONTAINER(tool_bar), GTK_WIDGET(tool_bar->bar));
	gtk_widget_show(GTK_WIDGET(tool_bar->bar));

	tool_bar->back_button = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), tool_bar->back_button, 0);
	g_signal_connect(G_OBJECT(tool_bar->back_button), "clicked", (GCallback)cb_sq_tool_bar_history_back, tool_bar);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), 0);

	tool_bar->forward_button = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), tool_bar->forward_button, 1);
	g_signal_connect(G_OBJECT(tool_bar->forward_button), "clicked", (GCallback)cb_sq_tool_bar_history_forward, tool_bar);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), 0);

	tool_bar->up_button = gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), tool_bar->up_button, 2);
	g_signal_connect(G_OBJECT(tool_bar->up_button), "clicked", (GCallback)cb_sq_tool_bar_up, tool_bar);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->up_button), 0);

	tool_bar->home_button = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), tool_bar->home_button, 3);
	g_signal_connect(G_OBJECT(tool_bar->home_button), "clicked", (GCallback)cb_sq_tool_bar_home, tool_bar);
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
	g_signal_connect(G_OBJECT(tool_bar->path_field), "activate", (GCallback)cb_sq_tool_bar_path_field_activated, tool_bar);
	gtk_tool_item_set_visible_horizontal(button, TRUE);
	gtk_tool_item_set_homogeneous(button, FALSE);

	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar->bar), button, 5);
	gtk_widget_show_all(GTK_WIDGET(button));
	gtk_widget_show(GTK_WIDGET(tool_bar->path_field));
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), 0);

	gtk_widget_show_all(GTK_WIDGET(tool_bar->bar));
	gtk_widget_ref((GtkWidget *)tool_bar);
}

SQNavigationBar *
sq_tool_bar_new(SQArchiveStore *store)
{
	SQNavigationBar *bar;

	bar = g_object_new(SQ_TYPE_TOOL_BAR, NULL);

	if(store)
		sq_navigation_bar_set_store(SQ_NAVIGATION_BAR(bar), store);

	return bar;
}

static void
sq_tool_bar_refresh(SQToolBar *tool_bar, gchar *path)
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
	if(SQ_NAVIGATION_BAR(tool_bar)->store)
	{
		if(sq_archive_store_has_future(SQ_NAVIGATION_BAR(tool_bar)->store))
			gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), 1);
		else
			gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), 0);

		if(sq_archive_store_has_history(SQ_NAVIGATION_BAR(tool_bar)->store))
			gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), 1);
		else
			gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), 0);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), 0);
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), 0);
	}
}

static void
sq_tool_bar_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	SQToolBar *tool_bar = SQ_TOOL_BAR(widget);

	if(tool_bar->bar && GTK_WIDGET_VISIBLE(tool_bar->bar))
		gtk_widget_size_request(GTK_WIDGET(tool_bar->bar), requisition);

	if(requisition->width < 400)
		requisition->width = 400;

	widget->requisition = *requisition;
}

static void
sq_tool_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	SQToolBar *tool_bar = SQ_TOOL_BAR(widget);

	widget->allocation = *allocation;

	if(tool_bar->bar && GTK_WIDGET_VISIBLE(tool_bar->bar))
		gtk_widget_size_allocate(GTK_WIDGET(tool_bar->bar), allocation);
}

static GType
sq_tool_bar_child_type(GtkContainer *container)
{
	if(!SQ_TOOL_BAR(container)->bar)
		return GTK_TYPE_WIDGET;
	else
		return G_TYPE_NONE;
}

static void
sq_tool_bar_add(GtkContainer *container, GtkWidget *child)
{
	SQToolBar *tool_bar = SQ_TOOL_BAR(container);

	g_return_if_fail(GTK_IS_WIDGET(child));

	if(((GtkWidget*)tool_bar->bar) != child)
	{
		g_warning("DON'T set another child as toolbar");
		return;
	}

	gtk_widget_set_parent(child, GTK_WIDGET(tool_bar));
}

static void
sq_tool_bar_remove(GtkContainer *container, GtkWidget *child)
{
	SQToolBar *tool_bar = SQ_TOOL_BAR(container);
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
sq_tool_bar_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
	SQToolBar *tool_bar = SQ_TOOL_BAR(container);

	g_return_if_fail(callback != NULL);

	if(tool_bar->bar)
		(* callback)(GTK_WIDGET(tool_bar->bar), callback_data);
}

static void
cb_sq_tool_bar_pwd_changed(SQArchiveStore *store, SQNavigationBar *bar)
{
	SQToolBar *tool_bar = SQ_TOOL_BAR(bar);
	gchar *path= sq_archive_store_get_pwd(store);
	if(!path)
		path = g_strdup("");
	sq_tool_bar_refresh(tool_bar, path);
	g_free(path);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), TRUE);
}

static void
cb_sq_tool_bar_new_archive(SQArchiveStore *store, SQNavigationBar *bar)
{
	SQToolBar *tool_bar = SQ_TOOL_BAR(bar);

	LSQArchive *lp_archive = sq_archive_store_get_archive(store);

	if(!lp_archive)
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), FALSE);
	else
	{
		if(lsq_archive_get_status(lp_archive) != LSQ_ARCHIVESTATUS_IDLE)
			gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), FALSE);
		else
			gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), TRUE);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->up_button), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->home_button), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->forward_button), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->back_button), FALSE);
}

static void
cb_sq_tool_bar_history_back(GtkWidget *back_button, SQToolBar *tool_bar)
{
	sq_archive_store_go_back(SQ_NAVIGATION_BAR(tool_bar)->store);
	gchar *path= sq_archive_store_get_pwd(SQ_NAVIGATION_BAR(tool_bar)->store);
	if(!path)
		path = g_strdup("");
	sq_tool_bar_refresh(tool_bar, path);
	g_free(path);
}

static void
cb_sq_tool_bar_history_forward(GtkWidget *forward_button, SQToolBar *tool_bar)
{
	sq_archive_store_go_forward(SQ_NAVIGATION_BAR(tool_bar)->store);
	gchar *path= sq_archive_store_get_pwd(SQ_NAVIGATION_BAR(tool_bar)->store);
	if(!path)
		path = g_strdup("");
	sq_tool_bar_refresh(tool_bar, path);
	g_free(path);
}

static void
cb_sq_tool_bar_up(GtkWidget *forward_button, SQToolBar *tool_bar)
{
	sq_archive_store_go_up(SQ_NAVIGATION_BAR(tool_bar)->store);
}

static void
cb_sq_tool_bar_home(GtkWidget *forward_button, SQToolBar *tool_bar)
{
	/* FIXME: the part about "/" could be bugged */
	sq_archive_store_set_pwd(SQ_NAVIGATION_BAR(tool_bar)->store, "");
}

static void
cb_sq_tool_bar_path_field_activated(GtkWidget *entry, SQToolBar *tool_bar)
{
	const gchar *path = gtk_entry_get_text(GTK_ENTRY(entry));
	sq_archive_store_set_pwd(SQ_ARCHIVE_STORE(SQ_NAVIGATION_BAR(tool_bar)->store), path);
}

static void
cb_sq_tool_bar_store_set(SQNavigationBar *bar)
{
	SQToolBar *tool_bar = SQ_TOOL_BAR(bar);
	if(bar->store)
	{
		gchar *path = sq_archive_store_get_pwd(bar->store);
		if(!path)
			path = g_strdup("");
		sq_tool_bar_refresh(tool_bar, path);
		if(bar->store->archive->status == LSQ_ARCHIVESTATUS_IDLE)
			gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), TRUE);
		else
			gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), FALSE);
		g_free(path);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(tool_bar->hbox), FALSE);
		sq_tool_bar_refresh(tool_bar, "");
	}
}
