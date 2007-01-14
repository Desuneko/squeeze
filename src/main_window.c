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

#undef SQ_MAIN_ANY_BAR

#ifdef ENABLE_PATHBAR
#define SQ_MAIN_ANY_BAR 1
#else
#ifdef ENABLE_TOOLBAR
#define SQ_MAIN_ANY_BAR 1
#endif
#endif

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>

#ifdef HAVE_LIBXFCE4UTIL
#include <libxfce4util/libxfce4util.h>
#endif

#include "settings.h"
#include "archive_store.h"
#include "navigation_bar.h"
#include "button_drag_box.h"

#ifdef ENABLE_PATHBAR
#include "path_bar.h"
#endif /* ENABLE_PATHBAR */

#ifdef ENABLE_TOOLBAR
#include "tool_bar.h"
#endif /* ENABLE_TOOLBAR */

#include "widget_factory.h"

#include "notebook.h"
#include "application.h"
#include "main_window.h"

#include "new_dialog.h"
#include "extract_dialog.h"
#include "add_dialog.h"
#include "preferences_dialog.h"
#include "properties_dialog.h"

#include "main.h"

enum
{
	SQ_MAIN_WINDOW_NAVIGATION_STYLE = 1
};

static void
sq_main_window_init(SQMainWindow *);
static void
sq_main_window_class_init(SQMainWindowClass *);
static void
sq_main_window_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
sq_main_window_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void
sq_main_window_finalize(GObject *object);


static void cb_sq_main_new_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_open_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_extract_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_add_to_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_remove_from_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_close_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_stop_archive(GtkWidget *widget, gpointer userdata);

static void cb_sq_main_close_window(GtkWidget *widget, gpointer userdata);

static void cb_sq_main_properties(GtkWidget *widget, gpointer userdata);
/*static void cb_sq_main_preferences(GtkWidget *widget, gpointer userdata);*/

static void cb_sq_main_about(GtkWidget *widget, gpointer userdata);

static void
cb_sq_main_window_notebook_page_switched(SQNotebook *, GtkNotebookPage *, guint, gpointer);
static void
cb_sq_main_window_notebook_page_removed(SQNotebook *, gpointer);
static void
cb_sq_main_window_notebook_file_activated(SQNotebook *, gchar *, gpointer);
static void
cb_sq_main_window_notebook_status_changed(SQNotebook *, LSQArchive *, gpointer);

static void
sq_main_window_set_navigation(SQMainWindow *window);

GType
sq_main_window_navigation_style_get_type()
{
	static GType nav_style_type = 0;

	if(!nav_style_type)
	{
		static GEnumValue style_types[] = {
			{SQ_MAIN_WINDOW_NAVIGATION_INTERNAL, "internal", N_("Internal Style")},
#ifdef ENABLE_TOOLBAR
			{SQ_MAIN_WINDOW_NAVIGATION_TOOL_BAR, "tool_bar", N_("Tool Bar Style")},
#endif
#ifdef ENABLE_PATHBAR
			{SQ_MAIN_WINDOW_NAVIGATION_PATH_BAR, "path_bar", N_("Path Bar Style")},
#endif
			{0, NULL, NULL}
		};

		nav_style_type = g_enum_register_static("SQMainWindowNavigationStyle", style_types);
	}

	return nav_style_type;
}

GType
sq_main_window_get_type ()
{
	static GType sq_main_window_type = 0;

	if (!sq_main_window_type)
	{
		static const GTypeInfo sq_main_window_info = 
		{
			sizeof (SQMainWindowClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_main_window_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQMainWindow),
			0,
			(GInstanceInitFunc) sq_main_window_init,
			NULL
		};

		sq_main_window_type = g_type_register_static (GTK_TYPE_WINDOW, "SQMainWindow", &sq_main_window_info, 0);
	}
	return sq_main_window_type;
}

static void
sq_main_window_class_init(SQMainWindowClass *window_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (window_class);
	GParamSpec *pspec = NULL;

	object_class->set_property = sq_main_window_set_property;
	object_class->get_property = sq_main_window_get_property;
	object_class->finalize     = sq_main_window_finalize;

	pspec = g_param_spec_enum("navigation-style",
		_("Navigation Style"),
		_("Style of navigation\nThe style to navigate trough the archive"),
		SQ_TYPE_MAIN_WINDOW_NAVIGATION_STYLE,
		SQ_MAIN_WINDOW_NAVIGATION_INTERNAL,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_MAIN_WINDOW_NAVIGATION_STYLE, pspec);



}

