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
#include <gettext.h>
#include <gtk/gtk.h>
#include "new_dialog.h"

static void
xa_new_dialog_class_init(XANewDialogClass *archive_class);

static void
xa_new_dialog_init(XANewDialog *archive);

static void
xa_new_dialog_finalize(GObject *object);

GType
xa_new_dialog_get_type ()
{
	static GType xa_new_dialog_type = 0;

 	if (!xa_new_dialog_type)
	{
 		static const GTypeInfo xa_new_dialog_info = 
		{
			sizeof (XANewDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_new_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XANewDialog),
			0,
			(GInstanceInitFunc) xa_new_dialog_init,
			NULL
		};

		xa_new_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "XANewDialog", &xa_new_dialog_info, 0);
	}
	return xa_new_dialog_type;
}

static void
xa_new_dialog_class_init(XANewDialogClass *archive_class)
{

}

static void
xa_new_dialog_init(XANewDialog *archive)
{
	
}

GtkWidget *
xa_new_dialog_new()
{
	GtkWidget *dialog;
	dialog = g_object_new(xa_new_dialog_get_type(), NULL);
	return dialog;
}
