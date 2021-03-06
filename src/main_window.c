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
#include <gio/gio.h>
#include <libxfce4util/libxfce4util.h>
#include <libsqueeze/libsqueeze.h>

#ifdef HAVE_LIBXFCE4UTIL
#include <libxfce4util/libxfce4util.h>
#endif

#include "settings.h"
#include "archive_store.h"
#include "navigation_bar.h"

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
#include "throbber.h"

#include "new_dialog.h"
#include "extract_dialog.h"

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
sq_main_window_dispose(GObject *object);

static gboolean show_toolbar = TRUE;


static void cb_sq_main_new_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_open_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_extract_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_add_files_to_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_add_folders_to_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_remove_from_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_close_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_stop_archive(GtkWidget *widget, gpointer userdata);
static void cb_sq_main_refresh_archive(GtkWidget *widget, gpointer userdata);

static void cb_sq_main_close_window(GtkWidget *widget, gpointer userdata);

/*static void cb_sq_main_properties(GtkWidget *widget, gpointer userdata);*/
/*static void cb_sq_main_preferences(GtkWidget *widget, gpointer userdata);*/

static void cb_sq_main_about(GtkWidget *widget, gpointer userdata);

static void
cb_sq_main_window_notebook_page_switched(SQNotebook *, GtkNotebookPage *, guint, gpointer);
static void
cb_sq_main_window_notebook_page_removed(SQNotebook *, gpointer);
static void
cb_sq_main_window_notebook_file_activated(SQNotebook *, LSQArchiveIter *, gpointer);
 static void cb_sq_main_window_notebook_state_changed(SQNotebook *, LSQArchive *, gpointer);

static void
sq_main_window_set_navigation(SQMainWindow *window);

static GObjectClass *parent_class;

GType
sq_main_window_navigation_style_get_type(void)
{
	static GType nav_style_type = 0;
	guint i = 0;

	if(!nav_style_type)
	{
		static GEnumValue style_types[] = {
			{SQ_MAIN_WINDOW_NAVIGATION_INTERNAL, "internal", NULL},
#ifdef ENABLE_TOOLBAR
			{SQ_MAIN_WINDOW_NAVIGATION_TOOL_BAR, "tool_bar", NULL},
#endif
#ifdef ENABLE_PATHBAR
			{SQ_MAIN_WINDOW_NAVIGATION_PATH_BAR, "path_bar", NULL},
#endif
			{0, NULL, NULL}
		};
		style_types[0].value_nick = _("Internal Style");
#ifdef ENABLE_TOOLBAR
		style_types[++i].value_nick = _("Tool Bar Style");
#endif
#ifdef ENABLE_PATHBAR
		style_types[++i].value_nick = _("Path Bar Style");
#endif

		nav_style_type = g_enum_register_static("SQMainWindowNavigationStyle", style_types);
	}

	return nav_style_type;
}

GType
sq_main_window_get_type (void)
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

	parent_class = gtk_type_class (GTK_TYPE_WINDOW);

	object_class->set_property = sq_main_window_set_property;
	object_class->get_property = sq_main_window_get_property;
	object_class->dispose	  = sq_main_window_dispose;

	pspec = g_param_spec_enum("navigation-style",
		_("Navigation Style"),
		_("Style of navigation\nThe style to navigate trough the archive"),
		SQ_TYPE_MAIN_WINDOW_NAVIGATION_STYLE,
		SQ_MAIN_WINDOW_NAVIGATION_INTERNAL,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_MAIN_WINDOW_NAVIGATION_STYLE, pspec);



}

static void
sq_main_window_dispose(GObject *object)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(object);
	gint width, height;

	if(window->main_vbox && window->notebook)
	{
		gtk_container_remove(GTK_CONTAINER(window->main_vbox), GTK_WIDGET(window->notebook));
		window->notebook = NULL;
	}

	if(window->settings)
	{
		sq_settings_set_group(window->settings, "Global");
		if(window->menu_bar)
			sq_settings_write_bool_entry(window->settings, "MenuBar", TRUE);
		else
			sq_settings_write_bool_entry(window->settings, "MenuBar", FALSE);

		if(window->tool_bar)
			sq_settings_write_bool_entry(window->settings, "ToolBar", TRUE);
		else
			sq_settings_write_bool_entry(window->settings, "ToolBar", FALSE);

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

		if(&window->parent != NULL)
		{
			gtk_window_get_size(&window->parent, &width, &height);
			sq_settings_write_int_entry(window->settings, "LastWindowWidth", width);
			sq_settings_write_int_entry(window->settings, "LastWindowHeight", height);
		}

		sq_settings_save(window->settings);

		g_object_unref(G_OBJECT(window->settings));
		window->settings = NULL;
	}

	if(window->app)
	{
		g_object_unref(G_OBJECT(window->app));
		window->app = NULL;
	}
	
	parent_class->dispose(object);
}

