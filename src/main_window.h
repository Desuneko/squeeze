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

#ifndef __SQRCHIVER_MAIN_WINDOW_H__
#define __SQRCHIVER_MAIN_WINDOW_H__
G_BEGIN_DECLS

typedef enum {
	SQ_MAIN_WINDOW_STATUS_NONE,
	SQ_MAIN_WINDOW_STATUS_IDLE,
	SQ_MAIN_WINDOW_STATUS_BUSY
}SQMainWindowStatus;

typedef enum
{
	SQ_MAIN_WINDOW_NAVIGATION_INTERNAL,
#ifdef ENABLE_TOOLBAR
	SQ_MAIN_WINDOW_NAVIGATION_TOOL_BAR,
#endif
#ifdef ENABLE_PATHBAR
	SQ_MAIN_WINDOW_NAVIGATION_PATH_BAR
#endif
} SQMainWindowNavigationStyle;

#define SQ_TYPE_MAIN_WINDOW_NAVIGATION_STYLE (sq_main_window_navigation_style_get_type())


#define SQ_TYPE_MAIN_WINDOW sq_main_window_get_type()

#define SQ_MAIN_WINDOW(obj)		 ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),	\
			sq_main_window_get_type(),	  \
			SQMainWindow))

#define SQ_IS_MAIN_WINDOW(obj)	  ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),	\
			sq_main_window_get_type()))

#define SQ_MAIN_WINDOW_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),	 \
			sq_main_window_get_type(),	  \
			SQMainWindowClass))

#define SQ_IS_MAIN_WINDOW_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),		\
			sq_main_window_get_type()))

typedef struct _SQMainWindow SQMainWindow;

struct _SQMainWindow
{
	GtkWindow parent;
	SQSettings *settings;
	GtkIconTheme *icon_theme;
	SQApplication *app;
	GtkWidget *menu_bar;
	GtkWidget *tool_bar;
	GtkWidget *main_vbox;
	SQWidgetFactory *widget_factory;
	GtkWidget *throbber;
	struct {
		/* 'file' menu */
		GtkWidget *menu_item_file;
		GtkWidget *menu_file;
		/* contents of 'file' menu */
			GtkWidget *menu_item_new;
			GtkWidget *menu_item_open;
			GtkWidget *menu_item_properties;
			GtkWidget *menu_item_close;
			GtkWidget *menu_item_quit;
		/***************************/


		/* 'action' menu */
		GtkWidget *menu_item_action;
		GtkWidget *menu_action;
		/* contents of 'action' menu */
			GtkWidget *menu_item_add_files;
			GtkWidget *menu_item_add_folders;
			GtkWidget *menu_item_extract;
			GtkWidget *menu_item_remove;
			GtkWidget *menu_item_refresh;
		/***************************/

		/* 'view' menu */
		GtkWidget *menu_item_view;
		GtkWidget *menu_view;
		/* contents of 'view' menu */
			GtkWidget *menu_item_settings;
		/***************************/

		/* 'help' menu */
		GtkWidget *menu_item_help;
		GtkWidget *menu_help;
		/* contents of 'help' menu */
		GtkWidget *menu_item_about;

	} menubar;
	struct {
		/* contents of 'archive' pane */
		GtkToolItem *tool_item_new;
		GtkToolItem *tool_item_open;

		/* contents of 'action' pane */
		GtkToolItem *tool_item_add_files;
		GtkToolItem *tool_item_add_folders;
		GtkToolItem *tool_item_extract;
		GtkToolItem *tool_item_remove;
		GtkToolItem *tool_item_stop;
	} toolbar;
	SQMainWindowNavigationStyle nav_style;
	SQNavigationBar *navigationbar;
	GtkAccelGroup *accel_group;
	GtkWidget *notebook;
	GtkWidget *statusbar;
	GtkWidget *about_dlg;
};

typedef struct _SQMainWindowClass SQMainWindowClass;

struct _SQMainWindowClass
{
	GtkWindowClass parent;
};

GType	  sq_main_window_navigation_style_get_type();

GtkWidget *sq_main_window_new(SQApplication *, GtkIconTheme *icon_theme);
GtkWidget *sq_main_window_find_image(gchar *, GtkIconSize);
GType	  sq_main_window_get_type ();

gint sq_main_window_open_archive(SQMainWindow *window, GFile *file, gint replace);


G_END_DECLS
#endif /* __SQRCHIVER_MAIN_WINDOW_H__ */
