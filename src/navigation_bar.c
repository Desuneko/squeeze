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

#ifndef XA_NAVIGATION_BAR_MAX_HISTORY
#define XA_NAVIGATION_BAR_MAX_HISTORY 10
#endif


static void
xa_navigation_bar_class_init(XANavigationBarClass *archive_class);

static void
xa_navigation_bar_init(XANavigationBar *archive);

static void
cb_xa_navigation_bar_pwd_changed(XAArchiveStore *store, XANavigationBar *bar);

static void
xa_navigation_bar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
xa_navigation_bar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

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
	GParamSpec *pspec = NULL;

	object_class->set_property = xa_navigation_bar_set_property;
	object_class->get_property = xa_navigation_bar_get_property;

	pspec = g_param_spec_uint("navigation_history",
		"",
		"",
		0,
		G_MAXUINT,
		XA_NAVIGATION_BAR_MAX_HISTORY,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_NAVIGATION_BAR_NAV_HISTORY, pspec);
}

static void
xa_navigation_bar_init(XANavigationBar *navigation_bar)
{
	navigation_bar->_cb_pwd_changed = (GCallback)cb_xa_navigation_bar_pwd_changed;
	navigation_bar->max_history = XA_NAVIGATION_BAR_MAX_HISTORY;
	navigation_bar->pwd = NULL;
	navigation_bar->history = NULL;
}

void
xa_navigation_bar_set_store(XANavigationBar *navigation_bar, XAArchiveStore *store)
{
	if(store)
	{
		g_return_if_fail(XA_IS_ARCHIVE_STORE(store));
		g_return_if_fail(XA_IS_NAVIGATION_BAR(navigation_bar));
	}

	navigation_bar->store = store;
	if(store)
		g_signal_connect(G_OBJECT(store), "xa_pwd_changed", (GCallback)navigation_bar->_cb_pwd_changed, navigation_bar);
}

GtkWidget *
xa_navigation_bar_new(XAArchiveStore *store)
{
	XANavigationBar *bar;

	bar = g_object_new(XA_TYPE_NAVIGATION_BAR, NULL);

	if(store)
		xa_navigation_bar_set_store(bar, store);

	return GTK_WIDGET(bar);
}

static void
cb_xa_navigation_bar_pwd_changed(XAArchiveStore *store, XANavigationBar *bar)
{
	gchar *path = xa_archive_store_get_pwd(store);
	xa_navigation_bar_history_push(bar, path);
	g_free(path);
}

void
xa_navigation_bar_history_push(XANavigationBar *nav_bar, const gchar *path)
{
	nav_bar->history = g_list_insert_before(nav_bar->history, nav_bar->pwd, g_strdup(path));
	if(!nav_bar->pwd)
		nav_bar->pwd = nav_bar->history;

	if(g_list_previous(nav_bar->pwd))
		nav_bar->pwd = g_list_previous(nav_bar->pwd);
	if(nav_bar->pwd)
		nav_bar->history = nav_bar->pwd;

	GList *temp_list = g_list_previous(nav_bar->pwd);

	if(nav_bar->history)
		nav_bar->history->prev = NULL;
	if(temp_list)
	{
		temp_list->next = NULL;
		temp_list = g_list_first(temp_list);
		g_list_foreach(temp_list, (GFunc)g_free, NULL);
		g_list_free(temp_list);
	}
	if(g_list_length(nav_bar->history) > nav_bar->max_history)
	{
		GList *last = g_list_last(nav_bar->history);
		nav_bar->history = g_list_remove(nav_bar->history, last->data);
	}
}

gint
xa_navigation_bar_history_get_length(XANavigationBar *nav_bar)
{
	return g_list_length(nav_bar->history);
}

gboolean
xa_navigation_bar_history_back(XANavigationBar *nav_bar)
{
	if(!g_list_next(nav_bar->pwd))
		return FALSE;
	nav_bar->pwd = g_list_next(nav_bar->pwd);
	xa_archive_store_set_pwd_silent(nav_bar->store, nav_bar->pwd->data);
	return TRUE;
}

gboolean
xa_navigation_bar_history_forward(XANavigationBar *nav_bar)
{
	if(!g_list_previous(nav_bar->pwd))
		return FALSE;
	nav_bar->pwd = g_list_previous(nav_bar->pwd);
	xa_archive_store_set_pwd_silent(nav_bar->store, nav_bar->pwd->data);
	return TRUE;
}

/* This *is* correct!!! */
gboolean
xa_navigation_bar_history_has_next(XANavigationBar *nav_bar)
{
	if(nav_bar->pwd)
		return (g_list_previous(nav_bar->pwd)?TRUE:FALSE);
	return FALSE;
}

/* idem */
gboolean
xa_navigation_bar_history_has_previous(XANavigationBar *nav_bar)
{
	if(nav_bar->pwd)
		return (g_list_next(nav_bar->pwd)?TRUE:FALSE);
	return FALSE;
}

void
xa_navigation_bar_clear_history(XANavigationBar *nav_bar)
{
	nav_bar->pwd = g_list_first(nav_bar->history);
	g_list_foreach(nav_bar->pwd, (GFunc)g_free, NULL);
	g_list_free(nav_bar->pwd);
	nav_bar->history = NULL;
	nav_bar->pwd = NULL;
}

static void
xa_navigation_bar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_NAVIGATION_BAR_NAV_HISTORY:
			XA_NAVIGATION_BAR(object)->max_history = g_value_get_uint(value);		
			break;
	}
}

static void
xa_navigation_bar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_NAVIGATION_BAR_NAV_HISTORY:
			g_value_set_uint(value, XA_NAVIGATION_BAR(object)->max_history);
			break;
	}
}
