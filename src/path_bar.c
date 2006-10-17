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
#include <gettext.h>
#include <libxarchiver/libxarchiver.h>
#include "archive_store.h"
#include "navigation_bar.h"
#include "path_bar.h"


static void
xa_path_bar_class_init(XAPathBarClass *archive_class);

static void
xa_path_bar_init(XAPathBar *archive);

static void
xa_path_bar_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void
xa_path_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation);

static GType
xa_path_bar_child_type(GtkContainer *container);
static void
xa_path_bar_add(GtkContainer *container, GtkWidget *child);
static void
xa_path_bar_remove(GtkContainer *container, GtkWidget *child);
static void
xa_path_bar_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data);

static void
cb_xa_path_bar_pwd_changed(XAArchiveStore *store, XAPathBar *bar);

static void
cb_xa_path_bar_path_button_clicked(GtkRadioButton *button, XAPathBar *path_bar);
static void
cb_xa_path_bar_scroll_left(GtkWidget *widget, gpointer user_data);
static void
cb_xa_path_bar_scroll_right(GtkWidget *widget, gpointer user_data);

GType
xa_path_bar_get_type ()
{
	static GType xa_path_bar_type = 0;

 	if (!xa_path_bar_type)
	{
 		static const GTypeInfo xa_path_bar_info = 
		{
			sizeof (XAPathBarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_path_bar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAPathBar),
			0,
			(GInstanceInitFunc) xa_path_bar_init,
			NULL
		};

		xa_path_bar_type = g_type_register_static (XA_TYPE_NAVIGATION_BAR, "XAPathBar", &xa_path_bar_info, 0);
	}
	return xa_path_bar_type;
}

static void
xa_path_bar_class_init(XAPathBarClass *path_bar_class)
{
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	widget_class = (GtkWidgetClass*)path_bar_class;
	container_class = (GtkContainerClass*)path_bar_class;

	widget_class->size_request = xa_path_bar_size_request;
	widget_class->size_allocate = xa_path_bar_size_allocate;

	container_class->add = xa_path_bar_add;
	container_class->remove = xa_path_bar_remove;
	container_class->forall = xa_path_bar_forall;
	container_class->child_type = xa_path_bar_child_type;

	gtk_widget_class_install_style_property (widget_class,
		g_param_spec_int ("spacing",
		_("Spacing"),
		_("The amount of space between the path buttons"),
		0, G_MAXINT, 3,
		G_PARAM_READABLE));
}

static void
xa_path_bar_init(XAPathBar *path_bar)
{
	GtkWidget *arrow;
	XA_NAVIGATION_BAR(path_bar)->_cb_pwd_changed = (GCallback)cb_xa_path_bar_pwd_changed;

	GTK_WIDGET_SET_FLAGS(path_bar, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate(GTK_WIDGET(path_bar), FALSE);

	path_bar->left_button = GTK_BUTTON(gtk_button_new());
	g_signal_connect(G_OBJECT(path_bar->left_button), "clicked", G_CALLBACK(cb_xa_path_bar_scroll_left), path_bar);
	gtk_container_add(GTK_CONTAINER(path_bar), GTK_WIDGET(path_bar->left_button));
	gtk_widget_show(GTK_WIDGET(path_bar->left_button));
	gtk_widget_ref(GTK_WIDGET(path_bar->left_button));

	arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (path_bar->left_button), arrow);
	gtk_widget_show (arrow);

	path_bar->right_button = GTK_BUTTON(gtk_button_new());
	g_signal_connect(G_OBJECT(path_bar->right_button), "clicked", G_CALLBACK(cb_xa_path_bar_scroll_right), path_bar);
	gtk_container_add(GTK_CONTAINER(path_bar), GTK_WIDGET(path_bar->right_button));
	gtk_widget_show(GTK_WIDGET(path_bar->right_button));
	gtk_widget_ref(GTK_WIDGET(path_bar->right_button));

	arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (path_bar->right_button), arrow);
	gtk_widget_show (arrow);

	path_bar->path_button = NULL;
	path_bar->first_button = NULL;

	gtk_widget_ref(GTK_WIDGET(path_bar));
}

