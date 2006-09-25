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
		FIXME: File is full of hacks.
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>
#include "main_window.h"
#include "new_dialog.h"
#include "extract_dialog.h"
#include "add_dialog.h"
#include "preferences_dialog.h"

#include "main.h"

enum {
	XA_MAIN_WINDOW_SHOW_ICONS = 1
};

static void
xa_main_window_init(XAMainWindow *);
static void
xa_main_window_class_init(XAMainWindowClass *);
static void
xa_main_window_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
xa_main_window_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

void
cb_xa_main_item_activated(GtkTreeView *treeview, GtkTreePath *treepath, GtkTreeViewColumn *column, gpointer userdata);

void
xa_main_window_archive_destroyed(LXAArchive *archive, XAMainWindow *window);

void 
xa_main_window_set_contents(XAMainWindow *, LXAArchive *, GSList *);

void
cb_xa_main_close_archive(GtkWidget *widget, gpointer userdata);

void
cb_xa_main_settings_archive(GtkWidget *widget, gpointer userdata);

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
	GObjectClass *object_class = G_OBJECT_CLASS (window_class);
	GParamSpec *pspec = NULL;

	object_class->set_property = xa_main_window_set_property;
	object_class->get_property = xa_main_window_get_property;

	pspec = g_param_spec_boolean("show_icons",
		_("Show icons"),
		_("Show icons"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_MAIN_WINDOW_SHOW_ICONS, pspec);
}

static void
xa_main_window_init(XAMainWindow *window)
{
	GtkWidget     *main_vbox;
	GtkWidget     *menubar, *toolbar;
	GtkWidget     *menu_separator;
	GtkToolItem   *tool_separator;
	GtkWidget     *tmp_image;
	GtkAccelGroup *accel_group = gtk_accel_group_new();

	window->working_node = NULL;
	window->parent_node = g_new0(GValue, 1);
	window->parent_node = g_value_init(window->parent_node, G_TYPE_STRING);
	g_value_set_string(window->parent_node, "..");

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

	window->menubar.menu_item_close = gtk_image_menu_item_new_from_stock("gtk-close", accel_group);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_archive), window->menubar.menu_item_close);

	menu_separator = gtk_separator_menu_item_new();
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_archive), menu_separator);

	window->menubar.menu_item_quit = gtk_image_menu_item_new_from_stock("gtk-quit", accel_group);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_archive), window->menubar.menu_item_quit);

	g_signal_connect(G_OBJECT(window->menubar.menu_item_new), "activate", G_CALLBACK(cb_xa_main_new_archive), window);
	g_signal_connect(G_OBJECT(window->menubar.menu_item_open), "activate", G_CALLBACK(cb_xa_main_open_archive), window);
	g_signal_connect(G_OBJECT(window->menubar.menu_item_close), "activate", G_CALLBACK(cb_xa_main_close_archive), window);
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

	g_signal_connect(G_OBJECT(window->menubar.menu_item_add), "activate", G_CALLBACK(cb_xa_main_add_to_archive), window);
	g_signal_connect(G_OBJECT(window->menubar.menu_item_extract), "activate", G_CALLBACK(cb_xa_main_extract_archive), window);
	g_signal_connect(G_OBJECT(window->menubar.menu_item_settings), "activate", G_CALLBACK(cb_xa_main_settings_archive), window);

	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_extract), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_remove), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_close), FALSE);

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

	g_signal_connect(G_OBJECT(window->toolbar.tool_item_add), "clicked", G_CALLBACK(cb_xa_main_add_to_archive), window);
	g_signal_connect(G_OBJECT(window->toolbar.tool_item_extract), "clicked", G_CALLBACK(cb_xa_main_extract_archive), window);

/* control pane */

	window->toolbar.tool_item_stop = gtk_tool_button_new_from_stock("gtk-stop");
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_stop));

	g_signal_connect(G_OBJECT(window->toolbar.tool_item_stop), "clicked", G_CALLBACK(cb_xa_main_stop_archive), window);

/* main view */
	window->scrollwindow = gtk_scrolled_window_new(NULL, NULL);

	window->treeview = gtk_tree_view_new();

	gtk_container_add(GTK_CONTAINER(window->scrollwindow), window->treeview);

	gtk_widget_show(window->scrollwindow);
	gtk_widget_show(window->treeview);
	g_signal_connect(G_OBJECT(window->treeview), "row-activated", G_CALLBACK(cb_xa_main_item_activated), window);
