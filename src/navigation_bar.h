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

#ifndef __XARCHIVER_NAVIGATION_BAR_H__
#define __XARCHIVER_NAVIGATION_BAR_H__
G_BEGIN_DECLS

#define XA_TYPE_NAVIGATION_BAR xa_navigation_bar_get_type()

#define XA_NAVIGATION_BAR(obj)(                \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
			XA_TYPE_NAVIGATION_BAR,                  \
			XANavigationBar))

#define XA_IS_NAVIGATION_BAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			XA_TYPE_NAVIGATION_BAR))

#define XA_NAVIGATION_BAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			XA_TYPE_NAVIGATION_BAR,      \
			XANavigationBarClass))

#define XA_IS_NAVIGATION_BAR_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			XA_TYPE_NAVIGATION_BAR()))	

typedef struct _XANavigationBar XANavigationBar;

struct _XANavigationBar
{
	GtkContainer parent;
	XAArchiveStore *store;
	void (*_cb_pwd_changed)(XAArchiveStore *, XANavigationBar *);
	void (*_cb_new_archive)(XAArchiveStore *, XANavigationBar *);
	void (*_cb_store_set)(XANavigationBar *);
};

typedef struct _XANavigationBarClass XANavigationBarClass;

struct _XANavigationBarClass
{
	GtkToolbarClass parent_class;
};

GType      xa_navigation_bar_get_type();
XANavigationBar *xa_navigation_bar_new();
void       xa_navigation_bar_set_store(XANavigationBar *navigation_bar, XAArchiveStore *store);

G_END_DECLS
#endif /* __XARCHIVER_NAVIGATION_BAR_H__*/
