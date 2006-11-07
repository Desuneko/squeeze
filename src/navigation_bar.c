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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>
#include "archive_store.h"
#include "navigation_bar.h"


static void
xa_navigation_bar_class_init(XANavigationBarClass *archive_class);

static void
xa_navigation_bar_init(XANavigationBar *archive);
static void
xa_navigation_bar_finalize(GObject *object);

static void
cb_xa_navigation_bar_pwd_changed(XAArchiveStore *store, XANavigationBar *bar);

static void
cb_xa_navigation_bar_new_archive(XAArchiveStore *store, XANavigationBar *bar);

/* properties */
enum {
	XA_NAVIGATION_BAR_NAV_HISTORY = 1
};

GType
xa_navigation_bar_get_type ()
{
	static GType xa_navigation_bar_type = 0;

 	if (!xa_navigation_bar_type)
	{
 		static const GTypeInfo xa_navigation_bar_info = 
		{
			sizeof (XANavigationBarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_navigation_bar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XANavigationBar),
			0,
			(GInstanceInitFunc) xa_navigation_bar_init,
			NULL
		};

		xa_navigation_bar_type = g_type_register_static (GTK_TYPE_CONTAINER, "XANavigationBar", &xa_navigation_bar_info, 0);
	}
	return xa_navigation_bar_type;
}

static void
xa_navigation_bar_class_init(XANavigationBarClass *navigation_bar_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (navigation_bar_class);

	object_class->finalize     = xa_navigation_bar_finalize;
}

static void
xa_navigation_bar_init(XANavigationBar *navigation_bar)
{
	GTK_WIDGET_SET_FLAGS(navigation_bar, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate(GTK_WIDGET(navigation_bar), FALSE);

	navigation_bar->_cb_pwd_changed = cb_xa_navigation_bar_pwd_changed;
	navigation_bar->_cb_new_archive = cb_xa_navigation_bar_new_archive;
}

static void
xa_navigation_bar_finalize(GObject *object)
{
	XANavigationBar *navigation_bar = XA_NAVIGATION_BAR(object);
	if(navigation_bar->store)
	{
		if(navigation_bar->_cb_pwd_changed)
			g_signal_handlers_disconnect_by_func(navigation_bar->store, navigation_bar->_cb_pwd_changed, navigation_bar);
		if(navigation_bar->_cb_new_archive)
			g_signal_handlers_disconnect_by_func(navigation_bar->store, navigation_bar->_cb_new_archive, navigation_bar);
	}
}

void
xa_navigation_bar_set_store(XANavigationBar *navigation_bar, XAArchiveStore *store)
{
	if(navigation_bar->store)
	{
		if(navigation_bar->_cb_pwd_changed)
			g_signal_handlers_disconnect_by_func(navigation_bar->store, navigation_bar->_cb_pwd_changed, navigation_bar);
		if(navigation_bar->_cb_new_archive)
			g_signal_handlers_disconnect_by_func(navigation_bar->store, navigation_bar->_cb_new_archive, navigation_bar);
	}

	navigation_bar->store = store;
	if(store)
	{
		g_return_if_fail(XA_IS_ARCHIVE_STORE(store));
		g_return_if_fail(XA_IS_NAVIGATION_BAR(navigation_bar));
		g_signal_connect(G_OBJECT(store), "xa-pwd-changed", (GCallback)navigation_bar->_cb_pwd_changed, navigation_bar);
		g_signal_connect(G_OBJECT(store), "xa-new-archive", (GCallback)navigation_bar->_cb_new_archive, navigation_bar);
	}

	navigation_bar->_cb_store_set(navigation_bar);
}

XANavigationBar *
xa_navigation_bar_new(XAArchiveStore *store)
{
	XANavigationBar *bar;

	bar = g_object_new(XA_TYPE_NAVIGATION_BAR, NULL);

	if(store)
		xa_navigation_bar_set_store(bar, store);

	return bar;
}

static void
cb_xa_navigation_bar_pwd_changed(XAArchiveStore *store, XANavigationBar *bar)
{
}

static void
cb_xa_navigation_bar_new_archive(XAArchiveStore *store, XANavigationBar *bar)
{
}

