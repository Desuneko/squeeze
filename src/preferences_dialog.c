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
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "preferences_dialog.h"
#include "button_drag_box.h"

static void
sq_preferences_dialog_class_init(SQPreferencesDialogClass *archive_class);

static void
sq_preferences_dialog_init(SQPreferencesDialog *archive);

static void
cb_sq_preferences_dialog_item_activated(GtkWidget *widget, GtkTreePath *path, gpointer user_data);

static GtkWidget *
sq_preferences_dialog_create_page(LSQArchiveSupport *support);

GType
sq_preferences_dialog_get_type ()
{
	static GType sq_preferences_dialog_type = 0;

 	if (!sq_preferences_dialog_type)
	{
 		static const GTypeInfo sq_preferences_dialog_info = 
		{
			sizeof (SQPreferencesDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_preferences_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQPreferencesDialog),
			0,
			(GInstanceInitFunc) sq_preferences_dialog_init,
			NULL
		};

		sq_preferences_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "SQPreferencesDialog", &sq_preferences_dialog_info, 0);
	}
	return sq_preferences_dialog_type;
}

static void
sq_preferences_dialog_class_init(SQPreferencesDialogClass *dialog_class)
{
}

static void
sq_preferences_dialog_init(SQPreferencesDialog *dialog)
{
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *iconview;
	dialog->notebook = gtk_notebook_new();

	box = gtk_vbox_new(FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("_General"));
	gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook), box, label);

	frame  = gtk_frame_new(_("Archive viewer:"));
	gtk_box_pack_start(GTK_BOX(box), frame,FALSE, FALSE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);

	GtkWidget *_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), _vbox);
	gtk_container_set_border_width(GTK_CONTAINER(_vbox), 0);

	dialog->general.viewer.show_icons = gtk_check_button_new_with_mnemonic(_("_Show Icons"));
	gtk_box_pack_start(GTK_BOX(_vbox), dialog->general.viewer.show_icons, FALSE, FALSE, 0);

	dialog->general.viewer.rules_hint = gtk_check_button_new_with_mnemonic(_("_Rules Hint"));
	gtk_box_pack_start(GTK_BOX(_vbox), dialog->general.viewer.rules_hint, FALSE, FALSE, 0);

	frame  = gtk_frame_new(_("Sorting:"));
	gtk_box_pack_start(GTK_BOX(_vbox), frame,FALSE, FALSE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);

	GtkWidget *sort_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), sort_vbox);
	gtk_container_set_border_width(GTK_CONTAINER(sort_vbox), 0);

	dialog->general.viewer.sorting.sort_case = gtk_check_button_new_with_mnemonic(_("Sort _Case Sensitive "));
	gtk_box_pack_start(GTK_BOX(sort_vbox), dialog->general.viewer.sorting.sort_case, FALSE, FALSE, 0);

	dialog->general.viewer.sorting.sort_folders = gtk_check_button_new_with_mnemonic(_("Sort _Folders First"));
	gtk_box_pack_start(GTK_BOX(sort_vbox), dialog->general.viewer.sorting.sort_folders, FALSE, FALSE, 0);

	frame  = gtk_frame_new(_("Navigation bar:"));
	gtk_box_pack_start(GTK_BOX(box), frame,FALSE, FALSE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);

	GtkWidget *nav_vbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), nav_vbox);
	gtk_container_set_border_width(GTK_CONTAINER(nav_vbox), 0);

	box = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("_Archivers"));
	gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook), box, label);

	GtkTreeModel *store = GTK_TREE_MODEL(gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING));
	GtkCellRenderer *render = gtk_cell_renderer_pixbuf_new();

	iconview = gtk_icon_view_new_with_model(store);
	g_signal_connect(G_OBJECT(iconview), "item-activated", (GCallback)cb_sq_preferences_dialog_item_activated, dialog);

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scroll, 84, 84);

	gtk_container_add(GTK_CONTAINER(scroll), iconview);
	/* gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), iconview); */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(scroll), GTK_CORNER_TOP_RIGHT);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);

	render = gtk_cell_renderer_text_new();
	gtk_icon_view_set_item_width(GTK_ICON_VIEW(iconview), 48);
	gtk_icon_view_set_orientation(GTK_ICON_VIEW(iconview), GTK_ORIENTATION_VERTICAL);
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(iconview), 0);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(iconview), 1);

	/* TODO: auto gen this here */

	GtkTreeIter iter;
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);
	gtk_list_store_set(GTK_LIST_STORE(store), &iter, 1, "zip", -1);
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);
	gtk_list_store_set(GTK_LIST_STORE(store), &iter, 1, "tar", -1);
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);
	gtk_list_store_set(GTK_LIST_STORE(store), &iter, 1, "ehmm", -1);

	gtk_widget_show(iconview);

	dialog->support.notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(dialog->support.notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(dialog->support.notebook), FALSE);

	gtk_notebook_append_page(GTK_NOTEBOOK(dialog->support.notebook), sq_preferences_dialog_create_page(NULL), NULL);

	gtk_box_pack_start(GTK_BOX(box), scroll, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), dialog->support.notebook, TRUE, TRUE, 0);
	gtk_widget_show_all(box);

	box = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("_Behaviour"));
	gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook), box, label);

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), dialog->notebook);
	gtk_widget_show_all(dialog->notebook);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
			GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

GtkWidget *
sq_preferences_dialog_new()
{
	GtkWidget *dialog;

	dialog = g_object_new(sq_preferences_dialog_get_type(),
			"title", _("Preferences"),
			NULL);

	return dialog;
}

static GtkWidget *
sq_preferences_dialog_create_page(LSQArchiveSupport *support)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

	GtkWidget *button_box = sq_button_drag_box_new();

	gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

	return vbox;
}

static void
cb_sq_preferences_dialog_item_activated(GtkWidget *widget, GtkTreePath *path, gpointer user_data)
{
	SQPreferencesDialog *dialog = SQ_PREFERENCES_DIALOG(user_data);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(dialog->support.notebook), gtk_tree_path_get_indices(path)[0]);
}

