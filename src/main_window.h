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

#ifndef __XARCHIVER_MAIN_WINDOW_H__
#define __XARCHIVER_MAIN_WINDOW_H__
G_BEGIN_DECLS

typedef enum {
	XA_MAIN_WINDOW_STATUS_NONE,
	XA_MAIN_WINDOW_STATUS_IDLE,
	XA_MAIN_WINDOW_STATUS_BUSY
}XAMainWindowStatus;

#define XA_TYPE_MAIN_WINDOW xa_main_window_get_type()

#define XA_MAIN_WINDOW(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			xa_main_window_get_type(),      \
			XAMainWindow))

#define XA_IS_MAIN_WINDOW(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_main_window_get_type()))

#define XA_MAIN_WINDOW_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			xa_main_window_get_type(),      \
			XAMainWindowClass))

#define XA_IS_MAIN_WINDOW_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			xa_main_window_get_type()))

typedef struct _XAMainWindow XAMainWindow;

struct _XAMainWindow
{
	GtkWindow parent;
	XASettings *settings;
	GtkIconTheme *icon_theme;
	XAApplication *app;
	GtkWidget *menu_bar;
	GtkWidget *main_vbox;
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
			GtkWidget *menu_item_add;
			GtkWidget *menu_item_extract;
			GtkWidget *menu_item_remove;
		/***************************/

		/* 'view' menu */
		GtkWidget *menu_item_view;
		GtkWidget *menu_view;
		/* contents of 'view' menu */
			GtkWidget *menu_item_nav_bar;
			GtkWidget *menu_nav_bar;
			/* contents of 'nav_bar' submenu */
				GtkWidget *menu_item_tool_bar;
				GtkWidget *menu_item_path_bar;
				GtkWidget *menu_item_no_bar;
			/*********************************/
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
		GtkToolItem *tool_item_add;
		GtkToolItem *tool_item_extract;
		GtkToolItem *tool_item_remove;
		GtkToolItem *tool_item_stop;
	} toolbar;
	XANavigationBar *navigationbar;
	GtkAccelGroup *accel_group;
	GtkWidget *notebook;
	GtkWidget *statusbar;
	GtkWidget *about_dlg;
};

typedef struct _XAMainWindowClass XAMainWindowClass;

struct _XAMainWindowClass
{
	GtkWindowClass parent;
};

GtkWidget *xa_main_window_new(XAApplication *, GtkIconTheme *icon_theme);
GtkWidget *xa_main_window_find_image(gchar *, GtkIconSize);
GType      xa_main_window_get_type ();

gint xa_main_window_open_archive(XAMainWindow *window, gchar *path, gint replace);


G_END_DECLS
#endif /* __XARCHIVER_MAIN_WINDOW_H__ */
