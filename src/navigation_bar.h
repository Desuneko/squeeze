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

#ifndef __SQRCHIVER_NAVIGATION_BAR_H__
#define __SQRCHIVER_NAVIGATION_BAR_H__
G_BEGIN_DECLS

#define SQ_TYPE_NAVIGATION_BAR sq_navigation_bar_get_type()

#define SQ_NAVIGATION_BAR(obj)(                \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
			SQ_TYPE_NAVIGATION_BAR,                  \
			SQNavigationBar))

#define SQ_IS_NAVIGATION_BAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			SQ_TYPE_NAVIGATION_BAR))

#define SQ_NAVIGATION_BAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			SQ_TYPE_NAVIGATION_BAR,      \
			SQNavigationBarClass))

#define SQ_IS_NAVIGATION_BAR_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			SQ_TYPE_NAVIGATION_BAR()))	

typedef struct _SQNavigationBar SQNavigationBar;

struct _SQNavigationBar
{
	GtkContainer parent;
	SQArchiveStore *store;
	void (*_cb_pwd_changed)(SQArchiveStore *, SQNavigationBar *);
	void (*_cb_new_archive)(SQArchiveStore *, SQNavigationBar *);
	void (*_cb_store_set)(SQNavigationBar *);
};

typedef struct _SQNavigationBarClass SQNavigationBarClass;

struct _SQNavigationBarClass
{
	GtkToolbarClass parent_class;
};

GType      sq_navigation_bar_get_type();
SQNavigationBar *sq_navigation_bar_new();
void       sq_navigation_bar_set_store(SQNavigationBar *navigation_bar, SQArchiveStore *store);

G_END_DECLS
#endif /* __SQRCHIVER_NAVIGATION_BAR_H__*/
