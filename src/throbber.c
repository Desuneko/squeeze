/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
 *
 * Modified by Stephan Arts <stephan@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.	See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA	02111-1307	USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "throbber.h"
#include "throbber-fallback.h"



/* Property identifiers */
enum
{
	PROP_0,
	PROP_ANIMATED,
};



static void	sq_throbber_class_init(SQThrobberClass *klass);
static void	sq_throbber_init(SQThrobber *throbber);
static void	sq_throbber_dispose(GObject *object);
static void	sq_throbber_get_property(GObject *object,
										  guint prop_id,
										  GValue *value,
										  GParamSpec *pspec);
static void	sq_throbber_set_property(GObject *object,
										  guint prop_id,
										  const GValue *value,
										  GParamSpec *pspec);
static void	sq_throbber_realize(GtkWidget *widget);
static void	sq_throbber_unrealize(GtkWidget *widget);
static void	sq_throbber_size_request(GtkWidget *widget,
										  GtkRequisition *requisition);
static gboolean sq_throbber_expose_event(GtkWidget *widget,
										 GdkEventExpose *event);
static gboolean sq_throbber_timer(gpointer user_data);
static void	sq_throbber_timer_destroy(gpointer user_data);



struct _SQThrobberClass
{
	GtkWidgetClass __parent__;
};

struct _SQThrobber
{
	GtkWidget __parent__;

	GdkPixbuf *icon;

	gboolean   animated;
	gint	   index;
	gint	   timer_id;
};



static GObjectClass *sq_throbber_parent_class;



GType
sq_throbber_get_type (void)
{
	static GType type = G_TYPE_INVALID;

	if (G_UNLIKELY (type == G_TYPE_INVALID))
	{
		static const GTypeInfo info =
		{
			sizeof (SQThrobberClass),
			NULL,
			NULL,
			(GClassInitFunc) sq_throbber_class_init,
			NULL,
			NULL,
			sizeof (SQThrobber),
			0,
			(GInstanceInitFunc) sq_throbber_init,
			NULL,
		};

		type = g_type_register_static (GTK_TYPE_WIDGET, "SQThrobber", &info, 0);
	}

	return type;
}



static void
sq_throbber_class_init (SQThrobberClass *klass)
{
	GtkWidgetClass *gtkwidget_class;
	GObjectClass   *gobject_class;
	GdkPixbuf	  *icon;

	/* determine the parent type class */
	sq_throbber_parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->dispose = sq_throbber_dispose;
	gobject_class->get_property = sq_throbber_get_property;
	gobject_class->set_property = sq_throbber_set_property;

	gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->realize = sq_throbber_realize;
	gtkwidget_class->unrealize = sq_throbber_unrealize;
	gtkwidget_class->size_request = sq_throbber_size_request;
	gtkwidget_class->expose_event = sq_throbber_expose_event;

	/**
	 * SQThrobber:animated:
	 *
	 * Whether the throbber should display an animation.
	 **/
	g_object_class_install_property (gobject_class,
									 PROP_ANIMATED,
									 g_param_spec_boolean ("animated",
									 "animated", "animated", \
									 FALSE, G_PARAM_READWRITE));

	/* register the "process-working" fallback icon */
	icon = gdk_pixbuf_new_from_inline (-1, sq_throbber_fallback, FALSE, NULL);
	gtk_icon_theme_add_builtin_icon ("process-working", 16, icon);
	g_object_unref (G_OBJECT (icon));
}



static void
sq_throbber_init (SQThrobber *throbber)
{
	GTK_WIDGET_SET_FLAGS (throbber, GTK_NO_WINDOW);
	throbber->timer_id = -1;
}



static void
sq_throbber_dispose (GObject *object)
{
	SQThrobber *throbber = SQ_THROBBER (object);

	/* stop any running animation */
	if (G_UNLIKELY (throbber->timer_id >= 0))
		g_source_remove (throbber->timer_id);

	(*G_OBJECT_CLASS (sq_throbber_parent_class)->dispose) (object);
}



