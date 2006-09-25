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
#include "preferences_dialog.h"

static void
xa_preferences_dialog_class_init(XAPreferencesDialogClass *archive_class);

static void
xa_preferences_dialog_init(XAPreferencesDialog *archive);

GType
xa_preferences_dialog_get_type ()
{
	static GType xa_preferences_dialog_type = 0;

 	if (!xa_preferences_dialog_type)
	{
 		static const GTypeInfo xa_preferences_dialog_info = 
		{
			sizeof (XAPreferencesDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_preferences_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAPreferencesDialog),
			0,
			(GInstanceInitFunc) xa_preferences_dialog_init,
			NULL
		};

		xa_preferences_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "XAPreferencesDialog", &xa_preferences_dialog_info, 0);
	}
	return xa_preferences_dialog_type;
}

static void
xa_preferences_dialog_class_init(XAPreferencesDialogClass *dialog_class)
{
}

static void
xa_preferences_dialog_init(XAPreferencesDialog *dialog)
{
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_OK,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

GtkWidget *
xa_preferences_dialog_new()
{
	GtkWidget *dialog;

	dialog = g_object_new(xa_preferences_dialog_get_type(),
			"title", _("Preferences"),
			NULL);

	return dialog;
}
