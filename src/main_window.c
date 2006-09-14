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
#include "main_window.h"
#include "new_dialog.h"

#include "main.h"

static void
xa_main_window_class_init(XAMainWindowClass *);

static void
xa_main_window_init(XAMainWindow *);

void
cb_xa_main_item_activated(GtkTreeView *treeview, GtkTreePath *treepath, GtkTreeViewColumn *column, gpointer userdata);

GType
xa_main_window_get_type ()
{
	static GType xa_main_window_type = 0;

	if (!xa_main_window_type)
	{
		static const GTypeInfo xa_main_window_info = 
		{
			sizeof (XAMainWindowClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_main_window_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAMainWindow),
			0,
			(GInstanceInitFunc) xa_main_window_init,
			NULL
		};

		xa_main_window_type = g_type_register_static (GTK_TYPE_WINDOW, "XAMainWindow", &xa_main_window_info, 0);
	}
	return xa_main_window_type;
}

static void
xa_main_window_class_init(XAMainWindowClass *window_class)
{
}

static void
xa_main_window_init(XAMainWindow *window)
{
	GtkWidget     *main_vbox;
	GtkWidget     *menubar, *toolbar, *statusbar;
	GtkWidget     *menu_separator;
	GtkToolItem   *tool_separator;
	GtkWidget     *tmp_image;
	GtkAccelGroup *accel_group = gtk_accel_group_new();

	window->working_dir = NULL;

	gtk_window_set_default_icon_from_file(DATADIR "/pixmaps/xarchiver.png", NULL);
	main_vbox = gtk_vbox_new(FALSE, 0);

	menubar = gtk_menu_bar_new();
	toolbar = gtk_toolbar_new();

	window->menubar.menu_item_archive = gtk_menu_item_new_with_mnemonic(_("_Archive"));
	window->menubar.menu_item_action = gtk_menu_item_new_with_mnemonic(_("A_ction"));
	window->menubar.menu_item_help = gtk_menu_item_new_with_mnemonic(_("_Help"));

	gtk_container_add(GTK_CONTAINER(menubar), window->menubar.menu_item_archive);
	gtk_container_add(GTK_CONTAINER(menubar), window->menubar.menu_item_action);
	gtk_container_add(GTK_CONTAINER(menubar), window->menubar.menu_item_help);

	window->menubar.menu_archive = gtk_menu_new();
	window->menubar.menu_action = gtk_menu_new();
	window->menubar.menu_help = gtk_menu_new();

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(window->menubar.menu_item_archive), window->menubar.menu_archive);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(window->menubar.menu_item_action), window->menubar.menu_action);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(window->menubar.menu_item_help), window->menubar.menu_help);

/* Archive menu */
	window->menubar.menu_item_new = gtk_image_menu_item_new_from_stock("gtk-new", accel_group);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_archive), window->menubar.menu_item_new);

	window->menubar.menu_item_open = gtk_image_menu_item_new_from_stock("gtk-open", accel_group);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_archive), window->menubar.menu_item_open);

	menu_separator = gtk_separator_menu_item_new();
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_archive), menu_separator);

	window->menubar.menu_item_properties = gtk_image_menu_item_new_from_stock("gtk-properties", accel_group);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_archive), window->menubar.menu_item_properties);

	menu_separator = gtk_separator_menu_item_new();
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_archive), menu_separator);

	window->menubar.menu_item_quit = gtk_image_menu_item_new_from_stock("gtk-quit", accel_group);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_archive), window->menubar.menu_item_quit);

	g_signal_connect(G_OBJECT(window->menubar.menu_item_new), "activate", G_CALLBACK(cb_xa_main_new_archive), window);
	g_signal_connect(G_OBJECT(window->menubar.menu_item_open), "activate", G_CALLBACK(cb_xa_main_open_archive), window);
	/* g_signal_connect(G_OBJECT(window->menubar.menu_item_properties), "activate", NULL, NULL);*/
	g_signal_connect(G_OBJECT(window->menubar.menu_item_quit), "activate", G_CALLBACK(gtk_main_quit), window);

