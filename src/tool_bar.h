/*
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

#ifndef __SQRCHIVER_TOOL_BAR_H__
#define __SQRCHIVER_TOOL_BAR_H__
G_BEGIN_DECLS

#define SQ_TYPE_TOOL_BAR sq_tool_bar_get_type()

#define SQ_TOOL_BAR(obj)(                \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
			SQ_TYPE_TOOL_BAR,                  \
			SQToolBar))

#define SQ_IS_TOOL_BAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			SQ_TYPE_TOOL_BAR))

#define SQ_TOOL_BAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			SQ_TYPE_TOOL_BAR,      \
			SQToolBarClass))

#define SQ_IS_TOOL_BAR_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			SQ_TYPE_TOOL_BAR()))	

typedef struct _SQToolBar SQToolBar;

struct _SQToolBar
{
	SQNavigationBar parent;
	GtkToolbar *bar;
	GtkToolItem *back_button;
	GtkToolItem *forward_button;
	GtkToolItem *up_button;
	GtkToolItem *home_button;
	GtkWidget *path_field;
	GtkWidget *hbox;
};

typedef struct _SQToolBarClass SQToolBarClass;

struct _SQToolBarClass
{
	SQNavigationBarClass parent_class;
};

GType            sq_tool_bar_get_type();
SQNavigationBar *sq_tool_bar_new();

G_END_DECLS
#endif /* __SQRCHIVER_TOOL_BAR_H__*/
