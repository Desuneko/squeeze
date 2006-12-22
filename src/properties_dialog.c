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
#include <glib.h>
#include <gtk/gtk.h>
#include <libsqueeze/libsqueeze.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef HAVE_THUNAR_VFS
#define EXO_API_SUBJECT_TO_CHANGE
#include <thunar-vfs/thunar-vfs.h>
#else
#include <gettext.h>
#endif

#include "properties_dialog.h"

static void
sq_properties_dialog_class_init(SQPropertiesDialogClass *archive_class);

static void
sq_properties_dialog_init(SQPropertiesDialog *archive);

GType
sq_properties_dialog_get_type ()
{
	static GType sq_properties_dialog_type = 0;

 	if (!sq_properties_dialog_type)
	{
 		static const GTypeInfo sq_properties_dialog_info = 
		{
			sizeof (SQPropertiesDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_properties_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQPropertiesDialog),
			0,
			(GInstanceInitFunc) sq_properties_dialog_init,
			NULL
		};

		sq_properties_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "SQPropertiesDialog", &sq_properties_dialog_info, 0);
	}
	return sq_properties_dialog_type;
}

static void
sq_properties_dialog_class_init(SQPropertiesDialogClass *dialog_class)
{
}

static void
sq_properties_dialog_init(SQPropertiesDialog *dialog)
{
}

GtkWidget *
sq_properties_dialog_new(LSQArchive *archive)
{
	GtkWidget *dialog;

	dialog = g_object_new(sq_properties_dialog_get_type(),
			"title", _("Properties"),
			NULL);

	return dialog;
}
