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
	gtk_box_pack_start (GTK_BOX (hbox),gtk_label_new (_("Archive type:")),FALSE, FALSE, 0);

	dialog->archive_types_combo = gtk_combo_box_new_text();
	gtk_box_pack_start (GTK_BOX (hbox),dialog->archive_types_combo,FALSE, FALSE, 0);

	gtk_widget_show_all(hbox);

	dialog->supported_mime_types = lsq_get_supported_mime_types(LSQ_OPERATION_SUPPORT_ADD);
	GSList *_supported_mime_types = dialog->supported_mime_types;

	dialog->file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(dialog->file_filter, _("Archives"));
	while(_supported_mime_types)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->archive_types_combo),
		        lsq_archive_mime_get_comment((LSQArchiveMime *)(_supported_mime_types->data)));

		gtk_file_filter_add_mime_type(dialog->file_filter,
		        lsq_archive_mime_get_name((LSQArchiveMime *)(_supported_mime_types->data)));
		if(!strcmp(lsq_archive_mime_get_name((LSQArchiveMime *)_supported_mime_types->data), "application/x-compressed-tar"))
			gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->archive_types_combo), g_slist_index(dialog->supported_mime_types, _supported_mime_types->data));
		_supported_mime_types = g_slist_next(_supported_mime_types);
	}

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
			"do-overwrite-confirmation", TRUE,
			"action", GTK_FILE_CHOOSER_ACTION_SAVE,
			NULL);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), SQ_NEW_ARCHIVE_DIALOG(dialog)->file_filter);

	return dialog;
}

gchar *
sq_new_archive_dialog_get_filename(SQNewArchiveDialog *dialog)
{
	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	gchar *base = g_path_get_basename(filename);
	if(lsq_is_supported(base))
	{
		g_free(base);
		return filename;
	}
	else
	{
		g_free(base);
		gint i;
		GSList *_supported_mime_types = dialog->supported_mime_types;
		for(i = 0; i < gtk_combo_box_get_active(GTK_COMBO_BOX(dialog->archive_types_combo)); i++)
		{
			_supported_mime_types = g_slist_next(_supported_mime_types);
		}
		const gchar *mime_type = lsq_archive_mime_get_name(((LSQArchiveMime *)_supported_mime_types->data));
		gchar *suffix = NULL;
		if(!strcmp(mime_type, "application/x-tar")) suffix = ".tar";
		if(!strcmp(mime_type, "application/x-compressed-tar")) suffix = ".tar.gz";
		if(!strcmp(mime_type, "application/x-bzip-compressed-tar")) suffix = ".tar.bz2";
		if(!strcmp(mime_type, "application/x-tarz")) suffix = ".tar.Z";
		if(!strcmp(mime_type, "application/x-tzo")) suffix = ".tzo";

		if(!strcmp(mime_type, "application/zip")) suffix = ".zip";

		if(!strcmp(mime_type, "application/x-rar"))  suffix = ".rar";


		if(!strcmp(mime_type, "application/x-gzip"))  suffix = ".gz";
		if(!strcmp(mime_type, "application/x-bzip"))  suffix = ".bz2";
		if(!strcmp(mime_type, "application/x-lzop"))  suffix = ".lzo";
		if(!strcmp(mime_type, "application/x-compress"))  suffix = ".Z";
		base = filename;
		filename = g_strconcat(base, suffix, NULL);
		g_free(base);
		return filename;
	}
}
