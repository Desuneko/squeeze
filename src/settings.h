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

#ifndef __SQ_SETTINGS_H__
#define __SQ_SETTINGS_H__
G_BEGIN_DECLS

#define SQ_TYPE_SETTINGS sq_settings_get_type()

#define SQ_SETTINGS(obj)		 ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),	\
			sq_settings_get_type(),	  \
			SQSettings))

#define SQ_IS_SETTINGS(obj)	  ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),	\
			sq_settings_get_type()))

#define SQ_SETTINGS_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),	 \
			sq_settings_get_type(),	  \
			SQSettingsClass))

#define SQ_IS_SETTINGS_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),		\
			sq_settings_get_type()))

typedef struct _SQSettings SQSettings;

struct _SQSettings
{
	GObject parent;
	gchar *config_file;
	GObject *xfce_rc;
};

typedef struct _SQSettingsClass SQSettingsClass;

struct _SQSettingsClass
{
	GObjectClass parent;
};

SQSettings   *sq_settings_new();
GType		 sq_settings_get_type ();

gboolean	  sq_settings_load(SQSettings *);
gboolean	  sq_settings_save(SQSettings *);

const gchar  *sq_settings_read_entry(SQSettings *settings, const gchar *key, const gchar *fallback);
gboolean	  sq_settings_read_bool_entry(SQSettings *settings, const gchar *key, const gboolean fallback);
gint		  sq_settings_read_int_entry(SQSettings *settings, const gchar *key, const gint fallback);

void		  sq_settings_write_entry(SQSettings *settings, const gchar *key, const gchar *value);
void		  sq_settings_write_bool_entry(SQSettings *settings, const gchar *key, const gboolean value);
void		  sq_settings_write_int_entry(SQSettings *settings, const gchar *key, const gint value);

void		  sq_settings_set_group(SQSettings *settings, const gchar *group);

G_END_DECLS

#endif /* __SQ_SETTINGS_H__ */