static void
sq_main_window_finalize(GObject *object)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(object);


	sq_settings_set_group(window->settings, "Global");
	if(window->menu_bar)
		sq_settings_write_bool_entry(window->settings, "MenuBar", TRUE);
	else
		sq_settings_write_bool_entry(window->settings, "MenuBar", FALSE);

	if(!window->navigationbar)
	{
		sq_settings_write_entry(window->settings, "NavigationBar", "None");
	}
#ifdef ENABLE_TOOLBAR
	else if(SQ_IS_TOOL_BAR(window->navigationbar))
	{
		sq_settings_write_entry(window->settings, "NavigationBar", "ToolBar");
	}	
#endif
#ifdef ENABLE_PATHBAR
	else if(SQ_IS_PATH_BAR(window->navigationbar))
	{
		sq_settings_write_entry(window->settings, "NavigationBar", "PathBar");
	}
#endif
	else
	{
		sq_settings_write_entry(window->settings, "NavigationBar", "None");
	}

	sq_settings_save(window->settings);

	g_object_unref(G_OBJECT(window->settings));

	//if(window->navigationbar)
	//	gtk_widget_destroy(GTK_WIDGET(window->navigationbar));

	g_object_unref(G_OBJECT(window->app));
}

static void
sq_main_window_init(SQMainWindow *window)
{
	GtkWidget     *toolbar;
	GtkToolItem   *tool_separator;
	GtkWidget     *menu_separator;
	GtkWidget     *tmp_image;
	const gchar   *nav_bar;
	GSList        *list, *iter;
	gboolean up_dir = TRUE;
	gboolean show_icons = TRUE;
	gboolean sort_case = TRUE;
	gboolean sort_folders = TRUE;
	gboolean use_tabs = TRUE;
	gboolean show_menubar = TRUE;

	window->accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(window), window->accel_group);

	window->settings = sq_settings_new();

	sq_settings_set_group(window->settings, "Global");

	window->main_vbox = gtk_vbox_new(FALSE, 0);

	window->widget_factory = sq_widget_factory_new();

	show_menubar = sq_settings_read_bool_entry(window->settings, "MenuBar", TRUE);

	if(show_menubar)
	{
		window->menu_bar = gtk_menu_bar_new();

		/* File menu */
		window->menubar.menu_item_file = gtk_menu_item_new_with_mnemonic(_("_File"));
		window->menubar.menu_file = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(window->menubar.menu_item_file), window->menubar.menu_file);

		window->menubar.menu_item_new = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, window->accel_group);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), window->menubar.menu_item_new);
		window->menubar.menu_item_open = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, window->accel_group);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), window->menubar.menu_item_open);
		gtk_widget_add_accelerator (window->menubar.menu_item_open, "activate", window->accel_group, GDK_o, GDK_SHIFT_MASK| GDK_CONTROL_MASK, GTK_ACCEL_LOCKED | GTK_ACCEL_MASK);

		menu_separator = gtk_separator_menu_item_new();
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), menu_separator);

		window->menubar.menu_item_properties = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, window->accel_group);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), window->menubar.menu_item_properties);
		gtk_widget_set_sensitive(window->menubar.menu_item_properties, FALSE);

		window->menubar.menu_item_close = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLOSE, window->accel_group);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), window->menubar.menu_item_close);
		gtk_widget_set_sensitive(window->menubar.menu_item_close, FALSE);

		menu_separator = gtk_separator_menu_item_new();
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), menu_separator);

		window->menubar.menu_item_quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, window->accel_group);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), window->menubar.menu_item_quit);

		g_signal_connect(G_OBJECT(window->menubar.menu_item_new), "activate", G_CALLBACK(cb_sq_main_new_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_open), "activate", G_CALLBACK(cb_sq_main_open_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_properties), "activate", G_CALLBACK(cb_sq_main_properties), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_close), "activate", G_CALLBACK(cb_sq_main_close_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_quit), "activate", G_CALLBACK(cb_sq_main_close_window), window);
		/* Action menu: ref all the childs*/
		window->menubar.menu_item_action = gtk_menu_item_new_with_mnemonic(_("_Action"));
		window->menubar.menu_action = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(window->menubar.menu_item_action), window->menubar.menu_action);

		tmp_image = sq_main_window_find_image("archive-add.png", GTK_ICON_SIZE_MENU);
		window->menubar.menu_item_add = g_object_ref(gtk_image_menu_item_new_with_mnemonic(_("_Add")));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(window->menubar.menu_item_add), tmp_image);
		gtk_widget_set_sensitive(window->menubar.menu_item_add, FALSE);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_add);

		tmp_image = sq_main_window_find_image("archive-extract.png", GTK_ICON_SIZE_MENU);
		window->menubar.menu_item_extract = g_object_ref(gtk_image_menu_item_new_with_mnemonic(_("_Extract")));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(window->menubar.menu_item_extract), tmp_image);
		gtk_widget_set_sensitive(window->menubar.menu_item_extract, FALSE);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_extract);

		window->menubar.menu_item_remove = g_object_ref(gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, window->accel_group));
		gtk_widget_set_sensitive(window->menubar.menu_item_remove, FALSE);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_remove);

		g_signal_connect(G_OBJECT(window->menubar.menu_item_add), "activate", G_CALLBACK(cb_sq_main_add_to_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_extract), "activate", G_CALLBACK(cb_sq_main_extract_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_remove), "activate", G_CALLBACK(cb_sq_main_remove_from_archive), window);

		/* View menu */
		window->menubar.menu_item_view = gtk_menu_item_new_with_mnemonic(_("_View"));
		window->menubar.menu_view = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(window->menubar.menu_item_view), window->menubar.menu_view);