static void
sq_main_window_init(SQMainWindow *window)
{
	GtkToolItem   *tool_separator;
	GtkWidget	 *menu_separator;
	GtkWidget	 *tmp_image;
	const gchar   *nav_bar;
	GSList		*list, *iter;
	gboolean use_tabs = TRUE;
	gboolean show_menubar = TRUE;
	GtkWidget *item;

	window->accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(window), window->accel_group);

	window->settings = sq_settings_new();

	sq_settings_set_group(window->settings, "Global");

	window->main_vbox = gtk_vbox_new(FALSE, 0);

	window->widget_factory = sq_widget_factory_new();

	show_menubar = sq_settings_read_bool_entry(window->settings, "MenuBar", TRUE);

	gtk_window_set_default_size (GTK_WINDOW(window), 
								 sq_settings_read_int_entry(window->settings, "LastWindowWidth", 500),
								 sq_settings_read_int_entry(window->settings, "LastWindowHeight", 300));

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

		window->menubar.menu_item_close = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLOSE, window->accel_group);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), window->menubar.menu_item_close);
		gtk_widget_set_sensitive(window->menubar.menu_item_close, FALSE);

		menu_separator = gtk_separator_menu_item_new();
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), menu_separator);

		window->menubar.menu_item_quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, window->accel_group);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_file), window->menubar.menu_item_quit);

		g_signal_connect(G_OBJECT(window->menubar.menu_item_new), "activate", G_CALLBACK(cb_sq_main_new_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_open), "activate", G_CALLBACK(cb_sq_main_open_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_close), "activate", G_CALLBACK(cb_sq_main_close_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_quit), "activate", G_CALLBACK(cb_sq_main_close_window), window);
		/* Action menu: ref all the childs*/
		window->menubar.menu_item_action = gtk_menu_item_new_with_mnemonic(_("_Action"));
		window->menubar.menu_action = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(window->menubar.menu_item_action), window->menubar.menu_action);

		tmp_image = sq_main_window_find_image("archive-add.png", GTK_ICON_SIZE_MENU);
		window->menubar.menu_item_add_files = g_object_ref(gtk_image_menu_item_new_with_mnemonic(_("_Add files")));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(window->menubar.menu_item_add_files), tmp_image);
		gtk_widget_set_sensitive(window->menubar.menu_item_add_files, FALSE);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_add_files);

		tmp_image = sq_main_window_find_image("archive-add.png", GTK_ICON_SIZE_MENU);
		window->menubar.menu_item_add_folders = g_object_ref(gtk_image_menu_item_new_with_mnemonic(_("_Add _folders")));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(window->menubar.menu_item_add_folders), tmp_image);
		gtk_widget_set_sensitive(window->menubar.menu_item_add_folders, FALSE);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_add_folders);

		tmp_image = sq_main_window_find_image("archive-extract.png", GTK_ICON_SIZE_MENU);
		window->menubar.menu_item_extract = g_object_ref(gtk_image_menu_item_new_with_mnemonic(_("_Extract")));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(window->menubar.menu_item_extract), tmp_image);
		gtk_widget_set_sensitive(window->menubar.menu_item_extract, FALSE);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_extract);

		window->menubar.menu_item_remove = g_object_ref(gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, window->accel_group));
		gtk_widget_set_sensitive(window->menubar.menu_item_remove, FALSE);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_remove);

		window->menubar.menu_item_refresh = g_object_ref(gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, window->accel_group));
		gtk_widget_set_sensitive(window->menubar.menu_item_refresh, FALSE);
		gtk_container_add(GTK_CONTAINER(window->menubar.menu_action), window->menubar.menu_item_refresh);

		g_signal_connect(G_OBJECT(window->menubar.menu_item_add_files), "activate", G_CALLBACK(cb_sq_main_add_files_to_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_add_folders), "activate", G_CALLBACK(cb_sq_main_add_folders_to_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_extract), "activate", G_CALLBACK(cb_sq_main_extract_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_remove), "activate", G_CALLBACK(cb_sq_main_remove_from_archive), window);
		g_signal_connect(G_OBJECT(window->menubar.menu_item_refresh), "activate", G_CALLBACK(cb_sq_main_refresh_archive), window);

		gtk_widget_add_accelerator (window->menubar.menu_item_refresh,
									"activate",
									window->accel_group,
									GDK_F5,
									0,
									GTK_ACCEL_LOCKED | GTK_ACCEL_MASK);
		gtk_widget_add_accelerator (window->menubar.menu_item_refresh,
									"activate",
									window->accel_group,
									GDK_r,
									GDK_CONTROL_MASK,
									GTK_ACCEL_LOCKED | GTK_ACCEL_MASK);

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
  
		item = gtk_menu_item_new ();
		gtk_widget_set_sensitive (item, FALSE);
		gtk_menu_item_set_right_justified (GTK_MENU_ITEM (item), TRUE);
		gtk_menu_shell_append (GTK_MENU_SHELL (window->menu_bar), item);
		gtk_widget_show (item);

		window->throbber = sq_throbber_new ();
		gtk_container_add (GTK_CONTAINER (item), window->throbber);
		gtk_widget_show (window->throbber);

	}

	show_toolbar = sq_settings_read_bool_entry(window->settings, "ToolBar", TRUE);

	if(show_toolbar)
	{
		window->tool_bar = gtk_toolbar_new();

	/* Archive pane */
		window->toolbar.tool_item_new = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
		window->toolbar.tool_item_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
		tool_separator = gtk_separator_tool_item_new ();

		gtk_container_add(GTK_CONTAINER(window->tool_bar), GTK_WIDGET(window->toolbar.tool_item_new));
		gtk_container_add(GTK_CONTAINER(window->tool_bar), GTK_WIDGET(window->toolbar.tool_item_open));
		gtk_container_add(GTK_CONTAINER(window->tool_bar), GTK_WIDGET(tool_separator));

		g_signal_connect(G_OBJECT(window->toolbar.tool_item_new), "clicked", G_CALLBACK(cb_sq_main_new_archive), window);
		g_signal_connect(G_OBJECT(window->toolbar.tool_item_open), "clicked", G_CALLBACK(cb_sq_main_open_archive), window);

	/* Action pane */
		tmp_image = sq_main_window_find_image("archive-add.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
		window->toolbar.tool_item_add_files = gtk_tool_button_new(tmp_image, _("Add files"));
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add_files), FALSE);

		tmp_image = sq_main_window_find_image("archive-add.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
		window->toolbar.tool_item_add_folders = gtk_tool_button_new(tmp_image, _("Add folders"));
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add_folders), FALSE);

		tmp_image = sq_main_window_find_image("archive-extract.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
		window->toolbar.tool_item_extract = gtk_tool_button_new(tmp_image, _("Extract"));
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);

		window->toolbar.tool_item_remove = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);

		tool_separator = gtk_separator_tool_item_new ();

		gtk_container_add(GTK_CONTAINER(window->tool_bar), GTK_WIDGET(window->toolbar.tool_item_add_files));
		gtk_container_add(GTK_CONTAINER(window->tool_bar), GTK_WIDGET(window->toolbar.tool_item_add_folders));
		gtk_container_add(GTK_CONTAINER(window->tool_bar), GTK_WIDGET(window->toolbar.tool_item_extract));
		gtk_container_add(GTK_CONTAINER(window->tool_bar), GTK_WIDGET(window->toolbar.tool_item_remove));
		gtk_container_add(GTK_CONTAINER(window->tool_bar), GTK_WIDGET(tool_separator));

		g_signal_connect(G_OBJECT(window->toolbar.tool_item_add_files), "clicked", G_CALLBACK(cb_sq_main_add_files_to_archive), window);
		g_signal_connect(G_OBJECT(window->toolbar.tool_item_add_folders), "clicked", G_CALLBACK(cb_sq_main_add_folders_to_archive), window);
		g_signal_connect(G_OBJECT(window->toolbar.tool_item_extract), "clicked", G_CALLBACK(cb_sq_main_extract_archive), window);
		g_signal_connect(G_OBJECT(window->toolbar.tool_item_remove), "clicked", G_CALLBACK(cb_sq_main_remove_from_archive), window);

	/* control pane */

		window->toolbar.tool_item_stop = gtk_tool_button_new_from_stock(GTK_STOCK_STOP);
		gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);

		gtk_container_add(GTK_CONTAINER(window->tool_bar), GTK_WIDGET(window->toolbar.tool_item_stop));

		g_signal_connect(G_OBJECT(window->toolbar.tool_item_stop), "clicked", G_CALLBACK(cb_sq_main_stop_archive), window);
	}

	nav_bar = sq_settings_read_entry(window->settings, "NavigationBar", "None");
	window->nav_style = SQ_MAIN_WINDOW_NAVIGATION_INTERNAL;
	window->navigationbar = NULL;