static void
sq_throbber_get_property (GObject *object,
						  guint prop_id,
						  GValue *value,
						  GParamSpec *pspec)
{
	SQThrobber *throbber = SQ_THROBBER (object);

	switch (prop_id)
	{
		case PROP_ANIMATED:
			g_value_set_boolean (value, sq_throbber_get_animated (throbber));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}



static void
sq_throbber_set_property (GObject *object,
						  guint prop_id,
						  const GValue *value,
						  GParamSpec *pspec)
{
	SQThrobber *throbber = SQ_THROBBER (object);

	switch (prop_id)
		{
		case PROP_ANIMATED:
			sq_throbber_set_animated (throbber, g_value_get_boolean (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
		}
}



static void
sq_throbber_realize (GtkWidget *widget)
{
	SQThrobber *throbber = SQ_THROBBER (widget);
	GtkIconTheme	 *icon_theme;

	/* let Gtk+ realize the widget */
	(*GTK_WIDGET_CLASS (sq_throbber_parent_class)->realize) (widget);

	/* determine the icon theme for our screen */
	icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));

	/* try to lookup the "process-working" icon */
	throbber->icon = gtk_icon_theme_load_icon (icon_theme, "process-working", 16, GTK_ICON_LOOKUP_USE_BUILTIN | GTK_ICON_LOOKUP_NO_SVG, NULL);
}



static void
sq_throbber_unrealize (GtkWidget *widget)
{
	SQThrobber *throbber = SQ_THROBBER (widget);

	/* release the icon if any */
	if (G_LIKELY (throbber->icon != NULL))
	{
		g_object_unref (G_OBJECT (throbber->icon));
		throbber->icon = NULL;
	}

	/* let Gtk+ unrealize the widget */
	(*GTK_WIDGET_CLASS (sq_throbber_parent_class)->unrealize) (widget);
}



static void
sq_throbber_size_request (GtkWidget *widget,
						  GtkRequisition *requisition)
{
	requisition->width = 16;
	requisition->height = 16;
}



static gboolean
sq_throbber_expose_event (GtkWidget *widget,
						  GdkEventExpose *event)
{
	SQThrobber *throbber = SQ_THROBBER (widget);
	gint icon_index;
	gint icon_cols;
	gint icon_rows;
	gint icon_x;
	gint icon_y;

	/* verify that we have a valid icon */
	if (G_LIKELY (throbber->icon != NULL))
	{
		/* determine the icon columns and icon rows */
		icon_cols = gdk_pixbuf_get_width (throbber->icon) / 16;
		icon_rows = gdk_pixbuf_get_height (throbber->icon) / 16;

		/* verify that the icon is usable */
		if (G_LIKELY (icon_cols > 0 && icon_rows > 0))
		{
			/* determine the icon index */
			icon_index = throbber->index % (icon_cols * icon_rows);

			/* make sure, we don't display the "empty state" while animated */
			if (G_LIKELY (throbber->timer_id >= 0))
				icon_index = MAX (icon_index, 1);

			/* determine the icon x/y offset for the icon index */
			icon_x = (icon_index % icon_cols) * 16;
			icon_y = (icon_index / icon_cols) * 16;

			/* render the given part of the icon */
			gdk_draw_pixbuf (event->window, NULL, throbber->icon, icon_x, icon_y,
									 widget->allocation.x, widget->allocation.y,
									 16, 16, GDK_RGB_DITHER_NONE, 0, 0);
		}
	}

	return TRUE;
}



static gboolean
sq_throbber_timer (gpointer user_data)
{
	SQThrobber *throbber = SQ_THROBBER (user_data);

	GDK_THREADS_ENTER ();
	throbber->index += 1;
	gtk_widget_queue_draw (GTK_WIDGET (throbber));
	GDK_THREADS_LEAVE ();

	return throbber->animated;
}



static void
sq_throbber_timer_destroy (gpointer user_data)
{
	SQ_THROBBER (user_data)->index = 0;
	SQ_THROBBER (user_data)->timer_id = -1;
}



/**
 * sq_throbber_new:
 *
 * Allocates a new #SQThrobber instance.
 *
 * Return value: the newly allocated #SQThrobber.
 **/
GtkWidget*
sq_throbber_new (void)
{
	return g_object_new (SQ_TYPE_THROBBER, NULL);
}



/**
 * sq_throbber_get_animated:
 * @throbber : a #SQThrobber.
 *
 * Returns whether @throbber is currently animated.
 *
 * Return value: %TRUE if @throbber is animated.
 **/
gboolean
sq_throbber_get_animated (const SQThrobber *throbber)
{
	g_return_val_if_fail (SQ_IS_THROBBER (throbber), FALSE);
	return throbber->animated;
}



/**
 * sq_throbber_set_animated:
 * @throbber : a #SQThrobber.
 * @animated : whether to animate @throbber.
 *
 * If @animated is %TRUE, @throbber will display an animation.
 **/
void
sq_throbber_set_animated (SQThrobber *throbber,
						  gboolean animated)
{
	g_return_if_fail (SQ_IS_THROBBER (throbber));

	/* check if we're already in the requested state */
	if (G_UNLIKELY (throbber->animated == animated))
		return;

	/* pick up the new state */
	throbber->animated = animated;

	/* start the timer if animated and not already running */
	if (animated && (throbber->timer_id < 0))
	{
		/* start the animation */
		throbber->timer_id = g_timeout_add_full (G_PRIORITY_LOW, 25, sq_throbber_timer,
												 throbber, sq_throbber_timer_destroy);
	}

	/* schedule a redraw with the new animation state */
	gtk_widget_queue_draw (GTK_WIDGET (throbber));

	/* notify listeners */
	g_object_notify (G_OBJECT (throbber), "animated");
}
