/*
 *  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by *  the Free Software Foundation; either version 2 of the License, or
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
#include <gettext.h>

#include "add_dialog.h"
#include "widget_factory.h"

static void
sq_add_dialog_class_init(SQAddDialogClass *archive_class);

static void
sq_add_dialog_init(SQAddDialog *archive);

GType
sq_add_dialog_get_type ()
{
	static GType sq_add_dialog_type = 0;

 	if (!sq_add_dialog_type)
	{
 		static const GTypeInfo sq_add_dialog_info = 
		{
			sizeof (SQAddDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_add_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQAddDialog),
			0,
			(GInstanceInitFunc) sq_add_dialog_init,
			NULL
		};

		sq_add_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "SQAddDialog", &sq_add_dialog_info, 0);
	}
	return sq_add_dialog_type;
}

static void
sq_add_dialog_class_init(SQAddDialogClass *dialog_class)
{
}

static void
sq_add_dialog_init(SQAddDialog *dialog)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *frame = gtk_frame_new(_("Files and directories to add"));
	dialog->optionframe = gtk_frame_new(_("Options:"));
	GtkWidget *vbox = gtk_vbox_new(FALSE,5);
	GtkWidget *hbox = gtk_hbox_new(FALSE, 3);
	GtkWidget *radio_vbox = gtk_vbox_new(FALSE,0);
	dialog->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(dialog->scrolled_window), GTK_SHADOW_IN);

	gtk_container_add(GTK_CONTAINER(vbox), dialog->scrolled_window);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	dialog->file_liststore = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

	dialog->file_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dialog->file_liststore));
	column = gtk_tree_view_column_new();

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "stock-id", 0);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 1);

	gtk_tree_view_append_column(GTK_TREE_VIEW(dialog->file_treeview), column);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog->file_treeview), FALSE);
	gtk_container_add (GTK_CONTAINER (dialog->scrolled_window), dialog->file_treeview);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	dialog->files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Files"));
	gtk_box_pack_start (GTK_BOX (radio_vbox), dialog->files_radio, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (dialog->files_radio), FALSE);

	dialog->dirs_radio = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON(dialog->files_radio), _("Directories"));
	gtk_box_pack_start (GTK_BOX (radio_vbox), dialog->dirs_radio, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (dialog->dirs_radio), FALSE);

	gtk_box_pack_start(GTK_BOX(hbox), radio_vbox, FALSE, FALSE, 5);
	dialog->remove = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	dialog->add    = gtk_button_new_from_stock(GTK_STOCK_ADD);

	gtk_box_pack_end(GTK_BOX(hbox), dialog->add, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), dialog->remove, FALSE, FALSE, 0);

	gtk_widget_show_all(dialog->scrolled_window);
	gtk_widget_show_all(frame);
	gtk_widget_show_all(hbox);
	gtk_widget_show_all(dialog->optionframe);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->vbox), dialog->optionframe, TRUE, TRUE, 0);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_ADD, GTK_RESPONSE_OK,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

GtkWidget *
sq_add_dialog_new(LSQArchiveSupport *support)
{
	GSList *add_options;
	SQAddDialog *dialog;
	GtkWidget *optionbox, *test;
	SQWidgetFactory *factory = sq_widget_factory_new();

	dialog = g_object_new(sq_add_dialog_get_type(),
			"title", _("Add file(s) to archive"),
			NULL);

	optionbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(dialog->optionframe), optionbox);

	dialog->support = support;
	if(support)
	{
		add_options = lsq_archive_support_list_properties(support, "add");
		while(add_options)
		{
			test = sq_widget_factory_create_property_widget(factory, G_OBJECT(support), g_param_spec_get_name(G_PARAM_SPEC(add_options->data)));
			gtk_box_pack_start(GTK_BOX(optionbox), test, FALSE, FALSE, 0);
			add_options = add_options->next;
		}
	}

	gtk_widget_set_size_request(GTK_WIDGET(dialog), 400,300);

	gtk_widget_show_all(optionbox);
	return (GtkWidget*)dialog;
}
