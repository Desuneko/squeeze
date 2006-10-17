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

#ifndef __XARCHIVER_TOOL_BAR_H__
#define __XARCHIVER_TOOL_BAR_H__
G_BEGIN_DECLS

#define XA_TYPE_TOOL_BAR xa_tool_bar_get_type()

#define XA_TOOL_BAR(obj)(                \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
			XA_TYPE_TOOL_BAR,                  \
			XAToolBar))

#define XA_IS_TOOL_BAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			XA_TYPE_TOOL_BAR))

#define XA_TOOL_BAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			XA_TYPE_TOOL_BAR,      \
			XAToolBarClass))

#define XA_IS_TOOL_BAR_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			XA_TYPE_TOOL_BAR()))	

typedef struct _XAToolBar XAToolBar;

struct _XAToolBar
{
	XANavigationBar parent;
	GtkToolbar *bar;
	GtkToolItem *back_button;
	GtkToolItem *forward_button;
	GtkToolItem *up_button;
	GtkToolItem *home_button;
	GtkWidget *path_field;
	GtkWidget *hbox;
};

typedef struct _XAToolBarClass XAToolBarClass;

struct _XAToolBarClass
{
	XANavigationBarClass parent_class;
};

GType      xa_tool_bar_get_type();
XANavigationBar *xa_tool_bar_new();

G_END_DECLS
#endif /* __XARCHIVER_TOOL_BAR_H__*/
