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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>
#include "archive_store.h"
#include "navigation_bar.h"


static void
sq_navigation_bar_class_init(SQNavigationBarClass *archive_class);

static void
sq_navigation_bar_init(SQNavigationBar *archive);
static void
sq_navigation_bar_dispose(GObject *object);

static void
cb_sq_navigation_bar_pwd_changed(SQArchiveStore *store, SQNavigationBar *bar);

static void
cb_sq_navigation_bar_new_archive(SQArchiveStore *store, SQNavigationBar *bar);

/* properties */
enum {
	SQ_NAVIGATION_BAR_NAV_HISTORY = 1
};

static GObjectClass *parent_class;

GType
sq_navigation_bar_get_type ()
{
	static GType sq_navigation_bar_type = 0;

 	if (!sq_navigation_bar_type)
	{
 		static const GTypeInfo sq_navigation_bar_info = 
		{
			sizeof (SQNavigationBarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_navigation_bar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQNavigationBar),
			0,
			(GInstanceInitFunc) sq_navigation_bar_init,
			NULL
		};

		sq_navigation_bar_type = g_type_register_static (GTK_TYPE_CONTAINER, "SQNavigationBar", &sq_navigation_bar_info, 0);
	}
	return sq_navigation_bar_type;
}

static void
sq_navigation_bar_class_init(SQNavigationBarClass *navigation_bar_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (navigation_bar_class);

	parent_class = gtk_type_class (GTK_TYPE_CONTAINER);

	object_class->dispose     = sq_navigation_bar_dispose;
}

static void
sq_navigation_bar_init(SQNavigationBar *navigation_bar)
{
	GTK_WIDGET_SET_FLAGS(navigation_bar, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate(GTK_WIDGET(navigation_bar), FALSE);

	navigation_bar->_cb_pwd_changed = cb_sq_navigation_bar_pwd_changed;
	navigation_bar->_cb_new_archive = cb_sq_navigation_bar_new_archive;
}

static void
sq_navigation_bar_dispose(GObject *object)
{
	SQNavigationBar *navigation_bar = SQ_NAVIGATION_BAR(object);
	if(navigation_bar->store)
	{
		if(navigation_bar->_cb_pwd_changed)
		{
			g_signal_handlers_disconnect_by_func(navigation_bar->store, navigation_bar->_cb_pwd_changed, navigation_bar);
			navigation_bar->_cb_pwd_changed = NULL;
		}
		if(navigation_bar->_cb_new_archive)
		{
			g_signal_handlers_disconnect_by_func(navigation_bar->store, navigation_bar->_cb_new_archive, navigation_bar);
			navigation_bar->_cb_new_archive = NULL;
		}
		navigation_bar->store = NULL;
	}
	parent_class->dispose(object);
}

void
sq_navigation_bar_set_store(SQNavigationBar *navigation_bar, SQArchiveStore *store)
{
	if(navigation_bar->store)
	{
		if(navigation_bar->_cb_pwd_changed)
		{
			g_signal_handlers_disconnect_by_func(navigation_bar->store, navigation_bar->_cb_pwd_changed, navigation_bar);
		}
		if(navigation_bar->_cb_new_archive)
		{
			g_signal_handlers_disconnect_by_func(navigation_bar->store, navigation_bar->_cb_new_archive, navigation_bar);
		}
		navigation_bar->store = NULL;
	}

	navigation_bar->store = store;
	if(store)
	{
		g_return_if_fail(SQ_IS_ARCHIVE_STORE(store));
		g_return_if_fail(SQ_IS_NAVIGATION_BAR(navigation_bar));
		g_signal_connect(G_OBJECT(store), "sq-pwd-changed", (GCallback)navigation_bar->_cb_pwd_changed, navigation_bar);
		g_signal_connect(G_OBJECT(store), "sq-new-archive", (GCallback)navigation_bar->_cb_new_archive, navigation_bar);
	}

	navigation_bar->_cb_store_set(navigation_bar);
}

SQNavigationBar *
sq_navigation_bar_new(SQArchiveStore *store)
{
	SQNavigationBar *bar;

	bar = g_object_new(SQ_TYPE_NAVIGATION_BAR, NULL);

	if(store)
		sq_navigation_bar_set_store(bar, store);

	return bar;
}

static void
cb_sq_navigation_bar_pwd_changed(SQArchiveStore *store, SQNavigationBar *bar)
{
}

static void
cb_sq_navigation_bar_new_archive(SQArchiveStore *store, SQNavigationBar *bar)
{
}