#ifdef SQ_MAIN_ANY_BAR
		list = sq_widget_factory_create_property_menu(window->widget_factory, G_OBJECT(window), "navigation-style");
		for(iter = list; iter; iter = iter->next)
		{
			gtk_container_add(GTK_CONTAINER(window->menubar.menu_view), iter->data);
			gtk_widget_show(iter->data);
		}
#endif

/*
		window->menubar.menu_item_settings = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, window->accel_group);

		g_signal_connect(G_OBJECT(window->menubar.menu_item_settings), "activate", G_CALLBACK(cb_sq_main_preferences), window);

		gtk_container_add(GTK_CONTAINER(window->menubar.menu_view), window->menubar.menu_item_settings);
*/

		gtk_widget_show_all(window->menubar.menu_view);

		/* Help menu */
		window->menubar.menu_item_help = gtk_menu_item_new_with_mnemonic(_("_Help"));
		window->menubar.menu_help = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(window->menubar.menu_item_help), window->menubar.menu_help);

		window->menubar.menu_item_about = g_object_ref(gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, window->accel_group));
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_help), window->menubar.menu_item_about);

		g_signal_connect(G_OBJECT(window->menubar.menu_item_about), "activate", G_CALLBACK(cb_sq_main_about), window);

		gtk_menu_bar_append(GTK_MENU_BAR(window->menu_bar), window->menubar.menu_item_file);
		gtk_menu_bar_append(GTK_MENU_BAR(window->menu_bar), window->menubar.menu_item_action);
		gtk_menu_bar_append(GTK_MENU_BAR(window->menu_bar), window->menubar.menu_item_view);
		gtk_menu_bar_append(GTK_MENU_BAR(window->menu_bar), window->menubar.menu_item_help);
	}

	toolbar = gtk_toolbar_new();

/* Archive pane */
	window->toolbar.tool_item_new = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	window->toolbar.tool_item_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	tool_separator = gtk_separator_tool_item_new ();

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_new));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_open));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(tool_separator));

	g_signal_connect(G_OBJECT(window->toolbar.tool_item_new), "clicked", G_CALLBACK(cb_sq_main_new_archive), window);
	g_signal_connect(G_OBJECT(window->toolbar.tool_item_open), "clicked", G_CALLBACK(cb_sq_main_open_archive), window);

/* Action pane */
	tmp_image = sq_main_window_find_image("archive-add.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
	window->toolbar.tool_item_add = gtk_tool_button_new(tmp_image, _("Add"));
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), FALSE);

	tmp_image = sq_main_window_find_image("archive-extract.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
	window->toolbar.tool_item_extract = gtk_tool_button_new(tmp_image, _("Extract"));
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);

	window->toolbar.tool_item_remove = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);

	tool_separator = gtk_separator_tool_item_new ();

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_add));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_extract));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_remove));
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(tool_separator));

	g_signal_connect(G_OBJECT(window->toolbar.tool_item_add), "clicked", G_CALLBACK(cb_sq_main_add_to_archive), window);
	g_signal_connect(G_OBJECT(window->toolbar.tool_item_extract), "clicked", G_CALLBACK(cb_sq_main_extract_archive), window);
	g_signal_connect(G_OBJECT(window->toolbar.tool_item_remove), "clicked", G_CALLBACK(cb_sq_main_remove_from_archive), window);