/* Statusbar */

	window->statusbar = gtk_statusbar_new();

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), window->scrollwindow, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(main_vbox), window->statusbar, FALSE, FALSE, 0);

	gtk_widget_show(main_vbox);
	gtk_widget_show_all(menubar);
	gtk_widget_show_all(toolbar);
	gtk_widget_show_all(window->statusbar);

	gtk_container_add(GTK_CONTAINER(window), main_vbox);
}

GtkWidget *
xa_main_window_new(GtkIconTheme *icon_theme)
{
	GtkWidget *window;
	GdkPixbuf *icon = gtk_icon_theme_load_icon(icon_theme, "xarchiver", 24, 0, NULL);

	window = g_object_new(xa_main_window_get_type(),
			"title", "Xarchiver " PACKAGE_VERSION,
			NULL);
	
	gtk_window_set_icon(GTK_WINDOW(window), icon);
	

	return window;
}

static void
xa_main_window_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_MAIN_WINDOW_SHOW_ICONS:
			g_value_set_boolean(value, XA_MAIN_WINDOW(object)->props._show_icons);
			break;
	}
}

static void
xa_main_window_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_MAIN_WINDOW_SHOW_ICONS:
			XA_MAIN_WINDOW(object)->props._show_icons = g_value_get_boolean(value);
			break;
	}
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
	XAMainWindow *window = XA_MAIN_WINDOW(userdata);
	LXAArchiveSupport *lp_support = NULL;
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
		if(window->lp_xa_archive)
		{
			g_object_unref(window->lp_xa_archive);
			window->lp_xa_archive = NULL;
		}
		if(!lxa_new_archive(new_archive_path, TRUE, NULL, &(window->lp_xa_archive)))
		{
			g_debug("Archive opened");
			g_signal_connect(G_OBJECT(window->lp_xa_archive), "lxa_status_changed", G_CALLBACK(xa_main_window_archive_status_changed), window);
			g_slist_free(window->working_node);
			window->working_node = NULL;
			lp_support = lxa_get_support_for_mime(window->lp_xa_archive->mime);

			gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_close), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_new), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_open), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_add), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_remove), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_extract), FALSE);

			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_new), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_open), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);
		}
		else
			g_debug("Archive-creation failed");
		gtk_widget_destroy (GTK_WIDGET (dialog) );
	}
}

void
cb_xa_main_close_archive(GtkWidget *widget, gpointer userdata)
{
	g_object_unref(XA_MAIN_WINDOW(userdata)->lp_xa_archive);
}

void
cb_xa_main_settings_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = xa_preferences_dialog_new();
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void
cb_xa_main_open_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	gchar *open_archive_path = NULL;
	gint result = 0;
	XAMainWindow *window = XA_MAIN_WINDOW(userdata);
	
	dialog = gtk_file_chooser_dialog_new(_("Open archive"), 
																			 GTK_WINDOW(window),
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
		if(window->lp_xa_archive)
		{
			g_object_unref(window->lp_xa_archive);
			window->lp_xa_archive = NULL;
		}

		xa_main_window_open_archive(window, open_archive_path);



		gtk_widget_destroy (GTK_WIDGET (dialog) );
	}

}

void
cb_xa_main_add_to_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	gint result = 0;
	XAMainWindow *window = XA_MAIN_WINDOW(userdata);
	LXAArchiveSupport *lp_support = NULL;

	lp_support = lxa_get_support_for_mime(window->lp_xa_archive->mime);

	dialog = xa_add_dialog_new(lp_support, window->lp_xa_archive, FALSE);
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(GTK_WIDGET(dialog));
	}
	gtk_widget_destroy (GTK_WIDGET (dialog) );

}

void
cb_xa_main_extract_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	gchar *extract_archive_path = NULL;
	gint result = 0;
	XAMainWindow *window = XA_MAIN_WINDOW(userdata);
	LXAArchiveSupport *lp_support = NULL;

	lp_support = lxa_get_support_for_mime(window->lp_xa_archive->mime);

	dialog = xa_extract_archive_dialog_new(lp_support, window->lp_xa_archive, FALSE);
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(GTK_WIDGET(dialog));
		extract_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		lxa_archive_support_extract(lp_support, window->lp_xa_archive, extract_archive_path, NULL);
		g_free(extract_archive_path);
		extract_archive_path = NULL;
	}
	gtk_widget_destroy (GTK_WIDGET (dialog) );
}

