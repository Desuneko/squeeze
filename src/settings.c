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

}

static void
xa_settings_class_init(XASettingsClass *object_class)
{

}

XASettings *
xa_settings_new(gchar *config_dir)
{
	XASettings *object = g_object_new(XA_TYPE_SETTINGS, NULL);
	if(!config_dir)
		config_dir = "~/.config/";
	object->config_file = g_strconcat(config_dir, "/xarchiver/config.xml", NULL);

	return object;
}

gboolean
xa_settings_load(XASettings *settings)
{
//	gint fd = g_open(settings->config_file, O_RDONLY, 0);
	return FALSE;
}