/* control pane */

	window->toolbar.tool_item_stop = gtk_tool_button_new_from_stock(GTK_STOCK_STOP);
	gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(window->toolbar.tool_item_stop));

	g_signal_connect(G_OBJECT(window->toolbar.tool_item_stop), "clicked", G_CALLBACK(cb_sq_main_stop_archive), window);

	nav_bar = sq_settings_read_entry(window->settings, "NavigationBar", "None");
	window->nav_style = SQ_MAIN_WINDOW_NAVIGATION_INTERNAL;
	window->navigationbar = NULL;
	up_dir = TRUE;

#ifdef ENABLE_TOOLBAR
	if(!strcmp(nav_bar, "ToolBar"))
	{
		window->nav_style = SQ_MAIN_WINDOW_NAVIGATION_TOOL_BAR;
		window->navigationbar = sq_tool_bar_new(NULL); 
		up_dir = FALSE;
	}
#endif
#ifdef ENABLE_PATHBAR
	if(!strcmp(nav_bar, "PathBar"))
	{
		window->nav_style = SQ_MAIN_WINDOW_NAVIGATION_PATH_BAR;
		window->navigationbar = sq_path_bar_new(NULL);
		gtk_container_set_border_width(GTK_CONTAINER(window->navigationbar), 3);
		up_dir = FALSE;
	}
#endif

	g_object_notify(G_OBJECT(window), "navigation-style");

	show_icons = sq_settings_read_bool_entry(window->settings, "ShowIcons", TRUE);
	sort_case = sq_settings_read_bool_entry(window->settings, "SortCaseSensitive", TRUE);
	sort_folders = sq_settings_read_bool_entry(window->settings, "SortFoldersFirst", TRUE);
	use_tabs = sq_settings_read_bool_entry(window->settings, "UseTabs", TRUE);

/* main view */
	window->notebook = sq_notebook_new(window->navigationbar, use_tabs, window->accel_group);
	g_signal_connect(G_OBJECT(window->notebook), "switch-page", G_CALLBACK(cb_sq_main_window_notebook_page_switched), window);
	g_signal_connect(G_OBJECT(window->notebook), "archive-removed", G_CALLBACK(cb_sq_main_window_notebook_page_removed), window);
	g_signal_connect(G_OBJECT(window->notebook), "file-activated", G_CALLBACK(cb_sq_main_window_notebook_file_activated), window);
	g_signal_connect(G_OBJECT(window->notebook), "active-archive-status-changed", G_CALLBACK(cb_sq_main_window_notebook_status_changed), window);

/* menu item */
	list = sq_widget_factory_create_property_menu(window->widget_factory, G_OBJECT(window->notebook), "show-icons");
	for(iter = list; iter; iter = iter->next)
	{
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_view), iter->data);
		gtk_widget_show(iter->data);
	}
	list = sq_widget_factory_create_property_menu(window->widget_factory, G_OBJECT(window->notebook), "sort-folders-first");
	for(iter = list; iter; iter = iter->next)
	{
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_view), iter->data);
		gtk_widget_show(iter->data);
	}
	list = sq_widget_factory_create_property_menu(window->widget_factory, G_OBJECT(window->notebook), "sort-case-sensitive");
	for(iter = list; iter; iter = iter->next)
	{
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_view), iter->data);
		gtk_widget_show(iter->data);
	}
	list = sq_widget_factory_create_property_menu(window->widget_factory, G_OBJECT(window->notebook), "rules-hint");
	for(iter = list; iter; iter = iter->next)
	{
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_view), iter->data);
		gtk_widget_show(iter->data);
	}

/* Statusbar */

	window->statusbar = gtk_statusbar_new();

	if(show_menubar)
		gtk_box_pack_start(GTK_BOX(window->main_vbox), window->menu_bar, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(window->main_vbox), toolbar, FALSE, FALSE, 0);

	if(window->navigationbar)
	{
		gtk_widget_ref(GTK_WIDGET(window->navigationbar));
		gtk_box_pack_start(GTK_BOX(window->main_vbox), GTK_WIDGET(window->navigationbar), FALSE, FALSE, 0);
		gtk_widget_show_all(GTK_WIDGET(window->navigationbar));
	}

	gtk_box_pack_start(GTK_BOX(window->main_vbox), window->notebook, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(window->main_vbox), window->statusbar, FALSE, FALSE, 0);

	gtk_widget_show_all(window->main_vbox);
	gtk_widget_show_all(toolbar);
	gtk_widget_show_all(window->notebook);
	gtk_widget_show_all(window->statusbar);

	gtk_container_add(GTK_CONTAINER(window), window->main_vbox);
}

