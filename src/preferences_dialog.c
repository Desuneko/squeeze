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

#include "button_drag_box.h"
#include "preferences_dialog.h"

static void
sq_preferences_dialog_class_init(SQPreferencesDialogClass *archive_class);

static void
sq_preferences_dialog_init(SQPreferencesDialog *archive);

static void
cb_sq_preferences_dialog_item_changed(GtkWidget *widget, gpointer user_data);

inline static void
sq_preferences_dialog_create_support_page(SQPreferencesDialog *dialog);
/*
inline static void
sq_preferences_dialog_create_general_page(SQPreferencesDialog *dialog);
*/
static GtkWidget *
sq_preferences_dialog_create_support_object_page(SQSupportTuple *tuple);

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

	sq_preferences_dialog_create_support_page(dialog);

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

/*
inline static void
sq_preferences_dialog_create_general_page(SQPreferencesDialog *dialog)
{
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

#ifdef SQ_MAIN_ANY_BAR
  frame = sq_widget_factory_create_property_widget(window->widget_factory, G_OBJECT(window), "navigation-style");
	gtk_box_pack_start(GTK_BOX(box), frame,FALSE, FALSE, 0);
#endif
}
*/

inline static void
sq_preferences_dialog_create_support_page(SQPreferencesDialog *dialog)
{
	GtkWidget *iconview;
	GtkWidget *label;
	GtkWidget *box = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("_Archivers"));
	gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook), box, label);

	GtkTreeModel *store = GTK_TREE_MODEL(gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING));
	GtkCellRenderer *render = gtk_cell_renderer_pixbuf_new();

	iconview = gtk_icon_view_new_with_model(store);
	g_signal_connect(G_OBJECT(iconview), "selection-changed", (GCallback)cb_sq_preferences_dialog_item_changed, dialog);

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scroll, 84, 84);

	gtk_container_add(GTK_CONTAINER(scroll), iconview);
	/* gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), iconview); */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(scroll), GTK_CORNER_TOP_RIGHT);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);

	render = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(render), "ellipsize", PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
	gtk_icon_view_set_item_width(GTK_ICON_VIEW(iconview), 48);
	/* gtk_icon_view_set_orientation(GTK_ICON_VIEW(iconview), GTK_ORIENTATION_HORIZONTAL); */
	gtk_icon_view_set_columns(GTK_ICON_VIEW(iconview), 1);
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(iconview), 0);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(iconview), 1);

	dialog->support.notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(dialog->support.notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(dialog->support.notebook), FALSE);

	dialog->support.support_list = NULL;

	GSList *sup_iter, *support_list = sup_iter = lsq_get_registered_support_list();

	GtkTreeIter iter;
	LSQArchiveSupport *support;
	SQSupportTuple *tuple;

	while(sup_iter)
	{
		tuple = g_new(SQSupportTuple, 1);
		tuple->support = support = LSQ_ARCHIVE_SUPPORT(sup_iter->data);
		gtk_list_store_append(GTK_LIST_STORE(store), &iter);
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 1, lsq_archive_support_get_id(support), -1);
		sup_iter = g_slist_next(sup_iter);

		gtk_notebook_append_page(GTK_NOTEBOOK(dialog->support.notebook), sq_preferences_dialog_create_support_object_page(tuple), NULL);

		dialog->support.support_list = g_slist_prepend(dialog->support.support_list, tuple);
	}

	g_slist_free(support_list);

	GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);

	gtk_icon_view_set_cursor(GTK_ICON_VIEW(iconview), path, NULL, FALSE);
	gtk_icon_view_select_path(GTK_ICON_VIEW(iconview), path);

	gtk_tree_path_free(path);

	gtk_widget_show(iconview);

	gtk_box_pack_start(GTK_BOX(box), scroll, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), dialog->support.notebook, TRUE, TRUE, 0);
	gtk_widget_show_all(box);
}
#if 1
static GtkWidget *
sq_preferences_dialog_create_support_object_page(SQSupportTuple *tuple)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

	tuple->box = sq_button_drag_box_new();
	SQButtonDragBox *button_box = SQ_BUTTON_DRAG_BOX(tuple->box);

	gtk_box_pack_start(GTK_BOX(vbox), tuple->box, FALSE, FALSE, 0);

	sq_button_drag_box_add_fixed_button(button_box, _("Filename"), NULL);
	sq_button_drag_box_lock_buttons(button_box, 1);

	GSList *iter, *view_props = iter = lsq_archive_support_list_properties(tuple->support, "view");
	GParamSpec *spec;
	gboolean visible;

	while(iter)
	{
		spec = G_PARAM_SPEC(iter->data);
		g_object_get(G_OBJECT(tuple->support), g_param_spec_get_name(spec), &visible, NULL);
		sq_button_drag_box_add_button(button_box, g_param_spec_get_nick(spec), visible, (gpointer)g_param_spec_get_name(spec));
		iter = g_slist_next(iter);
	}

	g_slist_free(view_props);

	return vbox;
}
#else
static GtkWidget *
sq_preferences_dialog_create_support_object_page(SQSupportTuple *tuple)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

	GtkTreeModel *store = GTK_TREE_MODEL(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING));

	tuple->box = gtk_tree_view_new_with_model(store);
	GtkTreeView *tree_view = GTK_TREE_VIEW(tuple->box);
	gtk_tree_view_set_reorderable(tree_view, TRUE);

	GtkCellRenderer *render = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Column", render, "text", 0, NULL);
	gtk_tree_view_append_column(tree_view, column);

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scroll, 100, 100);

	gtk_container_add(GTK_CONTAINER(scroll), tuple->box);
	/* gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), iconview); */
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);

	render = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Discription", render, "text", 1, NULL);
	gtk_tree_view_append_column(tree_view, column);

	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

	GSList *iter, *view_props = iter = lsq_archive_support_list_properties(tuple->support, "view");
	GParamSpec *spec;
	gboolean visible;
	GtkTreeIter titer;

	while(iter)
	{
		spec = G_PARAM_SPEC(iter->data);
		g_object_get(G_OBJECT(tuple->support), g_param_spec_get_name(spec), &visible, NULL);

		gtk_list_store_append(GTK_LIST_STORE(store), &titer);
		gtk_list_store_set(GTK_LIST_STORE(store), &titer, 0, g_param_spec_get_nick(spec), 1, g_param_spec_get_blurb(spec), -1);

		iter = g_slist_next(iter);
	}

	g_slist_free(view_props);

	return vbox;
}
#endif
static void
cb_sq_preferences_dialog_item_changed(GtkWidget *widget, gpointer user_data)
{
	SQPreferencesDialog *dialog = SQ_PREFERENCES_DIALOG(user_data);
	GtkTreePath *path;
	gtk_icon_view_get_cursor(GTK_ICON_VIEW(widget), &path, NULL);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(dialog->support.notebook), gtk_tree_path_get_indices(path)[0]);
	gtk_tree_path_free(path);
}