#ifdef ENABLE_TOOLBAR
	if(!strcmp(nav_bar, "ToolBar"))
	{
		window->nav_style = SQ_MAIN_WINDOW_NAVIGATION_TOOL_BAR;
		window->navigationbar = sq_tool_bar_new(NULL); 
	}
#endif
#ifdef ENABLE_PATHBAR
	if(!strcmp(nav_bar, "PathBar"))
	{
		window->nav_style = SQ_MAIN_WINDOW_NAVIGATION_PATH_BAR;
		window->navigationbar = sq_path_bar_new(NULL);
		gtk_container_set_border_width(GTK_CONTAINER(window->navigationbar), 3);
	}
#endif

	g_object_notify(G_OBJECT(window), "navigation-style");

	use_tabs = sq_settings_read_bool_entry(window->settings, "UseTabs", TRUE);

/* main view */
	window->notebook = sq_notebook_new(window->navigationbar, use_tabs, window->accel_group);
	g_signal_connect(G_OBJECT(window->notebook), "archive-state-changed", G_CALLBACK(cb_sq_main_window_notebook_state_changed), window);
	g_signal_connect(G_OBJECT(window->notebook), "switch-page", G_CALLBACK(cb_sq_main_window_notebook_page_switched), window);
	g_signal_connect(G_OBJECT(window->notebook), "archive-removed", G_CALLBACK(cb_sq_main_window_notebook_page_removed), window);
	g_signal_connect(G_OBJECT(window->notebook), "file-activated", G_CALLBACK(cb_sq_main_window_notebook_file_activated), window);

