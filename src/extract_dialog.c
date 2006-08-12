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
#include "extract_dialog.h"

enum
{
	XA_EXTRACT_ARCHIVE_DIALOG_ARCHIVE = 1
};

static void
xa_extract_archive_dialog_class_init(XAExtractArchiveDialogClass *archive_class);

static void
xa_extract_archive_dialog_init(XAExtractArchiveDialog *archive);

void
xa_extract_archive_dialog_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

void
xa_extract_archive_dialog_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

GType
xa_extract_archive_dialog_get_type ()
{
	static GType xa_extract_archive_dialog_type = 0;

 	if (!xa_extract_archive_dialog_type)
	{
 		static const GTypeInfo xa_extract_archive_dialog_info = 
		{
			sizeof (XAExtractArchiveDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_extract_archive_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAExtractArchiveDialog),
			0,
			(GInstanceInitFunc) xa_extract_archive_dialog_init,
			NULL
		};

		xa_extract_archive_dialog_type = g_type_register_static (GTK_TYPE_FILE_CHOOSER_DIALOG, "XAExtractArchiveDialog", &xa_extract_archive_dialog_info, 0);
	}
	return xa_extract_archive_dialog_type;
}

static void
xa_extract_archive_dialog_class_init(XAExtractArchiveDialogClass *dialog_class)
{
}

static void
xa_extract_archive_dialog_init(XAExtractArchiveDialog *dialog)
{
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			_("Extract"), GTK_RESPONSE_OK,
			NULL);
}

GtkWidget *
xa_extract_archive_dialog_new(LXAArchive *archive)
{
	GtkWidget *dialog;

	dialog = g_object_new(xa_extract_archive_dialog_get_type(), "title", _("Extract archive"), "action", GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, "do-overwrite-confirmation", TRUE, NULL);

	return dialog;
}
