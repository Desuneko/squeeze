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
#include "main_window_status_bar.h"
#include "main_window.h"

static void
xa_main_window_status_bar_class_init(XAMainWindowStatusBarClass *);

static void
xa_main_window_status_bar_init(XAMainWindowStatusBar *);

GType
xa_main_window_status_bar_get_type ()
{
	static GType xa_main_window_status_bar_type = 0;

 	if (!xa_main_window_status_bar_type)
	{
 		static const GTypeInfo xa_main_window_status_bar_info = 
		{
			sizeof (XAMainWindowStatusBarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_main_window_status_bar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAMainWindowStatusBar),
			0,
			(GInstanceInitFunc) xa_main_window_status_bar_init,
			NULL
		};

		xa_main_window_status_bar_type = g_type_register_static (GTK_TYPE_STATUSBAR, "XAMainWindowStatusBar", &xa_main_window_status_bar_info, 0);
	}
	return xa_main_window_status_bar_type;
}

static void
xa_main_window_status_bar_class_init(XAMainWindowStatusBarClass *menubar_class)
{
}

static void
xa_main_window_status_bar_init(XAMainWindowStatusBar *menubar)
{
}

GtkWidget *
xa_main_window_status_bar_new()
{
	GtkWidget *menubar;

	menubar = g_object_new(xa_main_window_status_bar_get_type(), NULL);

	return menubar;
}