/* menu item */
	if(show_menubar)
	{
		list = sq_widget_factory_create_property_menu(window->widget_factory, G_OBJECT(window->notebook), "show-full-path");
		for(iter = list; iter; iter = iter->next)
		{
			gtk_container_add(GTK_CONTAINER(window->menubar.menu_view), iter->data);
			gtk_widget_show(iter->data);
		}
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
	}

/* Statusbar */

	window->statusbar = gtk_statusbar_new();

	if(show_menubar)
		gtk_box_pack_start(GTK_BOX(window->main_vbox), window->menu_bar, FALSE, FALSE, 0);

	if(show_toolbar)
		gtk_box_pack_start(GTK_BOX(window->main_vbox), window->tool_bar, FALSE, FALSE, 0);

	if(window->navigationbar)
	{
		gtk_widget_ref(GTK_WIDGET(window->navigationbar));
		gtk_box_pack_start(GTK_BOX(window->main_vbox), GTK_WIDGET(window->navigationbar), FALSE, FALSE, 0);
		gtk_widget_show_all(GTK_WIDGET(window->navigationbar));
	}

	gtk_box_pack_start(GTK_BOX(window->main_vbox), window->notebook, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(window->main_vbox), window->statusbar, FALSE, FALSE, 0);

	gtk_widget_show_all(window->main_vbox);
	if(show_toolbar)
		gtk_widget_show_all(window->tool_bar);
	gtk_widget_show_all(window->notebook);
	gtk_widget_show_all(window->statusbar);

	gtk_container_add(GTK_CONTAINER(window), window->main_vbox);
}

GtkWidget *
sq_main_window_new(SQApplication *app, GtkIconTheme *icon_theme)
{
	SQMainWindow *window;

	window = g_object_new(sq_main_window_get_type(),
			"title", PACKAGE_STRING,
			NULL);

	window->icon_theme = icon_theme;

	sq_notebook_set_icon_theme(SQ_NOTEBOOK(window->notebook), icon_theme);

	g_object_ref(app);
	window->app = app;

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
	GdkPixbuf *file_pixbuf;
	path = g_build_filename(DATADIR, "pixmaps/squeeze", filename, NULL);

	gtk_icon_size_lookup(size, &width, &height);

	file_pixbuf = gdk_pixbuf_new_from_file_at_size(path, width, height, &error);
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
sq_main_window_new_action_menu(SQMainWindow *window, LSQArchive *archive)
{
}

static void
cb_sq_main_new_archive(GtkWidget *widget, gpointer userdata)
{
	GtkWidget *dialog = sq_new_archive_dialog_new();
	GFile *file = NULL;
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	LSQArchive *archive = NULL;
	LSQSupportType support_mask = 0;
	gint result = 0;

	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog) );
		return;
	}
	if(result == GTK_RESPONSE_OK)
	{
		file = sq_new_archive_dialog_get_file(SQ_NEW_ARCHIVE_DIALOG(dialog));
		
		if(!lsq_new_archive(file, TRUE, &archive))
		{
			support_mask = lsq_archive_get_support_mask(archive);
			sq_notebook_add_archive(SQ_NOTEBOOK(window->notebook), archive, TRUE);
			if(support_mask & LSQ_SUPPORT_FILES)
				gtk_widget_set_sensitive(window->menubar.menu_item_add_files, TRUE);
			if(support_mask & LSQ_SUPPORT_FOLDERS)
				gtk_widget_set_sensitive(window->menubar.menu_item_add_folders, TRUE);

			gtk_widget_set_sensitive(window->menubar.menu_item_extract, TRUE);
			gtk_widget_set_sensitive(window->menubar.menu_item_remove, TRUE);
			gtk_widget_set_sensitive(window->menubar.menu_item_refresh, TRUE);

			if(window->tool_bar)
			{
				if(support_mask & LSQ_SUPPORT_FILES)
					gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add_files), TRUE);
				if(support_mask & LSQ_SUPPORT_FOLDERS)
					gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add_folders), TRUE);

				gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), TRUE);

				gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);
			}
		}
		else
		{

		}
        g_object_unref (file);
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

	/*
	GSList *supported_mime_types = lsq_get_supported_mime_types(0);
	GSList *_supported_mime_types = supported_mime_types;
	
	GtkFileFilter *filter_all = gtk_file_filter_new();
	gtk_file_filter_set_name(filter_all, _("Archives"));
	while(_supported_mime_types)
	{
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_add_mime_type(filter,
				lsq_mime_support_get_name((LSQMimeSupport *)(_supported_mime_types->data)));

		gtk_file_filter_set_name(filter, lsq_mime_support_get_comment((LSQMimeSupport *)(_supported_mime_types->data)));
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

		gtk_file_filter_add_mime_type(filter_all,
				lsq_mime_support_get_name((LSQMimeSupport *)(_supported_mime_types->data)));
		_supported_mime_types = g_slist_next(_supported_mime_types);
	}
	g_slist_free(supported_mime_types);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_all);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter_all);
	*/
	

	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_CANCEL || result == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (dialog);
		return;
	}
	if(result == GTK_RESPONSE_OK)
	{
		open_archive_paths = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
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
		g_slist_foreach(open_archive_paths, (GFunc)g_object_unref, NULL);
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

	LSQArchive		*lp_archive = NULL;

  gchar **strv;

	GSList *filenames = sq_notebook_get_selected_items(SQ_NOTEBOOK(window->notebook));

	sq_notebook_get_active_archive(SQ_NOTEBOOK(window->notebook), &lp_archive);

	dialog = sq_extract_archive_dialog_new(lp_archive, g_slist_length(filenames));
	result = gtk_dialog_run (GTK_DIALOG (dialog) );
	if(result == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(dialog);
		extract_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SQ_EXTRACT_ARCHIVE_DIALOG(dialog)->all_files_radio)))
		{
			lsq_iter_slist_free(filenames);
			filenames = NULL;
		}
    strv = lsq_iter_list_to_strv(filenames);
		if(!lsq_archive_operate(lp_archive, LSQ_COMMAND_TYPE_EXTRACT, strv, extract_archive_path))
		{
			GtkWidget *warning_dialog;
      g_strfreev(strv);
			warning_dialog = gtk_message_dialog_new(GTK_WINDOW(window), 
															   GTK_DIALOG_DESTROY_WITH_PARENT, 
																												 GTK_MESSAGE_WARNING,
																												 GTK_BUTTONS_CLOSE,
																												 _("Squeeze cannot extract this archive type,\nthe application to support this is missing."));
			if(warning_dialog)
			{
				gtk_dialog_run (GTK_DIALOG (warning_dialog) );
				gtk_widget_destroy(warning_dialog);
			}
		}
    else
      g_strfreev(strv);
		g_free(extract_archive_path);
		extract_archive_path = NULL;
	}
	gtk_widget_destroy (dialog);

	lsq_iter_slist_free(filenames);
}

