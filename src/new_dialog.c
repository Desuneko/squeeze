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
#include <string.h>
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
	GtkWidget *hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start (GTK_BOX (hbox),gtk_label_new (("Archive type:")),FALSE, FALSE, 0);

	dialog->archive_types_combo = gtk_combo_box_new_text();
	gtk_box_pack_start (GTK_BOX (hbox),dialog->archive_types_combo,FALSE, FALSE, 0);
	dialog->append_extention_check = gtk_check_button_new_with_label(("Append extension to filename"));
	gtk_box_pack_start (GTK_BOX (hbox),dialog->append_extention_check,FALSE, FALSE, 0);
	gtk_widget_show_all(hbox);


	GSList *supported_mime_types = lsq_get_supported_mime_types();
	GSList *_supported_mime_types = supported_mime_types;

	GtkFileFilter *file_filter = gtk_file_filter_new();
	while(_supported_mime_types)
	{
		if(!strcmp(_supported_mime_types->data, "application/x-tar"))
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->archive_types_combo), ".tar");
		}
		if(!strcmp(_supported_mime_types->data, "application/x-tarz"))
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->archive_types_combo), ".tar.Z");
		}
		if(!strcmp(_supported_mime_types->data, "application/x-compressed-tar"))
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->archive_types_combo), ".tgz");
			gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->archive_types_combo), ".tar.gz");
		}
		if(!strcmp(_supported_mime_types->data, "application/x-bzip-compressed-tar"))
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->archive_types_combo), ".tbz");
			gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->archive_types_combo), ".tbz2");
		}
		if(!strcmp(_supported_mime_types->data, "application/x-tzo"))
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->archive_types_combo), ".tzo");
		}
		if(!strcmp(_supported_mime_types->data, "application/x-zip"))
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->archive_types_combo), ".zip");
		}
		gtk_file_filter_add_mime_type(file_filter, _supported_mime_types->data);
		_supported_mime_types = g_slist_next(_supported_mime_types);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->archive_types_combo), 0);

	g_slist_free(supported_mime_types);

/* WHY DOESN'T THIS WORK?!*/
/*	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(GTK_FILE_CHOOSER_DIALOG(dialog)), file_filter); */

	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 0);
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
