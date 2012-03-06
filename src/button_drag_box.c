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
#include <gio/gio.h>
#include <libsqueeze/libsqueeze.h>
#include "archive_store.h"
#include "button_drag_box.h"

#define SQ_INDICATOR_SIZE 9
#define SQ_BUTTON_USER_DATA "sq-user-data"
#define SQ_DRAG_TARGET_ID "_SQ_BUTTON_DRAG_BOX"

static GdkPixbuf *
sq_create_icon_from_widget(GtkWidget *widget);

static void
sq_create_indicator(SQButtonDragBox *box, gint x, gint y, gint width, gint height);
static void
sq_delete_indicator(SQButtonDragBox *box);

static gboolean
cb_signal_blocker(GtkWidget *widget, gpointer user_data);

static void
cb_sq_button_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data, guint info, guint time, gpointer user_data);

static void
cb_sq_button_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data);

static void
cb_sq_button_drag_end(GtkWidget *widget, GdkDragContext *context, gpointer user_data);

static void
cb_sq_visible_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint infom, guint time, gpointer user_data);

static gboolean
cb_sq_visible_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data);

static void
cb_sq_visible_drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer user_data);

static void
cb_sq_hidden_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint infom, guint time, gpointer user_data);

G_DEFINE_TYPE (SQButtonDragBox, sq_button_drag_box, GTK_TYPE_VBOX)

static void
sq_button_drag_box_class_init(SQButtonDragBoxClass *button_drag_box_class)
{
}

static void
sq_button_drag_box_init(SQButtonDragBox *box)
{
	GtkWidget *frame;

	box->locked_buttons = 0;

	box->visible_box = gtk_hbox_new(FALSE, 0);
	box->hidden_box = gtk_hbox_new(FALSE, 0);

	box->entry.target = SQ_DRAG_TARGET_ID;
	box->entry.flags = GTK_TARGET_SAME_APP;
	box->entry.info = 2;

	frame = gtk_frame_new(_("Visible:"));

	gtk_drag_dest_set(frame, GTK_DEST_DEFAULT_ALL, &box->entry, 1, GDK_ACTION_MOVE);
	g_signal_connect(frame, "drag_data_received", G_CALLBACK(cb_sq_visible_data_received), box);
	g_signal_connect(frame, "drag_motion", G_CALLBACK(cb_sq_visible_drag_motion), box);
	g_signal_connect(frame, "drag_leave", G_CALLBACK(cb_sq_visible_drag_leave), box);

	gtk_container_set_border_width(GTK_CONTAINER(box->visible_box), 5);
	gtk_container_add(GTK_CONTAINER(frame), box->visible_box);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("Available:"));

	gtk_drag_dest_set(frame, GTK_DEST_DEFAULT_ALL, &box->entry, 1, GDK_ACTION_MOVE);
	g_signal_connect(frame, "drag_data_received", G_CALLBACK(cb_sq_hidden_data_received), box);

	gtk_container_set_border_width(GTK_CONTAINER(box->hidden_box), 5);
	gtk_container_add(GTK_CONTAINER(frame), box->hidden_box);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	gtk_box_set_homogeneous(GTK_BOX(box), TRUE);
}

GtkWidget *
sq_button_drag_box_new(void)
{
	GtkWidget *box;

	box = g_object_new(SQ_TYPE_BUTTON_DRAG_BOX, NULL);

	return box;
}

void
sq_button_drag_box_add_fixed_button(SQButtonDragBox *box, const gchar *label, gpointer user_data)
{
	GtkWidget *button = gtk_button_new_with_label(label);

	g_object_set_data(G_OBJECT(button), SQ_BUTTON_USER_DATA, user_data);

	gtk_box_pack_start(GTK_BOX(box->visible_box), button, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(button), "button_press_event", G_CALLBACK(cb_signal_blocker), NULL);
	g_signal_connect(G_OBJECT(button), "enter_notify_event", G_CALLBACK(cb_signal_blocker), NULL);
	g_signal_connect(G_OBJECT(button), "focus", G_CALLBACK(cb_signal_blocker), NULL);
}

