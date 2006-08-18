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
#include "main_window_menu_bar.h"
#include "main_window.h"

static void
xa_main_window_menu_bar_class_init(XAMainWindowMenuBarClass *);

static void
xa_main_window_menu_bar_init(XAMainWindowMenuBar *);

GType
xa_main_window_menu_bar_get_type ()
{
	static GType xa_main_window_menu_bar_type = 0;

 	if (!xa_main_window_menu_bar_type)
	{
 		static const GTypeInfo xa_main_window_menu_bar_info = 
		{
			sizeof (XAMainWindowMenuBarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_main_window_menu_bar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAMainWindowMenuBar),
			0,
			(GInstanceInitFunc) xa_main_window_menu_bar_init,
			NULL
		};

		xa_main_window_menu_bar_type = g_type_register_static (GTK_TYPE_MENU_BAR, "XAMainWindowMenuBar", &xa_main_window_menu_bar_info, 0);
	}
	return xa_main_window_menu_bar_type;
}

static void
xa_main_window_menu_bar_class_init(XAMainWindowMenuBarClass *menubar_class)
{
}

static void
xa_main_window_menu_bar_init(XAMainWindowMenuBar *menubar)
{
	GtkAccelGroup *accel_group = gtk_accel_group_new();

	GtkWidget *tmp_image;

	menubar->menu_item_archive = gtk_menu_item_new_with_mnemonic(_("_Archive"));
	menubar->menu_item_action = gtk_menu_item_new_with_mnemonic(_("A_ction"));
	menubar->menu_item_help = gtk_menu_item_new_with_mnemonic(_("_Help"));

	gtk_container_add(GTK_CONTAINER(menubar), menubar->menu_item_archive);
	gtk_container_add(GTK_CONTAINER(menubar), menubar->menu_item_action);
	gtk_container_add(GTK_CONTAINER(menubar), menubar->menu_item_help);

	menubar->menu_archive = gtk_menu_new();
	menubar->menu_action = gtk_menu_new();
	menubar->menu_help = gtk_menu_new();

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menubar->menu_item_archive), menubar->menu_archive);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menubar->menu_item_action), menubar->menu_action);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menubar->menu_item_help), menubar->menu_help);

/* Archive menu */
	menubar->menu_item_new = gtk_image_menu_item_new_from_stock("gtk-new", accel_group);
	menubar->menu_item_open = gtk_image_menu_item_new_from_stock("gtk-open", accel_group);
	menubar->menu_item_quit = gtk_image_menu_item_new_from_stock("gtk-quit", accel_group);

	gtk_container_add(GTK_CONTAINER(menubar->menu_archive), menubar->menu_item_new);
	gtk_container_add(GTK_CONTAINER(menubar->menu_archive), menubar->menu_item_open);
	gtk_container_add(GTK_CONTAINER(menubar->menu_archive), menubar->menu_item_quit);

/* Action menu */
	tmp_image = xa_main_window_find_image("add_button.png", GTK_ICON_SIZE_MENU);
	menubar->menu_item_add = gtk_image_menu_item_new_with_mnemonic(_("Add"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menubar->menu_item_add), tmp_image);

	tmp_image = xa_main_window_find_image("extract_button.png", GTK_ICON_SIZE_MENU);
	menubar->menu_item_extract = gtk_image_menu_item_new_with_mnemonic(_("Extract"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menubar->menu_item_extract), tmp_image);

	menubar->menu_item_remove = gtk_image_menu_item_new_from_stock("gtk-delete", accel_group);

	gtk_container_add(GTK_CONTAINER(menubar->menu_action), menubar->menu_item_add);
	gtk_container_add(GTK_CONTAINER(menubar->menu_action), menubar->menu_item_extract);
	gtk_container_add(GTK_CONTAINER(menubar->menu_action), menubar->menu_item_remove);

/* Help menu */
	menubar->menu_item_about = gtk_image_menu_item_new_from_stock("gtk-about", accel_group);
	gtk_container_add(GTK_CONTAINER(menubar->menu_help), menubar->menu_item_about);
}

GtkWidget *
xa_main_window_menu_bar_new()
{
	GtkWidget *menubar;

	menubar = g_object_new(xa_main_window_menu_bar_get_type(), NULL);

	return menubar;
}
