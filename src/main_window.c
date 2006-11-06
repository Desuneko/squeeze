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
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>
#include <libxarchiver/mime.h>
#include <gettext.h>

#include "settings.h"
#include "archive_store.h"
#include "navigation_bar.h"

#ifdef ENABLE_PATHBAR
#include "path_bar.h"
#endif /* ENABLE_PATHBAR */

#ifdef ENABLE_TOOLBAR
#include "tool_bar.h"
#endif /* ENABLE_TOOLBAR */

#include "notebook.h"
#include "main_window.h"

#include "new_dialog.h"
#include "extract_dialog.h"
#include "add_dialog.h"
#include "preferences_dialog.h"

#include "main.h"

static void
xa_main_window_init(XAMainWindow *);
static void
xa_main_window_class_init(XAMainWindowClass *);
static void
xa_main_window_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
xa_main_window_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void
xa_main_window_finalize(GObject *object);


static void cb_xa_main_new_archive(GtkWidget *widget, gpointer userdata);
static void cb_xa_main_open_archive(GtkWidget *widget, gpointer userdata);
static void cb_xa_main_extract_archive(GtkWidget *widget, gpointer userdata);
static void cb_xa_main_add_to_archive(GtkWidget *widget, gpointer userdata);
static void cb_xa_main_stop_archive(GtkWidget *widget, gpointer userdata);

static void
cb_xa_main_window_notebook_page_switched(XANotebook *notebook, GtkNotebookPage *page, guint page_nr, gpointer data);
static void
cb_xa_main_window_notebook_page_removed(XANotebook *notebook, gpointer data);


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

	object_class->set_property = xa_main_window_set_property;
	object_class->get_property = xa_main_window_get_property;
	object_class->finalize     = xa_main_window_finalize;

}

static void
xa_main_window_finalize(GObject *object)
{
	XAMainWindow *window = XA_MAIN_WINDOW(object);


	xa_settings_set_group(window->settings, "Global");
	if(!window->navigationbar)
	{
		xa_settings_write_entry(window->settings, "NavigationBar", "None");
	}
#ifdef ENABLE_TOOLBAR
	else if(XA_IS_TOOL_BAR(window->navigationbar))
	{
		xa_settings_write_entry(window->settings, "NavigationBar", "ToolBar");
	}	
#endif
#ifdef ENABLE_PATHBAR
	else if(XA_IS_PATH_BAR(window->navigationbar))
	{
		xa_settings_write_entry(window->settings, "NavigationBar", "PathBar");
	}
#endif
	else
	{
		xa_settings_write_entry(window->settings, "NavigationBar", "None");
	}

	xa_settings_save(window->settings);

	gtk_widget_unref(GTK_WIDGET(window->navigationbar));
}

static void
xa_main_window_init(XAMainWindow *window)
{
	GtkWidget     *main_vbox;
	GtkWidget     *toolbar;
	GtkToolItem   *tool_separator;
	GtkWidget     *tmp_image;
	const gchar   *nav_bar;
	gboolean up_dir = TRUE;
	gboolean show_icons = TRUE;
	gboolean sort_case = TRUE;
	gboolean sort_folders = TRUE;

	window->settings = xa_settings_new(NULL);

	main_vbox = gtk_vbox_new(FALSE, 0);

	toolbar = gtk_toolbar_new();

/* Archive pane */
	window->toolbar.tool_item_new = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	window->toolbar.tool_item_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
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

	window->toolbar.tool_item_remove = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);

	tool_separator = gtk_separator_tool_item_new ();

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_add));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_extract));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_remove));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(tool_separator));

	g_signal_connect(G_OBJECT(window->toolbar.tool_item_add), "clicked", G_CALLBACK(cb_xa_main_add_to_archive), window);
	g_signal_connect(G_OBJECT(window->toolbar.tool_item_extract), "clicked", G_CALLBACK(cb_xa_main_extract_archive), window);

