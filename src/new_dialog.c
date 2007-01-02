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
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>

#include "new_dialog.h"

static void
sq_new_archive_dialog_class_init(SQNewArchiveDialogClass *archive_class);

static void
sq_new_archive_dialog_init(SQNewArchiveDialog *archive);

GType
sq_new_archive_dialog_get_type ()
{
	static GType sq_new_archive_dialog_type = 0;

 	if (!sq_new_archive_dialog_type)
	{
 		static const GTypeInfo sq_new_archive_dialog_info = 
		{
			sizeof (SQNewArchiveDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_new_archive_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQNewArchiveDialog),
			0,
			(GInstanceInitFunc) sq_new_archive_dialog_init,
			NULL
		};

		sq_new_archive_dialog_type = g_type_register_static (GTK_TYPE_FILE_CHOOSER_DIALOG, "SQNewArchiveDialog", &sq_new_archive_dialog_info, 0);
	}
	return sq_new_archive_dialog_type;
}

static void
sq_new_archive_dialog_class_init(SQNewArchiveDialogClass *dialog_class)
{
}

static void
sq_new_archive_dialog_init(SQNewArchiveDialog *dialog)
{
/*
	GtkWidget *hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start (GTK_BOX (hbox),gtk_label_new (_("Archive type:")),FALSE, FALSE, 0);
	
	gtk_widget_show_all(hbox);
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 0);
*/
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_NEW, GTK_RESPONSE_OK,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

GtkWidget *
sq_new_archive_dialog_new()
{
	GtkWidget *dialog;

	dialog = g_object_new(sq_new_archive_dialog_get_type(),
			"title", _("Create new archive"),
			"action", GTK_FILE_CHOOSER_ACTION_SAVE,
			"do-overwrite-confirmation", TRUE,
			NULL);

	return dialog;
}