void
sq_button_drag_box_add_button(SQButtonDragBox *box, const gchar *label, gboolean visible, gpointer user_data)
{
	GtkWidget *button = gtk_button_new_with_label(label);

	g_object_set_data(G_OBJECT(button), SQ_BUTTON_USER_DATA, user_data);

	gtk_box_pack_start(visible?GTK_BOX(box->visible_box):GTK_BOX(box->hidden_box), button, FALSE, FALSE, 0);

	gtk_drag_source_set(button, GDK_BUTTON1_MASK, &box->entry, 1, GDK_ACTION_MOVE);
	g_signal_connect(G_OBJECT(button), "drag_data_get", G_CALLBACK(cb_sq_button_data_get), NULL);
	g_signal_connect(G_OBJECT(button), "drag_begin", G_CALLBACK(cb_sq_button_drag_begin), NULL);
	g_signal_connect(G_OBJECT(button), "drag_end", G_CALLBACK(cb_sq_button_drag_end), NULL);
	g_signal_connect(G_OBJECT(button), "button_press_event", G_CALLBACK(cb_signal_blocker), NULL);
	g_signal_connect(G_OBJECT(button), "enter_notify_event", G_CALLBACK(cb_signal_blocker), NULL);
	g_signal_connect(G_OBJECT(button), "focus", G_CALLBACK(cb_signal_blocker), NULL);
}

void
sq_button_drag_box_lock_buttons(SQButtonDragBox *box, guint buttons)
{
	box->locked_buttons = buttons;
}

GSList *
sq_button_drag_box_get_visible(SQButtonDragBox *box)
{
	GList *iter, *children = iter = gtk_container_get_children(GTK_CONTAINER(box->visible_box));
	GSList *list = NULL;

	while(iter)
	{
		if(GTK_WIDGET_VISIBLE(iter->data))
		{
			list = g_slist_append(list, g_object_get_data(G_OBJECT(iter->data), SQ_BUTTON_USER_DATA));
		}

		iter = g_list_next(iter);
	}

	g_list_free(children);

	return list;
}

GSList *
sq_button_drag_box_get_hidden(SQButtonDragBox *box)
{
	GList *iter, *children = iter = gtk_container_get_children(GTK_CONTAINER(box->hidden_box));
	GSList *list = NULL;

	while(iter)
	{
		if(GTK_WIDGET_VISIBLE(iter->data))
		{
			list = g_slist_append(list, g_object_get_data(G_OBJECT(iter->data), SQ_BUTTON_USER_DATA));
		}

		iter = g_list_next(iter);
	}

	g_list_free(children);

	return list;
}

static GdkPixbuf *
sq_create_icon_from_widget(GtkWidget *widget)
{
	GdkWindow *drawable = GDK_DRAWABLE(gtk_widget_get_parent_window(widget));
	return gdk_pixbuf_get_from_drawable(NULL, drawable, NULL, widget->allocation.x, widget->allocation.y, 0, 0, widget->allocation.width, widget->allocation.height);
}

static void
sq_create_indicator(SQButtonDragBox *box, gint x, gint y, gint width, gint height)
{
	gint attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_COLORMAP | GDK_WA_VISUAL;
	GdkWindowAttr attributes = {
		NULL,
		0,
		x, y,
		width, height,
		GDK_INPUT_OUTPUT,
		gtk_widget_get_visual(box->visible_box),
		gtk_widget_get_colormap(box->visible_box),
		GDK_WINDOW_CHILD, 
		NULL, NULL, NULL, FALSE
	};

	sq_delete_indicator(box);

	box->indicator = gdk_window_new(gtk_widget_get_parent_window(box->visible_box), &attributes, attr_mask);
	gdk_window_set_user_data(box->indicator, box->visible_box);

	GdkPoint points[9];
	points[0].x = 0;
	points[0].y = 0;
	points[1].x = width;
	points[1].y = 0;
	points[2].x = width/2+1;
	points[2].y = width/2;
	points[3].x = width/2+1;
	points[3].y = height-1-width/2;
	points[4].x = width;
	points[4].y = height;
	points[5].x = 0;
	points[5].y = height-1;
	points[6].x = width/2;
	points[6].y = height-1-width/2;
	points[7].x = width/2;
	points[7].y = width/2;
	points[8].x = 0;
	points[8].y = 0;
	GdkRegion *shape = gdk_region_polygon(points, 9, GDK_WINDING_RULE);

	gdk_window_shape_combine_region(box->indicator, shape, 0, 0);

	gdk_window_show(box->indicator);
	gdk_window_raise(box->indicator);
}

static void
sq_delete_indicator(SQButtonDragBox *box)
{
	if(box->indicator)
	{
		gdk_window_destroy(box->indicator);
		box->indicator = NULL;
	}
}