GtkWidget *
sq_main_window_new(SQApplication *app, GtkIconTheme *icon_theme)
{
	SQMainWindow *window;
	GdkPixbuf *icon;

	window = g_object_new(sq_main_window_get_type(),
			"title", PACKAGE_STRING,
			NULL);

	window->icon_theme = icon_theme;

	sq_notebook_set_icon_theme(SQ_NOTEBOOK(window->notebook), icon_theme);

	icon = gtk_icon_theme_load_icon(icon_theme, "squeeze", 24, 0, NULL);

	g_object_ref(app);
	window->app = app;

	gtk_window_set_icon(GTK_WINDOW(window), icon);

	return GTK_WIDGET(window);
}

static void
sq_main_window_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(object);
	switch(prop_id)
	{
		case SQ_MAIN_WINDOW_NAVIGATION_STYLE:
			g_value_set_enum(value, window->nav_style);
		break;
	}
}

static void
sq_main_window_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(object);
	switch(prop_id)
	{
		case SQ_MAIN_WINDOW_NAVIGATION_STYLE:
			window->nav_style = g_value_get_enum(value);
			sq_main_window_set_navigation(window);
		break;
	}
}

GtkWidget *
sq_main_window_find_image(gchar *filename, GtkIconSize size)
{
	GError *error = NULL;
	gint width  = 0;
	gint height = 0;
	GtkWidget *file_image;
	gchar *path;
	path = g_strconcat(DATADIR, "/pixmaps/squeeze/", filename, NULL);

	gtk_icon_size_lookup(size, &width, &height);

	GdkPixbuf *file_pixbuf = gdk_pixbuf_new_from_file_at_size(path, width, height, &error);
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
sq_main_window_new_action_menu(SQMainWindow *window, LSQArchiveSupport *support, LSQArchive *archive)
{
	GSList *iter, *list;

	gtk_container_remove(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_add);
	gtk_container_remove(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_extract);
	gtk_container_remove(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_remove);

	window->menubar.menu_action = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(window->menubar.menu_item_action), window->menubar.menu_action);

	gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_add);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_extract);
	gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_remove);

	if(support)
	{
		iter = list = sq_widget_factory_create_action_menu(window->widget_factory, support, archive);
		if(list)
		{
			gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), gtk_separator_menu_item_new());

			do
			{
				gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), iter->data);
			}
			while((iter = iter->next));

			g_slist_free(list);
		}
	}

	gtk_widget_show_all(window->menubar.menu_action);
}

static void
cb_sq_main_new_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = sq_new_archive_dialog_new();
	gchar *new_archive_path = NULL;
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	LSQArchive *archive = NULL;
	LSQArchiveSupport *support = NULL;
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
		
		if(!lsq_new_archive(new_archive_path, TRUE, NULL, &archive))
		{
			support = lsq_get_support_for_mime(archive->mime_info);
			sq_notebook_add_archive(SQ_NOTEBOOK(window->notebook), archive, support, TRUE);
		}
		else
		{

		}
		gtk_widget_destroy (dialog );
	}

}

static void
cb_sq_main_open_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	GSList *open_archive_paths = NULL;
	GSList *_open_archive_paths = NULL;
	gint result = 0;
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	gint x, y;
	GdkModifierType mod_type;

	gdk_window_get_pointer(GTK_WIDGET(window)->window, &x, &y, &mod_type);
	
	if(mod_type & GDK_SHIFT_MASK)
		dialog = gtk_file_chooser_dialog_new(_("Open archive in new window"), 
																			 GTK_WINDOW(window),
																			 GTK_FILE_CHOOSER_ACTION_OPEN,
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
																			 GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
	else
		dialog = gtk_file_chooser_dialog_new(_("Open archive"), 
																			 GTK_WINDOW(window),
																			 GTK_FILE_CHOOSER_ACTION_OPEN,
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
																			 GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (dialog);
		return;
	}
	if(result == GTK_RESPONSE_OK)
	{
		open_archive_paths = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		_open_archive_paths = open_archive_paths;
		while(_open_archive_paths)
		{
			if(mod_type & GDK_SHIFT_MASK)
				sq_application_open_archive(window->app, NULL, _open_archive_paths->data);
			else
				sq_application_open_archive(window->app, (GtkWidget *)window, _open_archive_paths->data);
			/*
			if(sq_notebook_get_multi_tab(SQ_NOTEBOOK(window->notebook)))
				sq_application_open_archive(window->app, (GtkWidget *)window, _open_archive_paths->data);
			else
				sq_application_open_archive(window->app, NULL, _open_archive_paths->data);
			*/
			_open_archive_paths = _open_archive_paths->next;
		}
		g_slist_foreach(open_archive_paths, (GFunc)g_free, NULL);
		g_slist_free(open_archive_paths);
		gtk_widget_destroy(dialog);
	}
}


