/*
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
#include <gio/gio.h>
#include <libsqueeze/libsqueeze.h>
#include <gdk-pixbuf/gdk-pixbuf.h>


#include "properties_dialog.h"

static void
sq_properties_dialog_class_init(SQPropertiesDialogClass *archive_class);

static void
sq_properties_dialog_init(SQPropertiesDialog *archive);

GType
sq_properties_dialog_get_type (void)
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
	GtkWidget *box;
	GtkWidget *label;

	dialog->table = (GtkTable *)gtk_table_new(2, 2, FALSE);

	gtk_table_set_col_spacings (dialog->table, 12);
	gtk_container_set_border_width(GTK_CONTAINER(dialog->table), 6);
	gtk_container_add(GTK_CONTAINER(((GtkDialog *)dialog)->vbox), GTK_WIDGET(dialog->table));
	gtk_widget_show (GTK_WIDGET(dialog->table));

	box = gtk_hbox_new(6, FALSE);
	gtk_table_attach (dialog->table, box, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 3);
	gtk_widget_show (box);

	dialog->icon_image = gtk_image_new();
	gtk_box_pack_start (GTK_BOX (box), dialog->icon_image, FALSE, TRUE, 0);
	gtk_widget_show (dialog->icon_image);

	label = gtk_label_new(_("Name:"));
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);
	gtk_widget_show (label);

	dialog->filename_label = gtk_label_new("");
	gtk_table_attach (dialog->table, dialog->filename_label, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 3);
	gtk_widget_show(dialog->filename_label);

	dialog->prop_table = (GtkTable *)gtk_table_new(2, 1, TRUE);
	gtk_table_set_col_spacings (dialog->prop_table, 12);
	gtk_table_attach (dialog->table, GTK_WIDGET(dialog->prop_table), 0, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 3);
	gtk_widget_show(GTK_WIDGET(dialog->prop_table));

	label = gtk_label_new(_("Kind:"));
	gtk_misc_set_alignment(GTK_MISC(label), 1.0f, 0.5f);
	gtk_table_attach (dialog->prop_table, label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 3);
	gtk_widget_show (label);

	dialog->mimetype_label = gtk_label_new("");
	gtk_table_attach (dialog->prop_table, dialog->mimetype_label, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 3);
	gtk_widget_show(dialog->mimetype_label);


	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
		GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
			NULL);
}

GtkWidget *
sq_properties_dialog_new(LSQArchive *archive, GtkIconTheme *icon_theme)
{
	GtkWidget *dialog;
	gchar *filename;

	dialog = g_object_new(sq_properties_dialog_get_type(),
			"title", _("Properties"),
			NULL);

	gtk_widget_set_size_request(dialog, 220, 400);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	GdkPixbuf *icon = gtk_icon_theme_load_icon(icon_theme, thunar_vfs_mime_info_lookup_icon_name(archive->mime_info, icon_theme), 48, 0, NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(((SQPropertiesDialog *)dialog)->icon_image), icon);

	filename = lsq_archive_get_filename(archive);
	gtk_label_set_text(GTK_LABEL(((SQPropertiesDialog *)dialog)->filename_label), filename);
	g_free(filename);
	gtk_label_set_text(GTK_LABEL(((SQPropertiesDialog *)dialog)->mimetype_label), lsq_archive_get_mimetype(archive));

	return dialog;
}