XANavigationBar *
xa_path_bar_new(XAArchiveStore *store)
{
	XANavigationBar *bar;

	bar = g_object_new(XA_TYPE_PATH_BAR, NULL);

	if(store)
		xa_navigation_bar_set_store(XA_NAVIGATION_BAR(bar), store);

	return bar;
}

static GType
xa_path_bar_child_type(GtkContainer *container)
{
	if(!XA_PATH_BAR(container)->path_button)
		return GTK_TYPE_WIDGET;
	else
		return G_TYPE_NONE;
}

static void
xa_path_bar_add(GtkContainer *container, GtkWidget *child)
{
	XAPathBar *path_bar = XA_PATH_BAR(container);

	g_return_if_fail(GTK_IS_WIDGET(child));

	/* list is stored somewhere else */
	gtk_widget_set_parent(child, GTK_WIDGET(path_bar));
}

static void
xa_path_bar_remove(GtkContainer *container, GtkWidget *child)
{
	gboolean widget_was_visible;

	g_return_if_fail(GTK_IS_WIDGET(child));

	widget_was_visible = GTK_WIDGET_VISIBLE(child);

	gtk_widget_unparent(child);

	/* remove from list is somewhere else */
	if(widget_was_visible)
		gtk_widget_queue_resize(GTK_WIDGET(container));
}

static void
xa_path_bar_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
	XAPathBar *path_bar = XA_PATH_BAR(container);

	g_return_if_fail(callback != NULL);

	g_slist_foreach(path_bar->path_button, (GFunc)callback, callback_data);

	if(path_bar->left_button)
		(*callback)(GTK_WIDGET(path_bar->left_button), callback_data);
	if(path_bar->right_button)
		(*callback)(GTK_WIDGET(path_bar->right_button), callback_data);
}

static void
xa_path_bar_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	XAPathBar *path_bar = XA_PATH_BAR(widget);
	GSList *iter = NULL;
	GtkRequisition child_requisition;
	gint spacing = 0;
	gtk_widget_style_get(widget, "spacing", &spacing, NULL);

	requisition->width = 0;
	requisition->height = 0;

	/* get biggest button */
	for(iter = path_bar->path_button; iter; iter = iter->next)
	{
		gtk_widget_size_request(GTK_WIDGET(iter->data), &child_requisition);
		requisition->width = MAX(child_requisition.width, requisition->width);
		requisition->height = MAX(child_requisition.height, requisition->height);
	}

	gtk_widget_size_request(GTK_WIDGET(path_bar->left_button), &child_requisition);
	gtk_widget_size_request(GTK_WIDGET(path_bar->right_button), &child_requisition);
	/* add space for scroll buttons if more then 1 button */
	if(path_bar->path_button && path_bar->path_button->next)
	{
		gtk_widget_get_child_requisition(GTK_WIDGET(path_bar->left_button), &child_requisition);
		requisition->width += child_requisition.width + spacing;
		requisition->height = MAX(child_requisition.height, requisition->height);
		gtk_widget_get_child_requisition(GTK_WIDGET(path_bar->right_button), &child_requisition);
		requisition->width += child_requisition.width;
		requisition->height = MAX(child_requisition.height, requisition->height);
	}

	requisition->width += GTK_CONTAINER(path_bar)->border_width * 2;
	requisition->height += GTK_CONTAINER(path_bar)->border_width * 2;

	widget->requisition = *requisition;
}

