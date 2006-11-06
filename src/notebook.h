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

#ifndef __XARCHIVER_NOTEBOOK_H__
#define __XARCHIVER_NOTEBOOK_H__
G_BEGIN_DECLS

#define XA_TYPE_NOTEBOOK xa_notebook_get_type()

#define XA_NOTEBOOK(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			xa_notebook_get_type(),      \
			XANotebook))

#define XA_IS_NOTEBOOK(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_notebook_get_type()))

#define XA_NOTEBOOK_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			xa_notebook_get_type(),      \
			XANotebookClass))

#define XA_IS_NOTEBOOK_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			xa_notebook_get_type()))

typedef struct _XANotebook XANotebook;

struct _XANotebook
{
	GtkNotebook parent;
	XANavigationBar *navigation_bar;
	gboolean multi_tab;
	struct
	{
		gboolean _show_icons;
		gboolean _up_dir;
	} props;
	GtkIconTheme *icon_theme;
	GtkTooltips *tool_tips;
};

typedef struct _XANotebookClass XANotebookClass;

struct _XANotebookClass
{
	GtkNotebookClass parent;
};

GtkWidget   *xa_notebook_new (XANavigationBar *bar);
GType xa_notebook_get_type ();

void  xa_notebook_set_navigation_bar(XANotebook *, XANavigationBar *);
void  xa_notebook_add_archive(XANotebook *, LXAArchive *, LXAArchiveSupport *);
void xa_notebook_page_set_archive(XANotebook *, LXAArchive *, LXAArchiveSupport *, gint n);
void  xa_notebook_set_icon_theme(XANotebook *, GtkIconTheme *);
void  xa_notebook_get_active_archive(XANotebook *, LXAArchive **, LXAArchiveSupport **);
GtkWidget * xa_notebook_get_active_child(XANotebook *notebook);
gboolean    xa_notebook_get_multi_tab(XANotebook *notebook);

G_END_DECLS
#endif /* __XARCHIVER_NOTEBOOK_H__ */