/* Action menu */
	tmp_image = xa_main_window_find_image("add_button.png", GTK_ICON_SIZE_MENU);
	window->menubar.menu_item_add = gtk_image_menu_item_new_with_mnemonic(_("Add"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (window->menubar.menu_item_add), tmp_image);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_add);

	tmp_image = xa_main_window_find_image("extract_button.png", GTK_ICON_SIZE_MENU);
	window->menubar.menu_item_extract = gtk_image_menu_item_new_with_mnemonic(_("Extract"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (window->menubar.menu_item_extract), tmp_image);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_extract);

	window->menubar.menu_item_remove = gtk_image_menu_item_new_from_stock("gtk-delete", accel_group);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_remove);

	menu_separator = gtk_separator_menu_item_new();
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), menu_separator);

	window->menubar.menu_item_settings = gtk_image_menu_item_new_from_stock("gtk-preferences", accel_group);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_settings);

/* Help menu */
	window->menubar.menu_item_about = gtk_image_menu_item_new_from_stock("gtk-about", accel_group);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_help), window->menubar.menu_item_about);

/* Archive pane */
	window->toolbar.tool_item_new = gtk_tool_button_new_from_stock("gtk-new");
	window->toolbar.tool_item_open = gtk_tool_button_new_from_stock("gtk-open");
	tool_separator = gtk_separator_tool_item_new ();

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_new));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_open));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(tool_separator));

	g_signal_connect(G_OBJECT(window->toolbar.tool_item_new), "clicked", G_CALLBACK(cb_xa_main_new_archive), window);
	g_signal_connect(G_OBJECT(window->toolbar.tool_item_open), "clicked", G_CALLBACK(cb_xa_main_open_archive), window);

/* Action pane */
	tmp_image = xa_main_window_find_image("add.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
	window->toolbar.tool_item_add = gtk_tool_button_new(tmp_image, _("Add"));
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), FALSE);

	tmp_image = xa_main_window_find_image("extract.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
	window->toolbar.tool_item_extract = gtk_tool_button_new(tmp_image, _("Extract"));
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);

	window->toolbar.tool_item_remove = gtk_tool_button_new_from_stock("gtk-delete");
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);

	tool_separator = gtk_separator_tool_item_new ();

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_add));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_extract));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_remove));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(tool_separator));

/* main view */
	window->scrollwindow = gtk_scrolled_window_new(NULL, NULL);

	window->treeview = gtk_tree_view_new();

	gtk_container_add(GTK_CONTAINER(window->scrollwindow), window->treeview);

	gtk_widget_show(window->scrollwindow);
	gtk_widget_show(window->treeview);
	g_signal_connect(G_OBJECT(window->treeview), "row-activated", G_CALLBACK(cb_xa_main_item_activated), window);
/* Statusbar */

	statusbar = gtk_statusbar_new();

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), window->scrollwindow, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(main_vbox), statusbar, FALSE, FALSE, 0);

	gtk_widget_show(main_vbox);
	gtk_widget_show_all(menubar);
	gtk_widget_show_all(toolbar);
	gtk_widget_show_all(statusbar);

	gtk_container_add(GTK_CONTAINER(window), main_vbox);
}

GtkWidget *
xa_main_window_new()
{
	GtkWidget *window;

	window = g_object_new(xa_main_window_get_type(),
			"title", "Xarchiver " PACKAGE_VERSION,
			NULL);

	return window;
}

GtkWidget *
xa_main_window_find_image(gchar *filename, GtkIconSize size)
{
	GError *error = NULL;
	GtkWidget *file_image;
	gchar *path;
	path = g_strconcat(DATADIR, "/xarchiver/pixmaps/", filename, NULL);
	GdkPixbuf *file_pixbuf = gdk_pixbuf_new_from_file(path, &error);
	if(error)
	{
		/*
		* perhaps xarchiver has not been installed and is being executed from source dir
		*/
		g_free(error);
		error = NULL;
		path = g_strconcat("./pixmaps/", filename, NULL);
		file_pixbuf = gdk_pixbuf_new_from_file(path, &error);
	}
	if(file_pixbuf)
	{
		file_image = gtk_image_new_from_pixbuf(file_pixbuf);
		g_object_unref(file_pixbuf);
	}
	else
	{
		g_free(error);
		file_image = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, size);
	}
	g_free(path);
	return file_image;
}

void
cb_xa_main_new_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	gchar *new_archive_path = NULL;
	gint result = 0;
	
	dialog = xa_new_archive_dialog_new();
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog) );
		return;
	}
	if(result == GTK_RESPONSE_OK)
	{
		new_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if(lp_xa_archive)
		{
			g_object_unref(lp_xa_archive);
			lp_xa_archive = NULL;
		}
		if(!lxa_new_archive(new_archive_path, TRUE, NULL, &lp_xa_archive))
		{
			g_debug("Archive opened");
		}
		gtk_widget_destroy (GTK_WIDGET (dialog) );
	}
}

