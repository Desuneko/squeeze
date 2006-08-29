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
 /*
	TODO: Fix props
 */

#include <config.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>
#include "extract_dialog.h"

static void
xa_extract_archive_dialog_class_init(XAExtractArchiveDialogClass *archive_class);

static void
xa_extract_archive_dialog_init(XAExtractArchiveDialog *archive);

void
xa_extract_dialog_option_toggled (GtkWidget *widget, gpointer data);

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
	GtkWidget *hbox = gtk_hbox_new(TRUE, 5);
	GtkWidget *l_label = gtk_label_new(_("<b>Extract files:</b>"));
	GtkWidget *r_label = gtk_label_new(_("<b>Options:</b>"));
	gtk_label_set_use_markup(GTK_LABEL(l_label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(r_label), TRUE);

	GtkWidget *l_vbox = gtk_vbox_new(FALSE, 0);

	dialog->l_frame = gtk_frame_new( NULL );
	gtk_frame_set_label_widget(GTK_FRAME(dialog->l_frame), l_label);
	gtk_box_pack_start(GTK_BOX(hbox), dialog->l_frame, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(dialog->l_frame), l_vbox);
	dialog->all_files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("All files"));
	dialog->sel_files_radio = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON(dialog->all_files_radio), _("Selected files"));

	gtk_box_pack_start(GTK_BOX(l_vbox), dialog->all_files_radio, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(l_vbox), dialog->sel_files_radio, FALSE, FALSE, 0);

	dialog->r_frame = gtk_frame_new( NULL );
	gtk_frame_set_label_widget(GTK_FRAME(dialog->r_frame), r_label);
	gtk_box_pack_start(GTK_BOX(hbox), dialog->r_frame, TRUE, TRUE, 0);

	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			_("Extract"), GTK_RESPONSE_OK,
			NULL);
	gtk_widget_show_all(hbox);
}

GtkWidget *
xa_extract_archive_dialog_new(LXAArchiveSupport *support, LXAArchive *archive, gboolean sel_option)
{
	GSList *extract_options;
	GtkWidget *test;
	XAExtractArchiveDialog *dialog;

	dialog = g_object_new(xa_extract_archive_dialog_get_type(), "title", _("Extract archive"), "action", GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, "do-overwrite-confirmation", TRUE, NULL);
/* Handle 'extract selected files' option */
	gtk_widget_set_sensitive(dialog->sel_files_radio, sel_option);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->sel_files_radio), sel_option);

	GtkWidget *r_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(dialog->r_frame), r_vbox);

	dialog->support = support;
	if(dialog->support)
	{
		extract_options = lxa_archive_support_list_properties(support, "extract");
		while(extract_options)
		{
			switch(G_PARAM_SPEC(extract_options->data)->value_type)
			{
				case (G_TYPE_BOOLEAN):
					test = gtk_check_button_new_with_label(g_param_spec_get_nick(G_PARAM_SPEC(extract_options->data)));
					g_signal_connect(G_OBJECT(test), "toggled", G_CALLBACK(xa_extract_dialog_option_toggled), (void *)g_param_spec_get_name(G_PARAM_SPEC(extract_options->data)));
					gtk_box_pack_start(GTK_BOX(r_vbox), test, FALSE, FALSE, 0);
					break;
				case (G_TYPE_STRING): /* TODO: Add text-field */
					test = gtk_label_new(g_param_spec_get_nick(G_PARAM_SPEC(extract_options->data)));
					gtk_box_pack_start(GTK_BOX(r_vbox), test, FALSE, FALSE, 0);
					break;
			}
			extract_options = extract_options->next;
		}
	}
	gtk_widget_show_all(r_vbox);
	return GTK_WIDGET(dialog);
}

void
xa_extract_dialog_option_toggled (GtkWidget *widget, gpointer data)
{
	GValue *val = g_new0(GValue, 1);
	gboolean active;

	val = g_value_init(val, G_TYPE_BOOLEAN);

	g_value_set_boolean(val, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));

	g_object_set_property(G_OBJECT(XA_EXTRACT_ARCHIVE_DIALOG(gtk_widget_get_ancestor(widget, GTK_TYPE_DIALOG))->support), (gchar *)data, val);

	g_free(val);
}
