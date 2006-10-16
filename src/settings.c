/*  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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
#include <glib.h>
#include <glib-object.h>

#ifdef HAVE_LIBXFCE4UTIL
#include <libxfce4util/libxfce4util.h>
#endif

#include "settings.h"

static void
xa_settings_init(XASettings *);
static void
xa_settings_class_init(XASettingsClass *);

GType
xa_settings_get_type ()
{
	static GType xa_settings_type = 0;

	if (!xa_settings_type)
	{
		static const GTypeInfo xa_settings_info = 
		{
			sizeof (XASettingsClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_settings_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASettings),
			0,
			(GInstanceInitFunc) xa_settings_init,
			NULL
		};

		xa_settings_type = g_type_register_static (G_TYPE_OBJECT, "XASettings", &xa_settings_info, 0);
	}
	return xa_settings_type;
}

static void
xa_settings_init(XASettings *object)
{
#ifdef HAVE_LIBXFCE4UTIL
	object->xfce_rc = (GObject *)xfce_rc_config_open(XFCE_RESOURCE_CONFIG, "xarchiver/xarchiverrc", FALSE);
#else

#endif /* HAVE_LIBXFCE4UTIL */
}

static void
xa_settings_class_init(XASettingsClass *object_class)
{

}

XASettings *
xa_settings_new()
{
	XASettings *object = g_object_new(XA_TYPE_SETTINGS, NULL);

	return object;
}

gboolean
xa_settings_load(XASettings *settings)
{
#ifdef HAVE_LIBXFCE4UTIL

#else

#endif /* HAVE_LIBXFCE4UTIL */
	return TRUE;
}

gboolean
xa_settings_save(XASettings *settings)
{
#ifdef HAVE_LIBXFCE4UTIL
	xfce_rc_flush(XFCE_RC(settings->xfce_rc));
#else

#endif /* HAVE_LIBXFCE4UTIL */
	return TRUE;
}

void
xa_settings_set_group(XASettings *settings, const gchar *group)
{
#ifdef HAVE_LIBXFCE4UTIL
	xfce_rc_set_group(XFCE_RC(settings->xfce_rc), group);
#else

#endif /* HAVE_LIBXFCE4UTIL */
}

void
xa_settings_write_entry(XASettings *settings, const gchar *key, const gchar *value)
{
#ifdef HAVE_LIBXFCE4UTIL
	xfce_rc_write_entry(XFCE_RC(settings->xfce_rc), key, value);
#else

#endif /* HAVE_LIBXFCE4UTIL */
}

const gchar *
xa_settings_read_entry(XASettings *settings, const gchar *key, const gchar *fallback)
{
#ifdef HAVE_LIBXFCE4UTIL
	return xfce_rc_read_entry(XFCE_RC(settings->xfce_rc), key, fallback);
#else

#endif /* HAVE_LIBXFCE4UTIL */
}
