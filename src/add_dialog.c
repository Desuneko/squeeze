/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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
#include <libxarchiver/libxarchiver.h>
#include <gettext.h>

#include "add_dialog.h"
#include "widget_factory.h"

static void
xa_add_dialog_class_init(XAAddDialogClass *archive_class);

static void
xa_add_dialog_init(XAAddDialog *archive);

GType
xa_add_dialog_get_type ()
{
	static GType xa_add_dialog_type = 0;

 	if (!xa_add_dialog_type)
	{
 		static const GTypeInfo xa_add_dialog_info = 
		{
			sizeof (XAAddDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_add_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAAddDialog),
			0,
			(GInstanceInitFunc) xa_add_dialog_init,
			NULL
		};

		xa_add_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "XAAddDialog", &xa_add_dialog_info, 0);
	}
	return xa_add_dialog_type;
}

static void
xa_add_dialog_class_init(XAAddDialogClass *dialog_class)
{
}

static void
xa_add_dialog_init(XAAddDialog *dialog)
{
	GtkWidget *frame = gtk_frame_new(_("Files and directories to add"));
	dialog->optionframe = gtk_frame_new(_("Options:"));
	GtkWidget *vbox = gtk_vbox_new(FALSE,0);
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	gtk_container_add(GTK_CONTAINER(vbox), scrolled_window);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	gtk_widget_show_all(frame);
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
xa_add_dialog_new(LXAArchiveSupport *support)
{
	GSList *add_options;
	XAAddDialog *dialog;
	GtkWidget *optionbox, *test;
	XAWidgetFactory *factory = xa_widget_factory_new();

	dialog = g_object_new(xa_add_dialog_get_type(),
			"title", _("Add file(s) to archive"),
			NULL);

	optionbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(dialog->optionframe), optionbox);

	dialog->support = support;
	if(support)
	{
		add_options = lxa_archive_support_list_properties(support, "add");
		while(add_options)
		{
			test = xa_widget_factory_create_property_widget(factory, G_OBJECT(support), g_param_spec_get_name(G_PARAM_SPEC(add_options->data)));
			gtk_box_pack_start(GTK_BOX(optionbox), test, FALSE, FALSE, 0);
			add_options = add_options->next;
		}
	}

	gtk_widget_set_size_request(GTK_WIDGET(dialog), 600,400);

	gtk_widget_show_all(optionbox);
	return (GtkWidget*)dialog;
}