void
cb_xa_main_stop_archive(GtkWidget *widget, gpointer userdata)
{
	XAMainWindow *window = XA_MAIN_WINDOW(userdata);
	
	lxa_stop_archive_child(window->lp_xa_archive);
}

void
xa_main_window_archive_destroyed(LXAArchive *archive, XAMainWindow *window)
{
	GtkTreeModel *liststore = gtk_tree_view_get_model(GTK_TREE_VIEW(window->treeview));
	GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(window->treeview));

	gtk_list_store_clear(GTK_LIST_STORE(liststore));

	while(columns)
	{
		gtk_tree_view_remove_column(GTK_TREE_VIEW(window->treeview), columns->data);
		columns = columns->next;
	}
	g_list_free(columns);

	if(archive == window->lp_xa_archive)
		window->lp_xa_archive = NULL;
	
	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_close), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_new), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_open), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_remove), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_extract), FALSE);

	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_new), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_open), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);
	g_debug("AAA");
}

gint
xa_main_window_open_archive(XAMainWindow *window, gchar *archive_path)
{
	LXAArchiveSupport *lp_support = NULL;

	if(!lxa_open_archive(archive_path, &window->lp_xa_archive))
	{
		g_signal_connect(G_OBJECT(window->lp_xa_archive), "lxa_status_changed", G_CALLBACK(xa_main_window_archive_status_changed), window);
		g_slist_free(window->working_node);
		window->working_node = NULL;
		lp_support = lxa_get_support_for_mime(window->lp_xa_archive->mime);

		lxa_archive_support_refresh(lp_support, window->lp_xa_archive);
		return 0;
	}

	return 1;
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
	gchar *status_message = NULL; 
	switch(archive->status)
	{
		case(LXA_ARCHIVESTATUS_INIT):
			status_message = g_strdup(_("Initializing archive..."));
			break;
		case(LXA_ARCHIVESTATUS_REFRESH):
			status_message = g_strdup( _("Reading archive contents..."));
			break;
		case(LXA_ARCHIVESTATUS_EXTRACT):
			status_message = g_strdup(_("Extracting archive..."));
			break;
		case(LXA_ARCHIVESTATUS_ADD):
			status_message = g_strdup(_("Adding file(s) to archive..."));
			break;
		case(LXA_ARCHIVESTATUS_REMOVE):
			status_message = g_strdup(_("Removing file(s) from archive..."));
			break;
		case(LXA_ARCHIVESTATUS_ERROR):
			status_message = g_strdup(_("Error"));
			break;
		case(LXA_ARCHIVESTATUS_USERBREAK):
			status_message = g_strdup(_("Cancelled"));
			break;
		case(LXA_ARCHIVESTATUS_IDLE):
			status_message = g_strdup(_("Done"));
			gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_close), TRUE);
			if(archive->old_status == LXA_ARCHIVESTATUS_REFRESH)
			{
				gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_add), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_remove), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_extract), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_add), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_remove), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_extract), TRUE);

				GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(main_window->treeview));
				while(columns)
				{
					gtk_tree_view_remove_column(GTK_TREE_VIEW(main_window->treeview), columns->data);
					columns = columns->next;
				}
				g_list_free(columns);
				liststore = gtk_list_store_newv(archive->column_number, archive->column_types); 
	
				for(x = 0; x < archive->column_number; x++)
				{
					switch(archive->column_types[x])
					{
						case(G_TYPE_STRING):
						case(G_TYPE_UINT64):
							renderer = gtk_cell_renderer_text_new();
							column = gtk_tree_view_column_new_with_attributes(archive->column_names[x], renderer, "text", x, NULL);
							break;
					}
					gtk_tree_view_column_set_resizable(column, TRUE);
					gtk_tree_view_column_set_sort_column_id(column, x);
					gtk_tree_view_append_column(GTK_TREE_VIEW(main_window->treeview), column);
				}
				gtk_tree_view_set_model(GTK_TREE_VIEW(main_window->treeview), GTK_TREE_MODEL(liststore));
				main_window->working_node = g_slist_prepend(main_window->working_node, &(archive->root_entry));
				xa_main_window_set_contents(main_window, archive, archive->root_entry.children);
			}
			break;
	}
	if((archive->status != LXA_ARCHIVESTATUS_IDLE) && 
		(archive->status != LXA_ARCHIVESTATUS_ERROR) && 
		(archive->status != LXA_ARCHIVESTATUS_USERBREAK))

	{
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_new), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_open), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_add), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_remove), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_extract), FALSE);

		gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_new), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_open), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_add), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_remove), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_extract), FALSE);

		gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_stop), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_new), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->menubar.menu_item_open), TRUE);

		gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_new), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_open), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(main_window->toolbar.tool_item_stop), FALSE);
	}
	gtk_statusbar_push(GTK_STATUSBAR(main_window->statusbar), 1, status_message);
}