static void
cb_sq_main_add_files_to_archive(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);

	LSQArchive		*lp_archive = NULL;
	GtkWidget		 *dialog = NULL;
	GSList			*filenames = NULL;
	gint result;
	sq_notebook_get_active_archive(SQ_NOTEBOOK(window->notebook), &lp_archive);

	dialog = gtk_file_chooser_dialog_new(_("Add files"), 
										 GTK_WINDOW(window),
										 GTK_FILE_CHOOSER_ACTION_OPEN,
										 GTK_STOCK_CANCEL,
										 GTK_RESPONSE_CANCEL,
										 GTK_STOCK_OPEN,
										 GTK_RESPONSE_OK,
										 NULL);

	result = gtk_dialog_run (GTK_DIALOG(dialog));
	if(result == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(dialog);
		filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		if(filenames)
		{
      gchar **strv = lsq_iter_list_to_strv(filenames);
			if(!lsq_archive_operate(lp_archive, LSQ_COMMAND_TYPE_ADD, strv, NULL))
			{
				GtkWidget *warning_dialog;
        g_strfreev(strv);
				warning_dialog = gtk_message_dialog_new(GTK_WINDOW(window), 
																													 GTK_DIALOG_DESTROY_WITH_PARENT, 
																													 GTK_MESSAGE_WARNING,
																													 GTK_BUTTONS_CLOSE,
																													 _("Squeeze cannot add files to this archive type,\nthe application to support this is missing."));
				gtk_dialog_run (GTK_DIALOG (warning_dialog) );
				gtk_widget_destroy(warning_dialog);
			}
      else
        g_strfreev(strv);
		}
	}
	gtk_widget_destroy (dialog);
}

static void
cb_sq_main_add_folders_to_archive(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);

	LSQArchive		*lp_archive = NULL;
	GtkWidget		 *dialog = NULL;
	GSList			*filenames = NULL;
	gint result;
	sq_notebook_get_active_archive(SQ_NOTEBOOK(window->notebook), &lp_archive);

	dialog = gtk_file_chooser_dialog_new(_("Add folders"),
										 GTK_WINDOW(window),
										 GTK_FILE_CHOOSER_ACTION_OPEN,
										 GTK_STOCK_CANCEL,
										 GTK_RESPONSE_CANCEL,
										 GTK_STOCK_OPEN,
										 GTK_RESPONSE_OK,
										 NULL);

	result = gtk_dialog_run (GTK_DIALOG(dialog));
	if(result == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(dialog);
		filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		if(filenames)
		{
      gchar **strv = lsq_iter_list_to_strv(filenames);
			if(!lsq_archive_operate(lp_archive, LSQ_COMMAND_TYPE_ADD, strv, NULL))
			{
				GtkWidget *warning_dialog;
        g_strfreev(strv);
				warning_dialog = gtk_message_dialog_new(GTK_WINDOW(window),
									  GTK_DIALOG_DESTROY_WITH_PARENT, 
									  GTK_MESSAGE_WARNING,
									  GTK_BUTTONS_CLOSE,
									  _("Squeeze cannot add folders to this archive type,\n"
										"the application to support this is missing."));
				gtk_dialog_run (GTK_DIALOG (warning_dialog) );
				gtk_widget_destroy(warning_dialog);
			}
      else
        g_strfreev(strv);
		}
	}
	gtk_widget_destroy (dialog);
}

