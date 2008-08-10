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
 /*
	TODO: Fix props
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>

#include "extract_dialog.h"
#include "widget_factory.h"

static void
sq_extract_archive_dialog_class_init(SQExtractArchiveDialogClass *archive_class);

static void
sq_extract_archive_dialog_init(SQExtractArchiveDialog *archive);

void
sq_extract_dialog_option_toggled (GtkWidget *widget, gpointer data);
void
sq_extract_dialog_option_child_notify(GtkWidget *widget, GParamSpec *, gpointer data);

GType
sq_extract_archive_dialog_get_type ()
{
	static GType sq_extract_archive_dialog_type = 0;

 	if (!sq_extract_archive_dialog_type)
	{
 		static const GTypeInfo sq_extract_archive_dialog_info = 
		{
			sizeof (SQExtractArchiveDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_extract_archive_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQExtractArchiveDialog),
			0,
			(GInstanceInitFunc) sq_extract_archive_dialog_init,
			NULL
		};

		sq_extract_archive_dialog_type = g_type_register_static (GTK_TYPE_FILE_CHOOSER_DIALOG, "SQExtractArchiveDialog", &sq_extract_archive_dialog_info, 0);
	}
	return sq_extract_archive_dialog_type;
}

static void
sq_extract_archive_dialog_class_init(SQExtractArchiveDialogClass *dialog_class)
{
}

static void
sq_extract_archive_dialog_init(SQExtractArchiveDialog *dialog)
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
sq_extract_archive_dialog_new(LSQArchive *archive, gboolean sel_option)
{
	SQExtractArchiveDialog *dialog;

	dialog = g_object_new(sq_extract_archive_dialog_get_type(), "title", _("Extract archive"), "action", GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, "do-overwrite-confirmation", TRUE, NULL);
/* Handle 'extract selected files' option */
	gtk_widget_set_sensitive(dialog->sel_files_radio, sel_option);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->sel_files_radio), sel_option);

	GtkWidget *r_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(dialog->r_frame), r_vbox);

  if((dialog->command_options = lsq_archive_get_command_options(archive, LSQ_COMMAND_TYPE_EXTRACT)))
  {
    LSQCommandOptionPair **options_iter;
    for(options_iter = dialog->command_options; *options_iter; options_iter++)
    {
      GtkWidget *widget;
      widget = gtk_check_button_new_with_label((*options_iter)->option->blurb);
      gtk_box_pack_start(GTK_BOX(r_vbox), widget, FALSE, FALSE, 0);
    }
  }

	/* FIXME, does not work correctly when there are more dots in a filename then the one identifying the extention */
	gchar **filename_components = g_strsplit(g_path_get_basename(lsq_archive_get_filename(archive)), ".", 2);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename_components[0]);
	g_strfreev(filename_components);

	gtk_widget_show_all(r_vbox);
	return GTK_WIDGET(dialog);
}

void
sq_extract_dialog_option_toggled (GtkWidget *widget, gpointer data)
{
	GValue *val = g_new0(GValue, 1);

	val = g_value_init(val, G_TYPE_BOOLEAN);

	g_value_set_boolean(val, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));

	g_free(val);
}

void
sq_extract_dialog_option_child_notify (GtkWidget *widget, GParamSpec *pspec, gpointer data)
{
	GValue *val = g_new0(GValue, 1);
	if(strcmp(g_param_spec_get_name(pspec), "text"))
	{
		val = g_value_init(val, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(widget), "text", val);
	}
	g_free(val);
}