static gboolean
cb_signal_blocker(GtkWidget *widget, gpointer user_data)
{
	return TRUE;
}

static void
cb_sq_button_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	gtk_widget_hide(widget);
	gtk_selection_data_set(data, gdk_atom_intern(SQ_DRAG_TARGET_ID, FALSE), 8, NULL, 0);
}

static void
cb_sq_button_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
	GdkPixbuf *pixbuf = sq_create_icon_from_widget(widget);

	gtk_drag_source_set_icon_pixbuf(widget, pixbuf);
	g_object_unref(G_OBJECT(pixbuf));
	gtk_widget_hide(widget);
}

static void
cb_sq_button_drag_end(GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
	gtk_widget_show(widget);
}

static void
cb_sq_visible_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint infom, guint time, gpointer user_data)
{
	SQButtonDragBox *box = SQ_BUTTON_DRAG_BOX(user_data);

	GtkWidget *source = gtk_drag_get_source_widget(context);
	GtkWidget *parent = gtk_widget_get_parent(source);

	gtk_widget_ref(source);
	gtk_container_remove(GTK_CONTAINER(parent), source);
	gtk_box_pack_start(GTK_BOX(box->visible_box), source, FALSE, FALSE, 0);
	gtk_widget_unref(source);

	guint button = 0;
	gint xoffset = box->visible_box->allocation.x;
	GtkWidget *item;

	GList *iter, *children = iter = gtk_container_get_children(GTK_CONTAINER(box->visible_box));

	gint i = 0;

	while(iter)
	{
		item = GTK_WIDGET(iter->data);

		if(GTK_WIDGET_VISIBLE(item))
		{
			button++;
			if((box->locked_buttons < button) && (x < (item->allocation.width/2 + item->allocation.x - xoffset)))
			{
				break;
			}
		}
		i++;
		iter = g_list_next(iter);
	}

	g_list_free(children);

	gtk_box_reorder_child(GTK_BOX(box->visible_box), source, i);
}

static gboolean
cb_sq_visible_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data)
{
	SQButtonDragBox *box = SQ_BUTTON_DRAG_BOX(user_data);

	guint button = 0;
	gint ix, iy;
	gint xoffset = box->visible_box->allocation.x;
	GtkWidget *item;

	GList *iter, *children = iter = gtk_container_get_children(GTK_CONTAINER(box->visible_box));

	ix = xoffset + gtk_container_get_border_width(GTK_CONTAINER(box->visible_box));

	while(iter)
	{
		item = GTK_WIDGET(iter->data);

		if(GTK_WIDGET_VISIBLE(item))
		{
			button++;
			if((box->locked_buttons < button) && (x < (item->allocation.width/2 + item->allocation.x - xoffset)))
			{
				ix = item->allocation.x;
				break;
			}
			ix = item->allocation.x + item->allocation.width;
		}
		iter = g_list_next(iter);
	}

	g_list_free(children);

	ix -= SQ_INDICATOR_SIZE/2 + 1;
	iy = box->visible_box->allocation.y - SQ_INDICATOR_SIZE/2 + gtk_container_get_border_width(GTK_CONTAINER(box->visible_box));

	if(!box->indicator)
	{
		sq_create_indicator(box, ix, iy, SQ_INDICATOR_SIZE, box->visible_box->allocation.height + SQ_INDICATOR_SIZE - gtk_container_get_border_width(GTK_CONTAINER(box->visible_box))*2);
	}
	else
	{
		gdk_window_move(box->indicator, ix, iy);
	}

	return FALSE;
}

static void
cb_sq_visible_drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer user_data)
{
	sq_delete_indicator(SQ_BUTTON_DRAG_BOX(user_data));
}

static void
cb_sq_hidden_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint infom, guint time, gpointer user_data)
{
	GtkWidget *source = gtk_drag_get_source_widget(context);
	GtkWidget *parent = gtk_widget_get_parent(source);

	/* if the item was dragged back to the location it already was */
	if(parent == SQ_BUTTON_DRAG_BOX(user_data)->hidden_box)
	{
		return;
	}

	gtk_widget_ref(source);
	gtk_container_remove(GTK_CONTAINER(parent), source);
	gtk_box_pack_start(GTK_BOX(SQ_BUTTON_DRAG_BOX(user_data)->hidden_box), source, FALSE, FALSE, 0);
	gtk_widget_unref(source);
}

