/*  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
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

static SQSettings *sq_global_settings = NULL;

static void
sq_settings_init(SQSettings *);
static void
sq_settings_class_init(SQSettingsClass *);

GType
sq_settings_get_type ()
{
	static GType sq_settings_type = 0;

	if (!sq_settings_type)
	{
		static const GTypeInfo sq_settings_info = 
		{
			sizeof (SQSettingsClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_settings_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQSettings),
			0,
			(GInstanceInitFunc) sq_settings_init,
			NULL
		};

		sq_settings_type = g_type_register_static (G_TYPE_OBJECT, "SQSettings", &sq_settings_info, 0);
	}
	return sq_settings_type;
}

static void
sq_settings_init(SQSettings *object)
{
#ifdef HAVE_LIBXFCE4UTIL
	object->xfce_rc = (GObject *)xfce_rc_config_open(XFCE_RESOURCE_CONFIG, "squeeze/squeezerc", FALSE);
#else

#endif /* HAVE_LIBXFCE4UTIL */
}

static void
sq_settings_class_init(SQSettingsClass *object_class)
{

}

SQSettings *
sq_settings_new()
{
	if(!sq_global_settings)
		sq_global_settings = g_object_new(SQ_TYPE_SETTINGS, NULL);

	return sq_global_settings;
}

gboolean
sq_settings_load(SQSettings *settings)
{
#ifdef HAVE_LIBXFCE4UTIL

#else

#endif /* HAVE_LIBXFCE4UTIL */
	return TRUE;
}

gboolean
sq_settings_save(SQSettings *settings)
{
#ifdef HAVE_LIBXFCE4UTIL
	xfce_rc_flush(XFCE_RC(settings->xfce_rc));
#else

#endif /* HAVE_LIBXFCE4UTIL */
	return TRUE;
}

void
sq_settings_set_group(SQSettings *settings, const gchar *group)
{
#ifdef HAVE_LIBXFCE4UTIL
	xfce_rc_set_group(XFCE_RC(settings->xfce_rc), group);
#else

#endif /* HAVE_LIBXFCE4UTIL */
}

void
sq_settings_write_entry(SQSettings *settings, const gchar *key, const gchar *value)
{
#ifdef HAVE_LIBXFCE4UTIL
	xfce_rc_write_entry(XFCE_RC(settings->xfce_rc), key, value);
#else

#endif /* HAVE_LIBXFCE4UTIL */
}

void
sq_settings_write_bool_entry(SQSettings *settings, const gchar *key, const gboolean value)
{
#ifdef HAVE_LIBXFCE4UTIL
	xfce_rc_write_bool_entry(XFCE_RC(settings->xfce_rc), key, value);
#else

#endif /* HAVE_LIBXFCE4UTIL */
}

const gchar *
sq_settings_read_entry(SQSettings *settings, const gchar *key, const gchar *fallback)
{
#ifdef HAVE_LIBXFCE4UTIL
	return xfce_rc_read_entry(XFCE_RC(settings->xfce_rc), key, fallback);
#else
	return g_strdup(fallback);
#endif /* HAVE_LIBXFCE4UTIL */
}

gboolean
sq_settings_read_bool_entry(SQSettings *settings, const gchar *key, const gboolean fallback)
{
#ifdef HAVE_LIBXFCE4UTIL
	return xfce_rc_read_bool_entry(XFCE_RC(settings->xfce_rc), key, fallback);
#else
	return fallback;
#endif /* HAVE_LIBXFCE4UTIL */
}
