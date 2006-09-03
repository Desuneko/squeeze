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
	GtkWidget *main_vbox;
	GtkWidget *menubar;
	GtkWidget *toolbar;
	GtkWidget *statusbar;
	LXAArchive *archive;
};

typedef struct _XAMainWindowClass XAMainWindowClass;

struct _XAMainWindowClass
{
	GtkWindowClass parent;
};

GtkWidget *xa_main_window_new();
GtkWidget *xa_main_window_find_image(gchar *, GtkIconSize);

void cb_xa_main_new_archive(GtkWidget *widget, gpointer userdata);
void cb_xa_main_open_archive(GtkWidget *widget, gpointer userdata);

G_END_DECLS
#endif /* __XARCHIVER_MAIN_WINDOW_H__ */
