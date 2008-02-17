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
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>

#include "message_dialog.h"
#include "widget_factory.h"

static void
sq_message_dialog_class_init(SQMessageDialogClass *archive_class);

static void
sq_message_dialog_init(SQMessageDialog *archive);

static void
sq_message_dialog_dispose(GObject *dialog);

static gboolean
sq_message_dialog_progressbar_pulse(SQMessageDialog *dialog);
static void
cb_sq_message_dialog_close(GtkButton *button, SQMessageDialog *dialog);

static GObjectClass *parent_class = NULL;

GType
sq_message_dialog_get_type ()
{
	static GType sq_message_dialog_type = 0;

 	if (!sq_message_dialog_type)
	{
 		static const GTypeInfo sq_message_dialog_info = 
		{
			sizeof (SQMessageDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_message_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQMessageDialog),
			0,
			(GInstanceInitFunc) sq_message_dialog_init,
			NULL
		};

		sq_message_dialog_type = g_type_register_static (GTK_TYPE_WINDOW, "SQMessageDialog", &sq_message_dialog_info, 0);
	}
	return sq_message_dialog_type;
}

static void
sq_message_dialog_class_init(SQMessageDialogClass *dialog_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (dialog_class);

	parent_class = gtk_type_class (GTK_TYPE_WINDOW);

	object_class->dispose = sq_message_dialog_dispose;
}

static void
sq_message_dialog_init(SQMessageDialog *dialog)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 3);
	dialog->message_label = gtk_label_new("being silly here");
	dialog->progress_bar = gtk_progress_bar_new();

	gtk_label_set_ellipsize(GTK_LABEL(dialog->message_label), PANGO_ELLIPSIZE_MIDDLE);

	GtkWidget *separator = gtk_hseparator_new();
	GtkWidget *button_box= gtk_hbutton_box_new();

	gtk_box_pack_end(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), dialog->message_label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), dialog->progress_bar, TRUE, FALSE, 5);


	gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);

	GtkWidget *cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	g_signal_connect(cancel_button, "clicked", G_CALLBACK(cb_sq_message_dialog_close), dialog);

	gtk_container_add(GTK_CONTAINER(button_box), cancel_button);

	gtk_widget_show_all(vbox);

	gtk_container_add(GTK_CONTAINER(dialog), vbox);
}

static void
sq_message_dialog_dispose(GObject *dialog)
{
	if(SQ_MESSAGE_DIALOG(dialog)->archive)
	{
		GtkWidget *warning_dialog = gtk_message_dialog_new(GTK_WINDOW(dialog), 
									   GTK_DIALOG_MODAL,
									   GTK_MESSAGE_WARNING,
									   GTK_BUTTONS_YES_NO,
									   "Are you sure you want to cancel this operation?");
		if(gtk_dialog_run(GTK_DIALOG(warning_dialog)) == GTK_RESPONSE_YES)
		{
			lsq_archive_stop(SQ_MESSAGE_DIALOG(dialog)->archive);
			SQ_MESSAGE_DIALOG(dialog)->archive = NULL;

		}
		gtk_widget_destroy(warning_dialog);


		g_timeout_add(200, (GSourceFunc)sq_message_dialog_progressbar_pulse, dialog);
	}
	else
		parent_class->dispose(dialog);
}

static gboolean
sq_message_dialog_progressbar_pulse(SQMessageDialog *dialog)
{
	if(dialog->archive)
	{
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(dialog->progress_bar));
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(dialog->progress_bar), lsq_archive_get_state_msg(dialog->archive));
		return TRUE;
	}
	return FALSE;
}

static void
cb_sq_message_dialog_close(GtkButton *button, SQMessageDialog *dialog)
{
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

GtkWidget *
sq_message_dialog_new(GtkWindowType type, LSQArchive *archive)
{
	SQMessageDialog *dialog;

	dialog = g_object_new(sq_message_dialog_get_type(),
			"title", _("Archive manager"),
			"type", type,
			"resizable", FALSE,
			"deletable", FALSE,
			"modal", TRUE,
			NULL);

	gtk_widget_set_size_request(GTK_WIDGET(dialog), 300,100);

	SQ_MESSAGE_DIALOG(dialog)->archive = archive;

	g_timeout_add(200, (GSourceFunc)sq_message_dialog_progressbar_pulse, dialog);

	gtk_label_set_text(GTK_LABEL(dialog->message_label), lsq_archive_get_path(archive));

	return (GtkWidget*)dialog;
}