static void
xa_path_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	XAPathBar *path_bar = XA_PATH_BAR(widget);
	gint spacing = 0;
	gint width = 0;
	gint left_width = 0;
	gint right_width = 0;
	gint border_width = 0;
	GSList *iter  = NULL;
	GSList *first_display = NULL;
	GtkRequisition child_requisition;
	GtkAllocation child_allocation;

	widget->allocation = *allocation;

	if(!path_bar->path_button)
		return;
	
	gtk_widget_style_get(widget, "spacing", &spacing, NULL);

	border_width = GTK_CONTAINER(path_bar)->border_width;
	width = border_width * 2;

	iter = path_bar->path_button;

	/* are the scroll arrows needed? */
	while(iter)
	{
		gtk_widget_get_child_requisition(GTK_WIDGET(iter->data), &child_requisition);
		if(width)
			width += spacing;
		width += child_requisition.width;
		iter = iter->next;
	}

	first_display = g_slist_prepend(first_display, path_bar->path_button);
	
	/* scroll arrows are needed */
	if(width > allocation->width)
	{
		gtk_widget_get_child_requisition(GTK_WIDGET(path_bar->left_button), &child_requisition);
		left_width = child_requisition.width;
		gtk_widget_get_child_requisition(GTK_WIDGET(path_bar->right_button), &child_requisition);
		right_width = child_requisition.width;

		width = left_width + spacing + right_width;

		if(path_bar->first_button)
		{
			iter = path_bar->path_button;
			while(iter != path_bar->first_button)
			{
				iter = iter->next;
				first_display = g_slist_prepend(first_display, iter);
			}
		}

		iter = (GSList*)first_display->data;

		/* are there parent maps visible? */
		while(iter)
		{
			gtk_widget_get_child_requisition(GTK_WIDGET(iter->data), &child_requisition);
			width += child_requisition.width + spacing;
			iter = iter->next;
		}

		iter = first_display->next;

		/* which parent map is visible? */
		while(iter)
		{
			gtk_widget_get_child_requisition(GTK_WIDGET(((GSList*)iter->data)->data), &child_requisition);
			width += child_requisition.width + spacing;
			if(width > allocation->width)
				break;

			first_display = g_slist_delete_link(first_display, first_display);
			iter = first_display->next;
		}
		/* first_display is the first entry from the list that fits */
	}

	child_allocation.y = allocation->y + border_width;
	child_allocation.x = allocation->x + border_width;
	child_allocation.height = allocation->height - (border_width * 2);

	/* set visible and sensitive scroll buttons */
	if(left_width)
	{
		/* set visible */
		gtk_widget_set_child_visible(GTK_WIDGET(path_bar->left_button), TRUE);
		child_allocation.width = left_width;
		gtk_widget_size_allocate(GTK_WIDGET(path_bar->left_button), &child_allocation);

		child_allocation.x += left_width + spacing;
		gtk_widget_set_sensitive(GTK_WIDGET(path_bar->left_button), (first_display->data == (gpointer)path_bar->path_button)?FALSE:TRUE);
	}
	else
	{
		/* set invisible */
		gtk_widget_set_child_visible(GTK_WIDGET(path_bar->left_button), FALSE);
	}

	/* set visible for buttons */
	iter = (GSList*)first_display->data;
	while(iter)
	{
		gtk_widget_get_child_requisition(GTK_WIDGET(iter->data), &child_requisition);
		width = child_allocation.x + child_requisition.width;
		if(right_width)
			width += spacing + right_width + border_width;
		/* check to see if there is enough space */
		if(width > allocation->width)
			break;
		/* set visible */
		gtk_widget_set_child_visible(GTK_WIDGET(iter->data), TRUE);
		child_allocation.width = child_requisition.width;
		gtk_widget_size_allocate(GTK_WIDGET(iter->data), &child_allocation);

		child_allocation.x += child_requisition.width + spacing;
		iter = iter->next;
	}

	/* set visible and sensitive scroll buttons */
	if(right_width)
	{
		/* set visible */
		gtk_widget_set_child_visible(GTK_WIDGET(path_bar->right_button), TRUE);
		child_allocation.x = allocation->width - right_width - border_width;
		child_allocation.width = right_width;
		gtk_widget_size_allocate(GTK_WIDGET(path_bar->right_button), &child_allocation);

		gtk_widget_set_sensitive(GTK_WIDGET(path_bar->right_button), iter?TRUE:FALSE);
	}
	else
	{
		/* set invisible */
		gtk_widget_set_child_visible(GTK_WIDGET(path_bar->right_button), FALSE);
	}

	/* hide all buttons that don't fit */
	while(iter)
	{
		gtk_widget_set_child_visible(GTK_WIDGET(iter->data), FALSE);
		iter = iter->next;
	}

	iter = first_display->next;

	while(iter)
	{
		gtk_widget_set_child_visible(GTK_WIDGET(((GSList*)iter->data)->data), FALSE);
		iter = iter->next;
	}

	g_slist_free(first_display);
}

