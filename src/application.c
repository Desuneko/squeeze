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
#include "settings.h"
#include "archive_store.h"
#include "navigation_bar.h"
#include "main_window.h"
#include "application.h"

static void
xa_application_class_init(XAApplicationClass *archive_class);

static void
xa_application_init(XAApplication *archive);
static void
xa_application_finalize(GObject *object);

/* properties */
enum {
	XA_APPLICATION_NAV_HISTORY = 1
};

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

		xa_application_type = g_type_register_static (GTK_TYPE_CONTAINER, "XAApplication", &xa_application_info, 0);
	}
	return xa_application_type;
}

static void
xa_application_class_init(XAApplicationClass *application_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (application_class);
	object_class->finalize     = xa_application_finalize;
}

static void
xa_application_init(XAApplication *application)
{
}

static void
xa_application_finalize(GObject *object)
{
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
	return xa_main_window_new(app->icon_theme);
}