/* control pane */

	window->toolbar.tool_item_stop = gtk_tool_button_new_from_stock(GTK_STOCK_STOP);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_stop));

	g_signal_connect(G_OBJECT(window->toolbar.tool_item_stop), "clicked", G_CALLBACK(cb_xa_main_stop_archive), window);

	xa_settings_set_group(window->settings, "Global");
	nav_bar = xa_settings_read_entry(window->settings, "NavigationBar", "None");
	window->navigationbar = NULL;

#ifdef ENABLE_TOOLBAR
	if(!strcmp(nav_bar, "ToolBar"))
	{
		window->navigationbar = xa_tool_bar_new(NULL); 
		up_dir = FALSE;
	}
#endif
#ifdef ENABLE_PATHBAR
	if(!strcmp(nav_bar, "PathBar"))
	{
		window->navigationbar = xa_path_bar_new(NULL);
		gtk_container_set_border_width(GTK_CONTAINER(window->navigationbar), 3);
		up_dir = FALSE;
	}
#endif

	if(!window->navigationbar)
	{
		window->navigationbar = xa_navigation_bar_new(NULL);
	}

	show_icons = xa_settings_read_bool_entry(window->settings, "ShowIcons", TRUE);
	sort_case = xa_settings_read_bool_entry(window->settings, "SortCaseSensitive", TRUE);
	sort_folders = xa_settings_read_bool_entry(window->settings, "SortFoldersFirst", TRUE);

	gtk_widget_ref(GTK_WIDGET(window->navigationbar));

/* main view */
	window->notebook = xa_notebook_new(window->navigationbar);
	g_signal_connect(G_OBJECT(window->notebook), "switch-page", G_CALLBACK(cb_xa_main_window_notebook_page_switched), window);
	g_signal_connect(G_OBJECT(window->notebook), "archive-removed", G_CALLBACK(cb_xa_main_window_notebook_page_removed), window);
/* Statusbar */

	window->statusbar = gtk_statusbar_new();

	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);

	if(window->navigationbar)
		gtk_box_pack_start(GTK_BOX(main_vbox), GTK_WIDGET(window->navigationbar), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(main_vbox), window->notebook, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(main_vbox), window->statusbar, FALSE, FALSE, 0);

	gtk_widget_show(main_vbox);
	gtk_widget_show_all(toolbar);
	gtk_widget_show_all(window->notebook);
	gtk_widget_show_all(window->statusbar);

	gtk_container_add(GTK_CONTAINER(window), main_vbox);
}

GtkWidget *
xa_main_window_new(GtkIconTheme *icon_theme)
{
	GtkWidget *window;
	GdkPixbuf *icon;

	window = g_object_new(xa_main_window_get_type(),
			"title", "Xarchiver " PACKAGE_VERSION,
			NULL);

	XA_MAIN_WINDOW(window)->icon_theme = icon_theme;

	xa_notebook_set_icon_theme(XA_NOTEBOOK(XA_MAIN_WINDOW(window)->notebook), icon_theme);

	icon = gtk_icon_theme_load_icon(icon_theme, "xarchiver", 24, 0, NULL);

	gtk_window_set_icon(GTK_WINDOW(window), icon);

	return window;
}

static void
xa_main_window_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
}

static void
xa_main_window_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
}

GtkWidget *
xa_main_window_find_image(gchar *filename, GtkIconSize size)
{
	GError *error = NULL;
	GtkWidget *file_image;
	gchar *path;
	path = g_strconcat(DATADIR, "/pixmaps/xarchiver/", filename, NULL);
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

static void
cb_xa_main_new_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = xa_new_archive_dialog_new();
	gchar *new_archive_path = NULL;
	XAMainWindow *window = XA_MAIN_WINDOW(userdata);
	LXAArchive *archive = NULL;
	LXAArchiveSupport *support = NULL;
	gint result = 0;

	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog) );
		return;
	}
	if(result == GTK_RESPONSE_OK)
	{
		new_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		
		if(!lxa_new_archive(new_archive_path, TRUE, NULL, &archive))
		{
			support = lxa_get_support_for_mime(lxa_mime_info_get_name(archive->mime_info));
			xa_notebook_add_archive(XA_NOTEBOOK(window->notebook), archive, support);
		}
		else
		{

		}
		gtk_widget_destroy (GTK_WIDGET (dialog) );
	}

}

