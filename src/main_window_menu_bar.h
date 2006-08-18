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

#ifndef __XARCHIVER_MAIN_WINDOW_MENU_BAR_H__
#define __XARCHIVER_MAIN_WINDOW_MENU_BAR_H__
G_BEGIN_DECLS

#define XA_MAIN_WINDOW_MENU_BAR(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			xa_main_window_menu_bar_get_type(),      \
			XAMainWindowMenuBar))

#define XA_IS_MAIN_WINDOW_MENU_BAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_main_window_menu_bar_get_type()))

#define XA_MAIN_WINDOW_MENU_BAR_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			xa_main_window_menu_bar_get_type(),      \
			XAMainWindowMenuBarClass))

#define XA_IS_MAIN_WINDOW_MENU_BAR_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			xa_main_window_menu_bar_get_type()))

typedef struct _XAMainWindowMenuBar XAMainWindowMenuBar;

struct _XAMainWindowMenuBar
{
	GtkMenuBar parent;
	GtkWidget *menu_item_archive;
	GtkWidget *menu_archive;
	/* contents of 'archive' menu */
	GtkWidget *menu_item_new;
	GtkWidget *menu_item_open;
	GtkWidget *menu_item_quit;

	GtkWidget *menu_item_action;
	GtkWidget *menu_action;

	/* contents of 'action' menu */
	GtkWidget *menu_item_add;
	GtkWidget *menu_item_extract;
	GtkWidget *menu_item_remove;

	GtkWidget *menu_item_help;
	GtkWidget *menu_help;
	/* contents of 'help' menu */
	GtkWidget *menu_item_about;

};

typedef struct _XAMainWindowMenuBarClass XAMainWindowMenuBarClass;

struct _XAMainWindowMenuBarClass
{
	GtkMenuBarClass parent;
};

GtkWidget *xa_main_window_menu_bar_new();

G_END_DECLS
#endif /* __XARCHIVER_MAIN_WINDOW_MENU_BAR_H__ */