static void
cb_sq_main_extract_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = NULL;
	gchar *extract_archive_path = NULL;
	gint result = 0;
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);

	LSQArchive        *lp_archive = NULL;
	LSQArchiveSupport *lp_support = NULL;

	GSList *filenames = sq_notebook_get_selected_items(SQ_NOTEBOOK(window->notebook));

	sq_notebook_get_active_archive(SQ_NOTEBOOK(window->notebook), &lp_archive, &lp_support);

	dialog = sq_extract_archive_dialog_new(lp_support, lp_archive, g_slist_length(filenames));
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(dialog);
		extract_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SQ_EXTRACT_ARCHIVE_DIALOG(dialog)->all_files_radio)))
		{
			g_slist_free(filenames);
			filenames = NULL;
		}
		lsq_archive_support_extract(lp_support, lp_archive, extract_archive_path, filenames);
		g_free(extract_archive_path);
		extract_archive_path = NULL;
	}
	gtk_widget_destroy (dialog);

}

static void
cb_sq_main_add_to_archive(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);

	LSQArchive        *lp_archive = NULL;
	LSQArchiveSupport *lp_support = NULL;
	GtkWidget         *dialog = NULL;
	GSList            *filenames = NULL;
	gint result;
	sq_notebook_get_active_archive(SQ_NOTEBOOK(window->notebook), &lp_archive, &lp_support);

	dialog = sq_add_dialog_new(lp_support);

	result = gtk_dialog_run (GTK_DIALOG(dialog));
	if(result == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(dialog);
		filenames = sq_add_dialog_get_filenames(SQ_ADD_DIALOG(dialog));
		if(filenames)
			lsq_archive_support_add(lp_support, lp_archive, filenames);
	}
	gtk_widget_destroy (dialog);
}

static void
cb_sq_main_remove_from_archive(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	LSQArchive        *lp_archive = NULL;
	LSQArchiveSupport *lp_support = NULL;
	GtkWidget *dialog = NULL;
	gint result = 0;
	GSList *filenames = sq_notebook_get_selected_items(SQ_NOTEBOOK(window->notebook));

	if(filenames)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "Are you sure you want to remove the selected files?");
		result = gtk_dialog_run(GTK_DIALOG(dialog));
		if(result == GTK_RESPONSE_YES)
		{
			gtk_widget_hide(dialog);
			sq_notebook_get_active_archive(SQ_NOTEBOOK(window->notebook), &lp_archive, &lp_support);
			lsq_archive_support_remove(lp_support, lp_archive, filenames);
		}
		gtk_widget_destroy (dialog);
	}
}

static void
cb_sq_main_close_archive(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	sq_notebook_close_active_archive(SQ_NOTEBOOK(window->notebook));
}

static void
cb_sq_main_close_window(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	gtk_widget_destroy(GTK_WIDGET(window));
}

static void
cb_sq_main_stop_archive(GtkWidget *widget, gpointer userdata)
{

}

