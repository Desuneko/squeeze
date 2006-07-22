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
xa_new_archive_dialog_class_init(XANewArchiveDialogClass *archive_class);

static void
xa_new_archive_dialog_init(XANewArchiveDialog *archive);

static void
xa_new_archive_dialog_finalize(GObject *object);

GType
xa_new_archive_dialog_get_type ()
{
	static GType xa_new_archive_dialog_type = 0;

 	if (!xa_new_archive_dialog_type)
	{
 		static const GTypeInfo xa_new_archive_dialog_info = 
		{
			sizeof (XANewArchiveDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_new_archive_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XANewArchiveDialog),
			0,
			(GInstanceInitFunc) xa_new_archive_dialog_init,
			NULL
		};

		xa_new_archive_dialog_type = g_type_register_static (GTK_TYPE_FILE_CHOOSER_DIALOG, "XANewArchiveDialog", &xa_new_archive_dialog_info, 0);
	}
	return xa_new_archive_dialog_type;
}

static void
xa_new_archive_dialog_class_init(XANewArchiveDialogClass *dialog_class)
{
}

static void
xa_new_archive_dialog_init(XANewArchiveDialog *dialog)
{
	GtkWidget *hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start (GTK_BOX (hbox),gtk_label_new (_("Archive type:")),FALSE, FALSE, 0);
	dialog->filetype_selector = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->filetype_selector), _("Automatic"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->filetype_selector), 0);
	gtk_box_pack_start (GTK_BOX (hbox), dialog->filetype_selector,TRUE , TRUE, 0);
	
	gtk_widget_show_all(hbox);
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 0);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_NEW, GTK_RESPONSE_OK,
			NULL);
}

GtkWidget *
xa_new_archive_dialog_new()
{
	GtkWidget *dialog;
	dialog = g_object_new(xa_new_archive_dialog_get_type(), "title", _("Create new archive"), "action", GTK_FILE_CHOOSER_ACTION_SAVE, NULL);
	return dialog;
}