static void
cb_xa_path_bar_pwd_changed(XAArchiveStore *store, XAPathBar *path_bar)
{
	GSList *path = xa_archive_store_get_pwd_list(store);
	GSList *iter = path;
	GSList *buttons = path_bar->path_button;
	GSList *lastbutton = NULL;
	GtkRadioButton *button = NULL;
	const gchar *label = NULL;
	gint cmp = 0;

	while(iter && buttons)
	{
		button = GTK_RADIO_BUTTON(buttons->data);
		label = gtk_button_get_label(GTK_BUTTON(button));
		cmp = strcmp(label, (gchar*)iter->data);
		if(cmp != 0)
		{
			while(buttons)
			{
				gtk_container_remove(GTK_CONTAINER(path_bar), GTK_WIDGET(buttons->data));
				gtk_widget_unref(GTK_WIDGET(buttons->data));
				buttons = buttons->next;
			}
			if(lastbutton)
			{
				g_slist_free(lastbutton->next);
				lastbutton->next = NULL;
			}
			else
			{
				g_slist_free(path_bar->path_button);
				path_bar->path_button = NULL;
			}
			break;
		}

		lastbutton = buttons;
		buttons = buttons->next;

		g_free(iter->data);
		iter = iter->next;
	}

	while(iter)
	{
		button = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(path_bar->path_button, (const gchar*)iter->data));
		gtk_widget_ref(GTK_WIDGET(button));
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);
		path_bar->path_button = g_slist_append(path_bar->path_button, button);

		g_signal_connect(G_OBJECT(button), "clicked", (GCallback)cb_xa_path_bar_path_button_clicked, path_bar);

		gtk_container_add(GTK_CONTAINER(path_bar), GTK_WIDGET(button));
		gtk_widget_show(GTK_WIDGET(button));

		g_free(iter->data);
		iter = iter->next;
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

	g_slist_free(path);

	path_bar->first_button = g_slist_last(path_bar->path_button);
}

static void
cb_xa_path_bar_path_button_clicked(GtkRadioButton *button, XAPathBar *path_bar)
{
	gchar *path = g_strdup("/");
	gchar *folder = NULL;
	GSList *iter = path_bar->path_button;

	while(iter->data != (gpointer)button)
	{
		iter = iter->next;
		folder = path;
		path = g_strconcat(path, gtk_button_get_label(GTK_BUTTON(iter->data)), "/", NULL);
		g_free(folder);
	}

	xa_archive_store_set_pwd_silent(XA_NAVIGATION_BAR(path_bar)->store, path);
	g_free(path);
}

static void
cb_xa_path_bar_scroll_left(GtkWidget *widget, gpointer user_data)
{
	XAPathBar *path_bar = XA_PATH_BAR(user_data);

	GSList *iter = path_bar->path_button;
	GSList *prev = NULL;

	while(iter)
	{
		if(gtk_widget_get_child_visible(GTK_WIDGET(iter->data)))
		{
			path_bar->first_button = prev;
			break;
		}

		prev = iter;
		iter = iter->next;
	}
}

static void
cb_xa_path_bar_scroll_right(GtkWidget *widget, gpointer user_data)
{
	XAPathBar *path_bar = XA_PATH_BAR(user_data);

	if(!path_bar->first_button)
		path_bar->first_button = path_bar->path_button;

	if(path_bar->first_button->next)
		path_bar->first_button = path_bar->first_button->next;
}

