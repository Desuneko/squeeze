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

#ifndef __SQRCHIVER_PATH_BAR_H__
#define __SQRCHIVER_PATH_BAR_H__
G_BEGIN_DECLS

#define SQ_TYPE_PATH_BAR sq_path_bar_get_type()

#define SQ_PATH_BAR(obj)(                \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
			SQ_TYPE_PATH_BAR,                  \
			SQPathBar))

#define SQ_IS_PATH_BAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			SQ_TYPE_PATH_BAR))

#define SQ_PATH_BAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			SQ_TYPE_PATH_BAR,      \
			SQPathBarClass))

#define SQ_IS_PATH_BAR_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			SQ_TYPE_PATH_BAR()))	

typedef struct _SQPathBar SQPathBar;

struct _SQPathBar
{
	SQNavigationBar parent;
	GtkButton *left_button;
	GtkButton *right_button;
	GtkButton *home_button;
	GSList *path_button;
	GSList *first_button;
	guint scroll_timeout;
	guint scroll_dir;
	gboolean scroll_click;
	gboolean updating;
};

typedef struct _SQPathBarClass SQPathBarClass;

struct _SQPathBarClass
{
	SQNavigationBarClass parent_class;
};

GType      sq_path_bar_get_type();
SQNavigationBar *sq_path_bar_new(SQArchiveStore *);

G_END_DECLS
#endif /* __SQRCHIVER_PATH_BAR_H__*/