void 
xa_main_window_set_contents(XAMainWindow *main_window, LXAArchive *archive, GSList *items)
{
	gint i = 0;
	GtkTreeIter iter;
	GtkTreeModel *liststore = gtk_tree_view_get_model(GTK_TREE_VIEW(main_window->treeview));
	GValue *tmp_value;
	gpointer props;
	gpointer props_iter;

	gtk_list_store_clear(GTK_LIST_STORE(liststore));

	while(items)
	{
		gtk_list_store_append(GTK_LIST_STORE(liststore), &iter);
		tmp_value = g_new0(GValue, 1);
		tmp_value = g_value_init(tmp_value, G_TYPE_STRING);
		g_value_set_string(tmp_value, ((LXAEntry *)items->data)->filename);
		gtk_list_store_set_value(GTK_LIST_STORE(liststore), &iter, 0, tmp_value);

		props = ((LXAEntry *)items->data)->props;
		if(props)
		{
			props_iter = props;
			for(i = 0; i < archive->column_number-1; i++)
			{
				tmp_value = g_new0(GValue, 1);
				tmp_value = g_value_init(tmp_value, archive->column_types[i]);
				switch(archive->column_types[i])
				{
					case(G_TYPE_UINT):
						g_value_set_uint(tmp_value, *(guint *)props_iter);
						props_iter += sizeof(guint);
						break;
					case(G_TYPE_STRING):
						g_value_set_string(tmp_value, *(gchar **)props_iter);
						props_iter += sizeof(gchar *);
						break;
					case(G_TYPE_UINT64):
						g_value_set_uint64(tmp_value, *(guint64 *)props_iter);
						props_iter += sizeof(guint64);
						break;
				}
				gtk_list_store_set_value(GTK_LIST_STORE(liststore), &iter, i+1, tmp_value);
				g_free(tmp_value);
			}
		}
		items = items->next;
	}
	if(g_slist_length(main_window->working_node) > 1)
	{
		gtk_list_store_prepend(GTK_LIST_STORE(liststore), &iter);
		gtk_list_store_set_value(GTK_LIST_STORE(liststore), &iter, 0, main_window->parent_node);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(main_window->treeview), GTK_TREE_MODEL(liststore));
}

void
cb_xa_main_item_activated(GtkTreeView *treeview, GtkTreePath *treepath, GtkTreeViewColumn *column, gpointer userdata)
{
	GtkTreeIter iter;
	GSList *items = NULL;
	GValue *value = g_new0(GValue, 1);
	XAMainWindow *main_window = userdata;

	GtkTreeModel *tree_model = gtk_tree_view_get_model(treeview);

	gtk_tree_model_get_iter(tree_model, &iter, treepath);
	gtk_tree_model_get_value(tree_model, &iter, 0, value);

	const gchar *item_filename = g_value_get_string(value);
	if(!strcmp(item_filename, "..")) /* pop */
	{
		main_window->working_node = g_slist_delete_link(main_window->working_node, main_window->working_node);
	}
	else
	{
		LXAEntry *entry = lxa_entry_get_child(((LXAEntry *)main_window->working_node->data), item_filename);
		if(g_slist_length(entry->children))
		{
			main_window->working_node = g_slist_prepend(main_window->working_node, entry);
		}
		else
		{ }
	}
	items = ((LXAEntry *)main_window->working_node->data)->children;

	if(items)
		xa_main_window_set_contents(main_window, main_window->lp_xa_archive, items);
/*	else*/
		/* 'view' */

	g_value_reset(value);
	g_free(value);
}