static void 
cb_sq_main_properties(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	LSQArchive *lp_archive = NULL;

	sq_notebook_get_active_archive(SQ_NOTEBOOK(window->notebook), &lp_archive, NULL);

	GtkWidget *dialog = sq_properties_dialog_new(lp_archive, window->icon_theme);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/*
static void
cb_sq_main_preferences(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = sq_preferences_dialog_new();

	gtk_dialog_run(GTK_DIALOG(dialog));

	GSList *iter = SQ_PREFERENCES_DIALOG(dialog)->support.support_list;
	SQButtonDragBox *box;
	LSQArchiveSupport *support;
	GSList *buttons, *button_iter;
	
	while(iter)
	{
		support = ((SQSupportTuple*)iter->data)->support;
		box = SQ_BUTTON_DRAG_BOX(((SQSupportTuple*)iter->data)->box);

		buttons = button_iter = sq_button_drag_box_get_visible(box);

		while(button_iter)
		{
			g_object_set(G_OBJECT(support), (const gchar*)button_iter->data, TRUE, NULL);

			button_iter = g_slist_next(button_iter);
		}
		g_slist_free(buttons);

		buttons = button_iter = sq_button_drag_box_get_hidden(box);

		while(button_iter)
		{
			g_object_set(G_OBJECT(support), (const gchar*)button_iter->data, FALSE, NULL);

			button_iter = g_slist_next(button_iter);
		}
		g_slist_free(buttons);

		g_free(iter->data);

		iter = g_slist_next(iter);
	}
	
	g_slist_free(SQ_PREFERENCES_DIALOG(dialog)->support.support_list);
	SQ_PREFERENCES_DIALOG(dialog)->support.support_list = NULL;

	gtk_widget_destroy(dialog);
}
*/

static void
cb_sq_main_about(GtkWidget *widget, gpointer userdata)
{
	const gchar *authors[] = {
	  _("Lead developer:"),
		"Stephan Arts <stephan@xfce.org>",
		"",
		_("Contributors:"),
		"Peter de Ridder <peter@xfce.org>",
		"",
		_("Inspired by Xarchiver, written by Giuseppe Torelli"), NULL};
	GtkWidget *about_dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_name((GtkAboutDialog *)about_dialog, PACKAGE_NAME);
	gtk_about_dialog_set_version((GtkAboutDialog *)about_dialog, PACKAGE_VERSION);
	gtk_about_dialog_set_comments((GtkAboutDialog *)about_dialog, _("Squeeze is a lightweight and flexible archive manager for the Xfce Desktop Environment"));

	gtk_about_dialog_set_logo_icon_name((GtkAboutDialog *)about_dialog, "squeeze");

	gtk_about_dialog_set_authors((GtkAboutDialog *)about_dialog, authors);

	gtk_about_dialog_set_translator_credits((GtkAboutDialog *)about_dialog, _("translator-credits"));

	gtk_about_dialog_set_license((GtkAboutDialog *)about_dialog, xfce_get_license_text(XFCE_LICENSE_TEXT_GPL));

	gtk_dialog_run(GTK_DIALOG(about_dialog));

	gtk_widget_destroy(about_dialog);

}

static void
cb_sq_main_window_notebook_page_switched(SQNotebook *notebook, GtkNotebookPage *page, guint page_nr, gpointer data)
{
	LSQArchive *lp_archive;
	LSQArchiveSupport *lp_support;
	sq_notebook_page_get_archive(notebook, &lp_archive, &lp_support, page_nr);
	SQMainWindow *window = SQ_MAIN_WINDOW(data);

	if(lp_archive || lsq_archive_get_status(lp_archive) == LSQ_ARCHIVESTATUS_IDLE)
	{
		gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_add), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_extract), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_remove), TRUE);

		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_add), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_extract), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->menubar.menu_item_remove), FALSE);

		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), TRUE);
	}

	gtk_window_set_title(GTK_WINDOW(window), g_strconcat(PACKAGE_NAME, " - ", lsq_archive_get_filename(lp_archive), NULL));

	sq_main_window_new_action_menu(window, lp_support, lp_archive);

	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(window->statusbar), "Window Statusbar");
	gtk_statusbar_push(GTK_STATUSBAR(window->statusbar), context_id, lsq_archive_get_status_msg(lp_archive));
}

static void
cb_sq_main_window_notebook_page_removed(SQNotebook *notebook, gpointer data)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(data);

	if(!gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
	{
		gtk_widget_set_sensitive(window->menubar.menu_item_close, FALSE);
		gtk_widget_set_sensitive(window->menubar.menu_item_properties, FALSE);

		gtk_widget_set_sensitive(window->menubar.menu_item_add, FALSE);
		gtk_widget_set_sensitive(window->menubar.menu_item_extract, FALSE);
		gtk_widget_set_sensitive(window->menubar.menu_item_remove, FALSE);

		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);

		gtk_window_set_title(GTK_WINDOW(window), PACKAGE_STRING);
	}
}

