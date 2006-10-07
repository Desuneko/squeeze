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
#include "path_bar.h"


static void
xa_path_bar_class_init(XAPathBarClass *archive_class);

static void
xa_path_bar_init(XAPathBar *archive);

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

		xa_path_bar_type = g_type_register_static (GTK_TYPE_TOOLBAR, "XAPathBar", &xa_path_bar_info, 0);
	}
	return xa_path_bar_type;
}

static void
xa_path_bar_class_init(XAPathBarClass *dialog_class)
{
}

static void
xa_path_bar_init(XAPathBar *dialog)
{
}

GtkWidget *
xa_path_bar_new(XAArchiveStore *store)
{
	XAPathBar *bar;

	bar = g_object_new(XA_TYPE_PATH_BAR, NULL);

	bar->store = store;

	return GTK_WIDGET(bar);
}