static void
cb_sq_main_remove_from_archive(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	LSQArchive		*lp_archive = NULL;
	GtkWidget *dialog = NULL;
	gint result = 0;
  gchar **strv;
	GSList *filenames = sq_notebook_get_selected_items(SQ_NOTEBOOK(window->notebook));

	if(filenames)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, _("Are you sure you want to remove the selected files?"));
		result = gtk_dialog_run(GTK_DIALOG(dialog));
		if(result == GTK_RESPONSE_YES)
		{
			gtk_widget_hide(dialog);
			sq_notebook_get_active_archive(SQ_NOTEBOOK(window->notebook), &lp_archive);
			/* gtk_tree_view_set_model(sq_notebook_get_active_tree_view(SQ_NOTEBOOK(window->notebook)), NULL); */
      strv = lsq_iter_list_to_strv(filenames);
			if(!lsq_archive_operate(lp_archive, LSQ_COMMAND_TYPE_REMOVE, strv, NULL))
			{
				GtkWidget *warning_dialog;
        g_strfreev(strv);
				warning_dialog = gtk_message_dialog_new(GTK_WINDOW(window), 
																													 GTK_DIALOG_DESTROY_WITH_PARENT, 
																													 GTK_MESSAGE_WARNING,
																													 GTK_BUTTONS_CLOSE,
																													 _("Squeeze cannot remove files from this archive type,\nthe application to support this is missing."));
				gtk_dialog_run (GTK_DIALOG (warning_dialog) );
				gtk_widget_destroy(warning_dialog);
			}
      else
        g_strfreev(strv);
		}
		gtk_widget_destroy (dialog);

		lsq_iter_slist_free(filenames);
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
cb_sq_main_refresh_archive(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	SQArchiveStore *store = sq_notebook_get_active_store(SQ_NOTEBOOK(window->notebook));
	LSQArchive *archive = sq_archive_store_get_archive(store);
	lsq_archive_operate(archive, LSQ_COMMAND_TYPE_REFRESH, NULL, NULL);
}

static void
cb_sq_main_stop_archive(GtkWidget *widget, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);

	LSQArchive		*lp_archive = NULL;
	GtkWidget		 *dialog = NULL;
	gint result = 0;
	sq_notebook_get_active_archive(SQ_NOTEBOOK(window->notebook), &lp_archive);
	
	if(lsq_archive_can_stop(lp_archive))
		lsq_archive_stop(lp_archive);
	else
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, 
										_("Are you sure you want to cancel this operation?\nThis could damage the archive."));
		result = gtk_dialog_run(GTK_DIALOG(dialog));
		if(result == GTK_RESPONSE_YES)
		{
			gtk_widget_hide(dialog);
			lsq_archive_stop(lp_archive);
		}
		gtk_widget_destroy(dialog);
	}
}

/*
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
*/


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
	const gchar *artists[] = {
		_("Application Icon:"),
		"Stephan Arts <stephan@xfce.org>",
		"",
		_("Add / Extract icons:"),
		_("Based on the original extract icon created by Andreas Nilsson"),
		NULL
	};
	GtkWidget *about_dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_name((GtkAboutDialog *)about_dialog, PACKAGE_NAME);
	gtk_about_dialog_set_version((GtkAboutDialog *)about_dialog, PACKAGE_VERSION);
	gtk_about_dialog_set_comments((GtkAboutDialog *)about_dialog, _("Squeeze is a lightweight and flexible archive manager for the Xfce Desktop Environment"));
	gtk_about_dialog_set_website((GtkAboutDialog *)about_dialog, "http://squeeze.xfce.org");

	gtk_about_dialog_set_logo_icon_name((GtkAboutDialog *)about_dialog, "squeeze");

	gtk_about_dialog_set_authors((GtkAboutDialog *)about_dialog, authors);

	/* Translator credits as shown in the about dialog: NAME <E-MAIL> YEAR */
	gtk_about_dialog_set_translator_credits((GtkAboutDialog *)about_dialog, _("translator-credits"));

	gtk_about_dialog_set_license((GtkAboutDialog *)about_dialog, xfce_get_license_text(XFCE_LICENSE_TEXT_GPL));

	gtk_about_dialog_set_artists((GtkAboutDialog *)about_dialog, artists);

	gtk_about_dialog_set_copyright((GtkAboutDialog *)about_dialog, "Copyright \302\251 2006-2007 Stephan Arts");

	gtk_dialog_run(GTK_DIALOG(about_dialog));

	gtk_widget_destroy(about_dialog);

}