static void
cb_sq_main_window_notebook_file_activated(SQNotebook *notebook, gchar *path, gpointer data)
{
	GtkWindow *window = GTK_WINDOW(data);
	LSQArchive *lp_archive = NULL;
	LSQArchiveSupport *lp_support = NULL;
	gchar *extract_archive_path = NULL;
	GtkWidget *label = gtk_label_new(_("Which action do you want to perform on the selected file(s)?"));
	GtkWidget *dialog = gtk_dialog_new_with_buttons("",window,GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, _("View"), GTK_RESPONSE_OK, _("Extract"), GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	GtkWidget *extr_dialog = NULL;
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 20);
	gtk_widget_show(label);
	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);

	GSList *filenames = g_slist_prepend(NULL, path);
	switch(result)
	{
		case GTK_RESPONSE_OK: /* VIEW */
			/* extract to tmp and view */
			break;
		case GTK_RESPONSE_ACCEPT: /* EXTRACT */
			sq_notebook_get_active_archive(SQ_NOTEBOOK(notebook), &lp_archive, &lp_support);
			extr_dialog = sq_extract_archive_dialog_new(lp_support, lp_archive, 1);
			result = gtk_dialog_run (GTK_DIALOG (extr_dialog) );
			if(result == GTK_RESPONSE_OK)
			{
				gtk_widget_hide(extr_dialog);
				extract_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(extr_dialog));
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SQ_EXTRACT_ARCHIVE_DIALOG(extr_dialog)->all_files_radio)))
				{
					g_slist_free(filenames);
					filenames = NULL;
				}
				lsq_archive_support_extract(lp_support, lp_archive, extract_archive_path, filenames);
				g_free(extract_archive_path);
				extract_archive_path = NULL;
			}
			gtk_widget_destroy (extr_dialog);
			
			break;
		case GTK_RESPONSE_CANCEL: /* CANCEL */
			break;
	}
	g_slist_free(filenames);
	gtk_widget_destroy(dialog);
}

gint
sq_main_window_open_archive(SQMainWindow *window, gchar *path, gint replace)
{
	LSQArchive *archive = NULL;
	LSQArchiveSupport *support = NULL;

	if(!lsq_open_archive(path, &archive))
	{
		support = lsq_get_support_for_mime(archive->mime_info);
		if(replace < 0)
			sq_notebook_add_archive(SQ_NOTEBOOK(window->notebook), archive, support, FALSE);
		else
			sq_notebook_page_set_archive(SQ_NOTEBOOK(window->notebook), archive, support, replace);
		gtk_widget_set_sensitive(window->menubar.menu_item_close, TRUE);
		gtk_widget_set_sensitive(window->menubar.menu_item_properties, TRUE);
		return 0;
	}
	else
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), 
				GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL, 
				GTK_MESSAGE_ERROR, 
				GTK_BUTTONS_OK,
				_("Failed to open file"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), _("'%s'\nCould not be opened"), path);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	return 1;
}

static void
sq_main_window_set_navigation(SQMainWindow *window)
{
	SQNavigationBar *nav_bar = NULL; 
	gboolean up_dir = TRUE;

	switch(window->nav_style)
	{
		case SQ_MAIN_WINDOW_NAVIGATION_INTERNAL:
			break;
#ifdef ENABLE_TOOLBAR
		case SQ_MAIN_WINDOW_NAVIGATION_TOOL_BAR:
			nav_bar = sq_tool_bar_new(NULL);
			up_dir = FALSE;
			break;
#endif
#ifdef ENABLE_PATHBAR
		case SQ_MAIN_WINDOW_NAVIGATION_PATH_BAR:
			nav_bar = sq_path_bar_new(NULL);
			up_dir = FALSE;
			break;
#endif
		default:
			return;
	}

	sq_notebook_set_navigation_bar(SQ_NOTEBOOK(window->notebook), nav_bar);

	if(window->navigationbar)
		gtk_widget_destroy(GTK_WIDGET(window->navigationbar));

	window->navigationbar = nav_bar;
	if(nav_bar)
	{
		gtk_box_pack_start(GTK_BOX(window->main_vbox), (GtkWidget *)nav_bar, FALSE, FALSE, 0);
		gtk_box_reorder_child(GTK_BOX(window->main_vbox), (GtkWidget *)nav_bar, 2);
		gtk_widget_show_all((GtkWidget *)nav_bar);
	}	
}

static void
cb_sq_main_window_notebook_status_changed(SQNotebook *notebook, LSQArchive *archive, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);

	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(window->statusbar), "Window Statusbar");
	gtk_statusbar_push(GTK_STATUSBAR(window->statusbar), context_id, lsq_archive_get_status_msg(archive));
}