void
cb_xa_main_open_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	gchar *open_archive_path = NULL;
	gint result = 0;
	LXAArchiveSupport *lpSupport;
	XAMainWindow *parent_window = XA_MAIN_WINDOW(userdata);
	
	dialog = gtk_file_chooser_dialog_new(_("Open archive"), 
																			 GTK_WINDOW(parent_window),
																			 GTK_FILE_CHOOSER_ACTION_OPEN,
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
																			 GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog) );
		return;
	}
	if(result == GTK_RESPONSE_OK)
	{
		open_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if(lp_xa_archive)
		{
			g_object_unref(lp_xa_archive);
			lp_xa_archive = NULL;
		}
		if(!lxa_open_archive(open_archive_path, &lp_xa_archive))
		{
			g_debug("Archive opened");
			gtk_widget_set_sensitive(GTK_WIDGET(parent_window->toolbar.tool_item_add), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(parent_window->toolbar.tool_item_remove), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(parent_window->toolbar.tool_item_extract), TRUE);
			g_signal_connect(G_OBJECT(lp_xa_archive), "lxa_status_changed", G_CALLBACK(xa_main_window_archive_status_changed), parent_window);
			lpSupport = lxa_get_support_for_mime(lp_xa_archive->mime);
			lxa_archive_support_refresh(lpSupport, lp_xa_archive);
		}

		gtk_widget_destroy (GTK_WIDGET (dialog) );
	}

}

/*
 *
 *
 */
void
xa_main_window_archive_status_changed(LXAArchive *archive, gpointer userdata)
{
	GtkCellRenderer *renderer = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkListStore *liststore = NULL;
	gint x = 0;
	XAMainWindow *main_window = XA_MAIN_WINDOW(userdata);
	if(archive->old_status == LXA_ARCHIVESTATUS_REFRESH)
	{
		liststore = gtk_list_store_newv(archive->column_number, archive->column_types); 
		for(x = 0; x < archive->column_number; x++)
		{
			switch(archive->column_types[x])
			{
				case(G_TYPE_STRING):
					renderer = gtk_cell_renderer_text_new();
					column = gtk_tree_view_column_new_with_attributes(archive->column_names[x], renderer, "text", x, NULL);
					break;
			}
			gtk_tree_view_column_set_resizable(column, TRUE);
			gtk_tree_view_column_set_sort_column_id(column, x);
			gtk_tree_view_append_column(GTK_TREE_VIEW(main_window->treeview), column);
		}
		gtk_tree_view_set_model(GTK_TREE_VIEW(main_window->treeview), GTK_TREE_MODEL(liststore));
		GSList *items = lxa_archive_get_children(archive, "");
		xa_main_window_set_contents(main_window, archive, items, "");
	}
}

void
cb_xa_main_extract_archive(GtkWidget *widget, gpointer userdata)
{

}

void 
xa_main_window_set_contents(XAMainWindow *main_window, LXAArchive *archive, GSList *items, gchar *path)
{
	GtkTreeIter iter;
	GtkTreeModel *liststore = gtk_tree_view_get_model(GTK_TREE_VIEW(main_window->treeview));

	gtk_list_store_clear(GTK_LIST_STORE(liststore));

	main_window->working_dir = path;

	while(items)
	{
		gtk_list_store_append(GTK_LIST_STORE(liststore), &iter);
		gtk_list_store_set_value(GTK_LIST_STORE(liststore), &iter, 0, ((LXAEntry *)items->data)->filename);
		items = items->next;
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(main_window->treeview), GTK_TREE_MODEL(liststore));
}

void
cb_xa_main_item_activated(GtkTreeView *treeview, GtkTreePath *treepath, GtkTreeViewColumn *column, gpointer userdata)
{
	GtkTreeIter iter;
	GValue *value = g_new0(GValue, 1);
	XAMainWindow *main_window = userdata;

	GtkTreeModel *tree_model = gtk_tree_view_get_model(treeview);

	gtk_tree_model_get_iter(tree_model, &iter, treepath);
	gtk_tree_model_get_value(tree_model, &iter, 0, value);

	gchar *path = g_strconcat(main_window->working_dir, "/", g_value_get_string(value), NULL);

	g_debug("%s\n", path);

	GSList *items = lxa_archive_get_children(lp_xa_archive, path);
	if(items)
		xa_main_window_set_contents(main_window, lp_xa_archive, items, path);
/*	else*/
		/* 'view' */

g_free(value);

}
