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

#ifndef __SQRCHIVER_NOTEBOOK_H__
#define __SQRCHIVER_NOTEBOOK_H__
G_BEGIN_DECLS

#define SQ_TYPE_NOTEBOOK sq_notebook_get_type()

#define SQ_NOTEBOOK(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			sq_notebook_get_type(),      \
			SQNotebook))

#define SQ_IS_NOTEBOOK(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			sq_notebook_get_type()))

#define SQ_NOTEBOOK_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			sq_notebook_get_type(),      \
			SQNotebookClass))

#define SQ_IS_NOTEBOOK_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			sq_notebook_get_type()))

typedef struct _SQNotebook SQNotebook;

struct _SQNotebook
{
	GtkNotebook parent;
	SQNavigationBar *navigation_bar;
	gboolean multi_tab;
	struct
	{
		gboolean _show_icons;
		gboolean _up_dir;
	} props;
	GtkIconTheme *icon_theme;
	GtkAccelGroup *accel_group;
	GtkTooltips *tool_tips;
};

typedef struct _SQNotebookClass SQNotebookClass;

struct _SQNotebookClass
{
	GtkNotebookClass parent;
};

GtkWidget  *sq_notebook_new (SQNavigationBar *, gboolean , GtkAccelGroup *);
GType       sq_notebook_get_type ();

void        sq_notebook_set_navigation_bar(SQNotebook *, SQNavigationBar *);
void        sq_notebook_add_archive(SQNotebook *, LSQArchive *, LSQArchiveSupport *);
void        sq_notebook_page_set_archive(SQNotebook *, LSQArchive *, LSQArchiveSupport *, gint n);
void        sq_notebook_page_get_archive(SQNotebook *, LSQArchive **, LSQArchiveSupport **, gint n);
void        sq_notebook_set_icon_theme(SQNotebook *, GtkIconTheme *);
void        sq_notebook_get_active_archive(SQNotebook *, LSQArchive **, LSQArchiveSupport **);
gboolean    sq_notebook_is_active_archive(SQNotebook *, LSQArchive *);
GtkWidget  *sq_notebook_get_active_child(SQNotebook *notebook);
gboolean    sq_notebook_get_multi_tab(SQNotebook *notebook);
void        sq_notebook_close_active_archive(SQNotebook *);

GSList     *sq_notebook_get_selected_items(SQNotebook *notebook);

G_END_DECLS
#endif /* __SQRCHIVER_NOTEBOOK_H__ */