static void
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
		gtk_widget_destroy (dialog);
		return;
	}
	if(result == GTK_RESPONSE_OK)
	{
		open_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if(xa_notebook_get_multi_tab(XA_NOTEBOOK(window->notebook)))
			xa_main_window_open_archive(window, open_archive_path, -1);
		else
			xa_main_window_open_archive(window, open_archive_path, 0);
		gtk_widget_destroy(dialog);
	}
}


static void
cb_xa_main_extract_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	gchar *extract_archive_path = NULL;
	gint result = 0;
	XAMainWindow *window = XA_MAIN_WINDOW(userdata);

	LXAArchive        *lp_archive = NULL;
	LXAArchiveSupport *lp_support = NULL;

	GSList *filenames = NULL;
	GValue *value = g_new0(GValue, 1);

	GtkTreeView *treeview = GTK_TREE_VIEW(xa_notebook_get_active_child(XA_NOTEBOOK(window->notebook)));
	GtkTreeModel *treemodel = gtk_tree_view_get_model(treeview);
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);

	xa_notebook_get_active_archive(XA_NOTEBOOK(window->notebook), &lp_archive, &lp_support);

	dialog = xa_extract_archive_dialog_new(lp_support, lp_archive, gtk_tree_selection_count_selected_rows (selection));
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(GTK_WIDGET(dialog));
		extract_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(XA_EXTRACT_ARCHIVE_DIALOG(dialog)->sel_files_radio)))
		{
			GList *rows = gtk_tree_selection_get_selected_rows(selection, &treemodel);
			GList *_rows = rows;
			while(_rows)
			{
				gtk_tree_model_get_iter(GTK_TREE_MODEL(treemodel), &iter, _rows->data);
				if(xa_archive_store_get_show_icons(XA_ARCHIVE_STORE(treemodel)))
					gtk_tree_model_get_value(GTK_TREE_MODEL(treemodel), &iter, 1, value);
				else
					gtk_tree_model_get_value(GTK_TREE_MODEL(treemodel), &iter, 0, value);

				g_value_unset(value);
				_rows = _rows->next;
			}
			g_list_free(rows);
		}
		lxa_archive_support_extract(lp_support, lp_archive, extract_archive_path, filenames);
		g_free(extract_archive_path);
		extract_archive_path = NULL;
	}
	gtk_widget_destroy (GTK_WIDGET (dialog) );

}

static void
cb_xa_main_add_to_archive(GtkWidget *widget, gpointer userdata)
{
}

static void
cb_xa_main_stop_archive(GtkWidget *widget, gpointer userdata)
{
}

static void
cb_xa_main_window_notebook_page_switched(XANotebook *notebook, GtkNotebookPage *page, guint page_nr, gpointer data)
{
	XAMainWindow *window = XA_MAIN_WINDOW(data);

	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), TRUE);
}

static void
cb_xa_main_window_notebook_page_removed(XANotebook *notebook, gpointer data)
{
	XAMainWindow *window = XA_MAIN_WINDOW(data);

	if(!gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
	{
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);
	}
}

gint
xa_main_window_open_archive(XAMainWindow *window, gchar *path, gint replace)
{
	LXAArchive *archive = NULL;
	LXAArchiveSupport *support = NULL;

	if(!lxa_open_archive(path, &archive))
	{
		support = lxa_get_support_for_mime(lxa_mime_info_get_name(archive->mime_info));
		if(replace < 0)
			xa_notebook_add_archive(XA_NOTEBOOK(window->notebook), archive, support);
		/* else */
		return 0;
	}
	return 1;
}
