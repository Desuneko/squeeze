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
#include <glib.h>
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>
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
	GtkTreeIter iter;
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GValue *str_value = g_new0(GValue, 1);
	GValue *at_value = g_new0(GValue, 1);
	GValue *ct_value = g_new0(GValue, 1);

	GtkWidget *hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start (GTK_BOX (hbox),gtk_label_new (_("Archive type:")),FALSE, FALSE, 0);
	dialog->filetype_model = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);
	dialog->filetype_selector = gtk_combo_box_new_with_model(GTK_TREE_MODEL(dialog->filetype_model));
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(dialog->filetype_selector), renderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(dialog->filetype_selector), renderer, "text", 0);

	str_value = g_value_init(str_value, G_TYPE_STRING);
	at_value = g_value_init(at_value, G_TYPE_UINT);
	ct_value = g_value_init(ct_value, G_TYPE_UINT);

	gtk_list_store_append(dialog->filetype_model, &iter);

	g_value_set_string(str_value, _("Automatic"));
	gtk_list_store_set_value(dialog->filetype_model, &iter, 0, str_value);
	g_value_set_uint(at_value, LXA_ARCHIVETYPE_UNKNOWN);
	g_value_set_uint(ct_value, LXA_COMPRESSIONTYPE_UNKNOWN);
	gtk_list_store_set_value(dialog->filetype_model, &iter, 1, at_value);
	gtk_list_store_set_value(dialog->filetype_model, &iter, 2, ct_value);

	if(lxa_archivetype_supported(LXA_ARCHIVETYPE_TAR))
	{
		gtk_list_store_append(dialog->filetype_model, &iter);
		g_value_set_string(str_value, _("Tar (uncompressed) '.tar'"));
		gtk_list_store_set_value(dialog->filetype_model, &iter, 0, str_value);

		g_value_set_uint(at_value, LXA_ARCHIVETYPE_TAR);
		g_value_set_uint(ct_value, LXA_COMPRESSIONTYPE_NONE);
		gtk_list_store_set_value(dialog->filetype_model, &iter, 1, at_value);
		gtk_list_store_set_value(dialog->filetype_model, &iter, 2, ct_value);
		if(lxa_compressiontype_supported(LXA_COMPRESSIONTYPE_GZIP))
		{
			gtk_list_store_append(dialog->filetype_model, &iter);
			g_value_set_string(str_value, _("Gzip compressed Tar '.tar.gz'"));
			gtk_list_store_set_value(dialog->filetype_model, &iter, 0, str_value);
			g_value_set_uint(at_value, LXA_ARCHIVETYPE_TAR);
			g_value_set_uint(ct_value, LXA_COMPRESSIONTYPE_GZIP);
			gtk_list_store_set_value(dialog->filetype_model, &iter, 1, at_value);
			gtk_list_store_set_value(dialog->filetype_model, &iter, 2, ct_value);
		}
		if(lxa_compressiontype_supported(LXA_COMPRESSIONTYPE_BZIP2))
		{
			gtk_list_store_append(dialog->filetype_model, &iter);
			g_value_set_string(str_value, _("Bzip2 compressed Tar '.tar.bz2'"));
			gtk_list_store_set_value(dialog->filetype_model, &iter, 0, str_value);
			g_value_set_uint(at_value, LXA_ARCHIVETYPE_TAR);
			g_value_set_uint(ct_value, LXA_COMPRESSIONTYPE_BZIP2);
			gtk_list_store_set_value(dialog->filetype_model, &iter, 1, at_value);
			gtk_list_store_set_value(dialog->filetype_model, &iter, 2, ct_value);
		}
	}
	if(lxa_archivetype_supported(LXA_ARCHIVETYPE_ZIP))
	{
		gtk_list_store_append(dialog->filetype_model, &iter);
		g_value_set_string(str_value, _("Zip archive '.zip'"));
		gtk_list_store_set_value(dialog->filetype_model, &iter, 0, str_value);
		g_value_set_uint(at_value, LXA_ARCHIVETYPE_ZIP);
		g_value_set_uint(ct_value, LXA_COMPRESSIONTYPE_NONE);
		gtk_list_store_set_value(dialog->filetype_model, &iter, 1, at_value);
		gtk_list_store_set_value(dialog->filetype_model, &iter, 2, ct_value);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->filetype_selector), 0);
	gtk_box_pack_start (GTK_BOX (hbox), dialog->filetype_selector,TRUE , TRUE, 0);
	
	gtk_widget_show_all(hbox);
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 0);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_NEW, GTK_RESPONSE_OK,
			NULL);
	g_free(str_value);
	g_free(at_value);
	g_free(ct_value);
}

GtkWidget *
xa_new_archive_dialog_new()
{
	GtkWidget *dialog;
	GtkFileFilter *filter = NULL;

	dialog = g_object_new(xa_new_archive_dialog_get_type(), "title", _("Create new archive"), "action", GTK_FILE_CHOOSER_ACTION_SAVE, "do-overwrite-confirmation", TRUE, NULL);

	if(lxa_archivetype_supported(LXA_ARCHIVETYPE_TAR))
	{
		filter = gtk_file_filter_new();
		gtk_file_filter_add_mime_type(filter, "application/x-tar");
		gtk_file_filter_set_name(filter, _("Tar (uncompressed) '.tar'"));
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		if(lxa_compressiontype_supported(LXA_COMPRESSIONTYPE_GZIP))
		{
			filter = gtk_file_filter_new();
			gtk_file_filter_add_mime_type(filter, "application/x-compressed-tar");
			gtk_file_filter_set_name(filter, _("Gzip compressed Tar '.tar.gz'"));
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
		if(lxa_compressiontype_supported(LXA_COMPRESSIONTYPE_BZIP2))
		{
			filter = gtk_file_filter_new();
			gtk_file_filter_add_mime_type(filter, "application/x-bzip-compressed-tar");
			gtk_file_filter_set_name(filter, _("Bzip2 compressed Tar '.tar.bz2'"));
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}
	if(lxa_archivetype_supported(LXA_ARCHIVETYPE_ZIP))
	{
		filter = gtk_file_filter_new();
		gtk_file_filter_add_mime_type(filter, "application/x-zip");
		gtk_file_filter_add_mime_type(filter, "application/zip");
		gtk_file_filter_set_name(filter, _("Zip archive '.zip'"));
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
		
	return dialog;
}

LXAArchiveType
xa_new_archive_dialog_get_archive_type (XANewArchiveDialog *dialog)
{
	GtkTreeIter iter;
	GValue *at_value = g_new0(GValue, 1);
	LXAArchiveType type;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dialog->filetype_selector), &iter);
	gtk_tree_model_get_value(GTK_TREE_MODEL(dialog->filetype_model), &iter, 1, at_value);
	type = g_value_get_uint(at_value);
	g_free(at_value);
	return type; }

LXACompressionType
xa_new_archive_dialog_get_compression_type (XANewArchiveDialog *dialog)
{
	GtkTreeIter iter;
	GValue *ct_value = g_new0(GValue, 1); 
	LXACompressionType type;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dialog->filetype_selector), &iter);
	gtk_tree_model_get_value(GTK_TREE_MODEL(dialog->filetype_model), &iter, 2, ct_value);
	type = g_value_get_uint(ct_value);
	g_free(ct_value);
	return type;
}

gchar *
xa_new_archive_dialog_get_filename (XANewArchiveDialog *dialog)
{
	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	return filename;
}