static void
cb_sq_main_window_notebook_page_switched(SQNotebook *notebook, GtkNotebookPage *page, guint page_nr, gpointer data)
{
	LSQArchive *lp_archive;
	SQMainWindow *window = SQ_MAIN_WINDOW(data);
	guint context_id;
	const gchar *message;
	gchar *filename;
	sq_notebook_page_get_archive(notebook, &lp_archive, page_nr);

    filename = lsq_archive_get_filename(lp_archive);
	gtk_window_set_title(GTK_WINDOW(window), g_strconcat(PACKAGE_NAME, " - ", filename , NULL));
    g_free (filename);

	sq_main_window_new_action_menu(window, lp_archive);

	context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(window->statusbar), "Window Statusbar");
	message = lsq_archive_get_state_msg(lp_archive);
	if(!message)
	{
		message = _("Done");
		if(window->menu_bar)
		{
			sq_throbber_set_animated(SQ_THROBBER(window->throbber), FALSE);
			/* FIXME: */
			/* gtk_widget_set_sensitive(window->menubar.menu_item_add, TRUE); */
			gtk_widget_set_sensitive(window->menubar.menu_item_extract, TRUE);
			gtk_widget_set_sensitive(window->menubar.menu_item_remove, TRUE);
			gtk_widget_set_sensitive(window->menubar.menu_item_refresh, TRUE);
		}
		if(window->tool_bar)
		{
			/* gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), TRUE); */
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);
		}
	}
	else
	{
		if(window->menu_bar)
		{
			sq_throbber_set_animated(SQ_THROBBER(window->throbber), TRUE);
			/* FIXME: */
			/* gtk_widget_set_sensitive(window->menubar.menu_item_add, FALSE); */
			gtk_widget_set_sensitive(window->menubar.menu_item_extract, FALSE);
			gtk_widget_set_sensitive(window->menubar.menu_item_remove, FALSE);
			gtk_widget_set_sensitive(window->menubar.menu_item_refresh, FALSE);
		}

		if(window->tool_bar)
		{
			/* gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), FALSE); */
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), TRUE);
		}

	}
	gtk_statusbar_push(GTK_STATUSBAR(window->statusbar), context_id, message);
}

static void
cb_sq_main_window_notebook_page_removed(SQNotebook *notebook, gpointer data)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(data);
	guint context_id;

	if(!gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
	{
		if(window->menu_bar)
		{
			gtk_widget_set_sensitive(window->menubar.menu_item_close, FALSE);
			/*gtk_widget_set_sensitive(window->menubar.menu_item_properties, FALSE);*/

			/* FIXME: */
			/*gtk_widget_set_sensitive(window->menubar.menu_item_add, FALSE);*/
			gtk_widget_set_sensitive(window->menubar.menu_item_extract, FALSE);
			gtk_widget_set_sensitive(window->menubar.menu_item_remove, FALSE);
			gtk_widget_set_sensitive(window->menubar.menu_item_refresh, FALSE);
		}

		if(window->tool_bar)
		{
			/* gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add), FALSE); */
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);
		}

		gtk_window_set_title(GTK_WINDOW(window), PACKAGE_STRING);
		context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(window->statusbar), "Window Statusbar");
		gtk_statusbar_push(GTK_STATUSBAR(window->statusbar), context_id, _("Done"));
	}
}

static void
cb_sq_main_window_notebook_file_activated(SQNotebook *notebook, LSQArchiveIter *iter, gpointer data)
{
	GtkWindow *window = GTK_WINDOW(data);
	LSQArchive *lp_archive = NULL;
	gchar *extract_archive_path = NULL;
	GtkWidget *label = gtk_label_new(_("Which action do you want to perform on the selected file(s)?"));
	GtkWidget *dialog = gtk_dialog_new_with_buttons("",window,GTK_DIALOG_DESTROY_WITH_PARENT, _("Open"), GTK_RESPONSE_OK, _("Extract"), GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	GtkWidget *extr_dialog = NULL;
	gint result;
	gchar **strv;
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 20);
	gtk_widget_show(label);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);

	strv = g_new(gchar *, 2);
  strv[0] = lsq_archive_iter_get_path(iter);
  strv[1] = NULL;

	switch(result)
	{
		case GTK_RESPONSE_OK: /* VIEW */
			sq_notebook_get_active_archive(SQ_NOTEBOOK(notebook), &lp_archive);
			if(lsq_archive_operate(lp_archive, LSQ_COMMAND_TYPE_OPEN, strv, NULL))
			{
				GtkWidget *warning_dialog = gtk_message_dialog_new(window, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, _("Squeeze cannot view this file.\nthe application to support this is missing."));
				if(warning_dialog)
				{
					gtk_dialog_run(GTK_DIALOG(warning_dialog));
					gtk_widget_destroy(warning_dialog);
				}
			}
			break;
		case GTK_RESPONSE_ACCEPT: /* EXTRACT */
			sq_notebook_get_active_archive(SQ_NOTEBOOK(notebook), &lp_archive);
			extr_dialog = sq_extract_archive_dialog_new(lp_archive, 1);
			result = gtk_dialog_run (GTK_DIALOG (extr_dialog) );
			if(result == GTK_RESPONSE_OK)
			{
				gtk_widget_hide(extr_dialog);
				extract_archive_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(extr_dialog));
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(SQ_EXTRACT_ARCHIVE_DIALOG(extr_dialog)->all_files_radio)))
				{
					g_strfreev(strv);
					strv = NULL;
				}
				if(lsq_archive_operate(lp_archive, LSQ_COMMAND_TYPE_EXTRACT, strv, extract_archive_path))
				{
					GtkWidget *warning_dialog = gtk_message_dialog_new(GTK_WINDOW(window), 
																	 GTK_DIALOG_DESTROY_WITH_PARENT, 
																	 GTK_MESSAGE_WARNING,
																	 GTK_BUTTONS_CLOSE,
																	 _("Squeeze cannot extract this archive type,\nthe application to support this is missing."));
					gtk_dialog_run (GTK_DIALOG (warning_dialog) );
					gtk_widget_destroy(warning_dialog);

				}
				g_free(extract_archive_path);
				extract_archive_path = NULL;
			}
			gtk_widget_destroy (extr_dialog);
			
			break;
		case GTK_RESPONSE_CANCEL: /* CANCEL */
			break;
	}
	g_strfreev(strv);
	gtk_widget_destroy(dialog);
}

