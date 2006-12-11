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

#ifndef __SQRCHIVER_APPLICATION_H__
#define __SQRCHIVER_APPLICATION_H__
G_BEGIN_DECLS

#define SQ_TYPE_APPLICATION sq_application_get_type()

#define SQ_APPLICATION(obj)(                \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
			SQ_TYPE_APPLICATION,                  \
			SQApplication))

#define SQ_IS_APPLICATION(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			SQ_TYPE_APPLICATION))

#define SQ_APPLICATION_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			SQ_TYPE_APPLICATION,      \
			SQApplicationClass))

#define SQ_IS_APPLICATION_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			SQ_TYPE_APPLICATION()))	

typedef struct _SQApplication SQApplication;

struct _SQApplication
{
	GObject parent;
	GtkIconTheme *icon_theme;
	SQSettings *settings;
	struct {
		gboolean _tabs;
	} props;
};

typedef struct _SQApplicationClass SQApplicationClass;

struct _SQApplicationClass
{
	GObjectClass parent_class;
};

GType      sq_application_get_type();
SQApplication *sq_application_new(GtkIconTheme *icon_theme);

GtkWidget *sq_application_new_window(SQApplication *);

gint sq_application_extract_archive(SQApplication *, gchar *, gchar *);
gint sq_application_new_archive(SQApplication *, gchar *, GSList *);

gint sq_application_open_archive(SQApplication *, GtkWidget *, gchar *);

G_END_DECLS
#endif /* __SQRCHIVER_APPLICATION_H__*/
