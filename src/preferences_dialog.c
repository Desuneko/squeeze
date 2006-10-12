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
#include <glib.h>
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>

#ifdef HAVE_THUNAR_VFS
#define EXO_API_SUBJECT_TO_CHANGE
#include <thunar-vfs/thunar-vfs.h>
#else
#include <gettext.h>
#endif

#include "preferences_dialog.h"

static void
xa_preferences_dialog_class_init(XAPreferencesDialogClass *archive_class);

static void
xa_preferences_dialog_init(XAPreferencesDialog *archive);

GType
xa_preferences_dialog_get_type ()
{
	static GType xa_preferences_dialog_type = 0;

 	if (!xa_preferences_dialog_type)
	{
 		static const GTypeInfo xa_preferences_dialog_info = 
		{
			sizeof (XAPreferencesDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_preferences_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAPreferencesDialog),
			0,
			(GInstanceInitFunc) xa_preferences_dialog_init,
			NULL
		};

		xa_preferences_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "XAPreferencesDialog", &xa_preferences_dialog_info, 0);
	}
	return xa_preferences_dialog_type;
}

static void
xa_preferences_dialog_class_init(XAPreferencesDialogClass *dialog_class)
{
}

static void
xa_preferences_dialog_init(XAPreferencesDialog *dialog)
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

	box = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("_Archivers"));
	gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook), box, label);

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
xa_preferences_dialog_new()
{
	GtkWidget *dialog;

	dialog = g_object_new(xa_preferences_dialog_get_type(),
			"title", _("Preferences"),
			NULL);

	return dialog;
}