gint
sq_main_window_open_archive(SQMainWindow *window, GFile *file, gint replace)
{
	LSQArchive *archive = NULL;

	if(!lsq_open_archive(file, &archive))
	{
		if(replace < 0)
			sq_notebook_add_archive(SQ_NOTEBOOK(window->notebook), archive, FALSE);
		else
			sq_notebook_page_set_archive(SQ_NOTEBOOK(window->notebook), archive, replace);
		gtk_widget_set_sensitive(window->menubar.menu_item_close, TRUE);

		/*gtk_widget_set_sensitive(window->menubar.menu_item_properties, TRUE);*/
		return 0;
	}
	else
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), 
				GTK_DIALOG_DESTROY_WITH_PARENT, 
				GTK_MESSAGE_ERROR, 
				GTK_BUTTONS_OK,
				_("Failed to open file"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), _("'%s'\nCould not be opened"), g_file_get_path (file));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	return 1;
}

static void
sq_main_window_set_navigation(SQMainWindow *window)
{
	SQNavigationBar *nav_bar = NULL; 

	switch(window->nav_style)
	{
		case SQ_MAIN_WINDOW_NAVIGATION_INTERNAL:
			break;
#ifdef ENABLE_TOOLBAR
		case SQ_MAIN_WINDOW_NAVIGATION_TOOL_BAR:
			nav_bar = sq_tool_bar_new(NULL);
			break;
#endif
#ifdef ENABLE_PATHBAR
		case SQ_MAIN_WINDOW_NAVIGATION_PATH_BAR:
			nav_bar = sq_path_bar_new(NULL);
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
		if(show_toolbar)
			gtk_box_reorder_child(GTK_BOX(window->main_vbox), (GtkWidget *)nav_bar, 2);
		else
			gtk_box_reorder_child(GTK_BOX(window->main_vbox), (GtkWidget *)nav_bar, 1);
		gtk_widget_show_all((GtkWidget *)nav_bar);
	}	
}

static void
cb_sq_main_window_notebook_state_changed(SQNotebook *notebook, LSQArchive *archive, gpointer userdata)
{
	SQMainWindow *window = SQ_MAIN_WINDOW(userdata);
	LSQSupportType support_mask = lsq_archive_get_support_mask(archive);

	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(window->statusbar), "Window Statusbar");
	const gchar *message = lsq_archive_get_state_msg(archive);
	if(!message)
	{
		sq_throbber_set_animated(SQ_THROBBER(window->throbber), FALSE);
		message = _("Done");
		if(window->menu_bar)
		{
			if(support_mask & LSQ_SUPPORT_FILES)
				gtk_widget_set_sensitive(window->menubar.menu_item_add_files, TRUE);
			if(support_mask & LSQ_SUPPORT_FOLDERS)
				gtk_widget_set_sensitive(window->menubar.menu_item_add_folders, TRUE);
			gtk_widget_set_sensitive(window->menubar.menu_item_extract, TRUE);
			gtk_widget_set_sensitive(window->menubar.menu_item_remove, TRUE);
			gtk_widget_set_sensitive(window->menubar.menu_item_refresh, TRUE);
		}

		if(window->tool_bar)
		{
			if(support_mask & LSQ_SUPPORT_FILES)
				gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add_files), TRUE);
			if(support_mask & LSQ_SUPPORT_FOLDERS)
				gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add_folders), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), TRUE);

			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), FALSE);
		}
	}
	else
	{

		if(window->menu_bar)
		{
			sq_throbber_set_animated(SQ_THROBBER(window->throbber), TRUE);
			gtk_widget_set_sensitive(window->menubar.menu_item_add_files, FALSE);
			gtk_widget_set_sensitive(window->menubar.menu_item_add_folders, FALSE);
			gtk_widget_set_sensitive(window->menubar.menu_item_extract, FALSE);
			gtk_widget_set_sensitive(window->menubar.menu_item_remove, FALSE);
			gtk_widget_set_sensitive(window->menubar.menu_item_refresh, FALSE);
		}

		if(window->tool_bar)
		{
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add_files), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_add_folders), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_extract), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_remove), FALSE);

			gtk_widget_set_sensitive(GTK_WIDGET(window->toolbar.tool_item_stop), TRUE);
		}
	}

	gtk_statusbar_push(GTK_STATUSBAR(window->statusbar), context_id, message);
}
