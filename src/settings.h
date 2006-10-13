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

#ifndef __XA_SETTINGS_H__
#define __XA_SETTINGS_H__
G_BEGIN_DECLS

#define XA_TYPE_SETTINGS xa_settings_get_type()

#define XA_SETTINGS(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			xa_settings_get_type(),      \
			XASettings))

#define XA_IS_SETTINGS(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_settings_get_type()))

#define XA_SETTINGS_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			xa_settings_get_type(),      \
			XASettingsClass))

#define XA_IS_SETTINGS_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			xa_settings_get_type()))

typedef struct _XASettings XASettings;

struct _XASettings
{
	GObject parent;
	gchar *config_file;
};

typedef struct _XASettingsClass XASettingsClass;

struct _XASettingsClass
{
	GObjectClass parent;
};

XASettings  *xa_settings_new(gchar *config_dir);
GType        xa_settings_get_type ();

gboolean     xa_settings_load(XASettings *);

G_END_DECLS

#endif /* __XA_SETTINGS_H__ */
