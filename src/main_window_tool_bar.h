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

#ifndef __XARCHIVER_MAIN_WINDOW_TOOLBAR_H__
#define __XARCHIVER_MAIN_WINDOW_TOOLBAR_H__
G_BEGIN_DECLS


#define XA_MAIN_WINDOW_TOOLBAR(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			xa_main_window_tool_bar_get_type(),      \
			XAMainWindowToolBar))

#define XA_IS_MAIN_WINDOW_TOOLBAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_main_window_tool_bar_get_type()))

#define XA_MAIN_WINDOW_TOOLBAR_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			xa_main_window_tool_bar_get_type(),      \
			XAMainWindowToolBarClass))

#define XA_IS_MAIN_WINDOW_TOOLBAR_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			xa_main_window_tool_bar_get_type()))

typedef struct _XAMainWindowToolBar XAMainWindowToolBar;

struct _XAMainWindowToolBar
{
	GtkToolbar parent;
	/* contents of 'archive' pane */
	GtkToolItem *tool_item_new;
	GtkToolItem *tool_item_open;

	/* contents of 'action' pane */
	GtkToolItem *tool_item_add;
	GtkToolItem *tool_item_extract;
	GtkToolItem *tool_item_remove;

};

typedef struct _XAMainWindowToolBarClass XAMainWindowToolBarClass;

struct _XAMainWindowToolBarClass
{
	GtkToolbarClass parent;
};

GtkWidget *xa_main_window_tool_bar_new();

gboolean xa_main_window_tool_bar_set_status(XAMainWindowToolBar *, XAMainWindowStatus);

G_END_DECLS
#endif /* __XARCHIVER_MAIN_WINDOW_TOOLBAR_H__ */
