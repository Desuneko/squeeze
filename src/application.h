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

#ifndef __XARCHIVER_APPLICATION_H__
#define __XARCHIVER_APPLICATION_H__
G_BEGIN_DECLS

#define XA_TYPE_APPLICATION xa_application_get_type()

#define XA_APPLICATION(obj)(                \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
			XA_TYPE_APPLICATION,                  \
			XAApplication))

#define XA_IS_APPLICATION(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			XA_TYPE_APPLICATION))

#define XA_APPLICATION_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			XA_TYPE_APPLICATION,      \
			XAApplicationClass))

#define XA_IS_APPLICATION_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			XA_TYPE_APPLICATION()))	

typedef struct _XAApplication XAApplication;

struct _XAApplication
{
	GObject parent;
	GtkIconTheme *icon_theme;
	XASettings *settings;
	struct {
		gboolean _tabs;
	} props;
};

typedef struct _XAApplicationClass XAApplicationClass;

struct _XAApplicationClass
{
	GObjectClass parent_class;
};

GType      xa_application_get_type();
XAApplication *xa_application_new(GtkIconTheme *icon_theme);

GtkWidget *xa_application_new_window(XAApplication *);

gint xa_application_extract_archive(XAApplication *, gchar *, gchar *);
gint xa_application_new_archive(XAApplication *, gchar *, GSList *);

gint xa_application_open_archive(XAApplication *, GtkWidget *, gchar *);

G_END_DECLS
#endif /* __XARCHIVER_APPLICATION_H__*/
