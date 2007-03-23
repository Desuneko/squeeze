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
#include <glib/gstdio.h>
#include <glib-object.h> 
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze-module.h"
#include "builder-settings.h"

static void
lsq_builder_settings_class_init(LSQBuilderSettingsClass *);
static void
lsq_builder_settings_init(LSQBuilderSettings *builder);
static void
lsq_builder_settings_dispose(GObject *object);
static void
lsq_builder_settings_finalize(GObject *object);

static GObjectClass *parent_class;

GType
lsq_builder_settings_get_type ()
{
	static GType lsq_builder_settings_type = 0;

	if (!lsq_builder_settings_type)
	{
		static const GTypeInfo lsq_builder_settings_info = 
		{
			sizeof (LSQBuilderSettingsClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_builder_settings_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQBuilderSettings),
			0,
			(GInstanceInitFunc) lsq_builder_settings_init,
			NULL
		};

		lsq_builder_settings_type = g_type_register_static (G_TYPE_OBJECT, "LSQBuilderSettings", &lsq_builder_settings_info, 0);
	}
	return lsq_builder_settings_type;
}

static void
lsq_builder_settings_class_init(LSQBuilderSettingsClass *builder_settings_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(builder_settings_class);

	object_class->dispose = lsq_builder_settings_dispose;
	object_class->finalize = lsq_builder_settings_finalize;

	parent_class = g_type_class_peek(G_TYPE_OBJECT); 

}

static void
lsq_builder_settings_init(LSQBuilderSettings *builder_settings)
{
	builder_settings->n_properties = 0;
	builder_settings->property_types = NULL;
	builder_settings->property_names = NULL;
}

/**
 * lsq_builder_settings_dispose:
 *
 * @object: LSQBuilderSettings object
 *
 */
static void
lsq_builder_settings_dispose(GObject *object)
{

	parent_class->dispose(object);
}

/**
 * lsq_builder_settings_finalize:
 *
 * @object: LSQBuilderSettings object
 *
 */
static void
lsq_builder_settings_finalize(GObject *object)
{
	parent_class->finalize(object);
}

LSQBuilderSettings  *
lsq_builder_settings_new()
{
	LSQBuilderSettings *settings;

	settings = g_object_new(LSQ_TYPE_BUILDER_SETTINGS, NULL);

	return settings;
}

guint
lsq_builder_settings_get_n_properties(LSQBuilderSettings *settings)
{
	return settings->n_properties;
}

GType
lsq_builder_settings_get_property_type(LSQBuilderSettings *settings, guint n)
{
	g_return_val_if_fail(settings, G_TYPE_INVALID);
	g_return_val_if_fail(n < settings->n_properties, G_TYPE_INVALID);

	return settings->property_types[n];
}

const gchar *
lsq_builder_settings_get_property_name(LSQBuilderSettings *settings, guint n)
{
	g_return_val_if_fail(settings, G_TYPE_INVALID);
	g_return_val_if_fail(n < settings->n_properties, G_TYPE_INVALID);

	return settings->property_names[n];
}

/********************
 * LSQArchive stuff *
 ********************/

/*
static GType *
lsq_archive_get_entry_property_types(LSQArchive *archive, guint size)
{
	GType *new_props;
	gchar **new_names;
	guint i;

	if(archive->entry_n_property < size)
	{
		new_props = g_new0(GType, size);
		new_names = g_new0(gchar*, size);
		for(i = 0; i < archive->entry_n_property; ++i)
		{
			new_props[i] = archive->entry_property_types[i];
			new_names[i] = archive->entry_property_names[i];
		}
		g_free(archive->entry_property_types);
		g_free(archive->entry_property_names);
		archive->entry_property_types = new_props;
		archive->entry_property_names = new_names;
		archive->entry_n_property = size;
	}
	return archive->entry_property_types;
}

static gchar **
lsq_archive_get_entry_property_names(LSQArchive *archive, guint size)
{
	GType *new_types;
	gchar **new_names;
	guint i;

	if(archive->entry_n_property < size)
	{
		new_types = g_new0(GType, size);
		new_names = g_new0(gchar*, size);
		for(i = 0; i < archive->entry_n_property; ++i)
		{
			new_types[i] = archive->entry_property_types[i];
			new_names[i] = archive->entry_property_names[i];
		}
		g_free(archive->entry_property_types);
		g_free(archive->entry_property_names);
		archive->entry_property_types = new_types;
		archive->entry_property_names = new_names;
		archive->entry_n_property = size;
	}
	return archive->entry_property_names;
}

GType
lsq_archive_get_entry_property_type(LSQArchive *archive, guint i)
{
#ifdef DEBUG // n_property + 2, filename and MIME
	g_return_val_if_fail(archive, G_TYPE_INVALID);
	g_return_val_if_fail(i < lsq_archive_n_entry_properties(archive), G_TYPE_INVALID);
#endif

	switch(i)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			return G_TYPE_STRING;
		default:
#ifdef DEBUG
			g_return_val_if_fail(archive->entry_property_types, G_TYPE_INVALID);
#endif
			return archive->entry_property_types[i - LSQ_ARCHIVE_PROP_USER];
	}
}

const gchar *
lsq_archive_get_entry_property_name(LSQArchive *archive, guint i)
{
#ifdef DEBUG // n_property + 2, filename and MIME
	g_return_val_if_fail(archive, G_TYPE_INVALID);
	g_return_val_if_fail(i < lsq_archive_n_entry_properties(archive), G_TYPE_INVALID);
#endif
	
	switch(i)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
			return _("Name");
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			return _("Mime type");
		default:
#ifdef DEBUG
			g_return_val_if_fail(archive->entry_property_names, G_TYPE_INVALID);
#endif
			return archive->entry_property_names[i - LSQ_ARCHIVE_PROP_USER];
	}
}

void
lsq_archive_set_entry_property_type(LSQArchive *archive, guint i, GType type, const gchar *name)
{
#ifdef DEBUG
	g_return_if_fail(archive);
	g_return_if_fail(i >= LSQ_ARCHIVE_PROP_USER);
#endif

	GType *types_iter = lsq_archive_get_entry_property_types(archive, i+1-LSQ_ARCHIVE_PROP_USER);
	gchar **names_iter = lsq_archive_get_entry_property_names(archive, i+1-LSQ_ARCHIVE_PROP_USER);

	types_iter[i-LSQ_ARCHIVE_PROP_USER] = type;
	g_free(names_iter[i-LSQ_ARCHIVE_PROP_USER]);
	names_iter[i-LSQ_ARCHIVE_PROP_USER] = g_strdup(name);
}

void
lsq_archive_set_entry_property_types(LSQArchive *archive, ...)
{
#ifdef DEBUG
	g_return_if_fail(archive);
#endif
	GType   type;
	gchar  *name;
	guint   size = 0;
	va_list ap;
	va_start(ap, archive);
	while(va_arg(ap, GType) && va_arg(ap, gchar*))
	{
		size++;
	}
	va_end(ap);
	GType *types_iter = lsq_archive_get_entry_property_types(archive, size);
	gchar **names_iter = lsq_archive_get_entry_property_names(archive, size);
	va_start(ap, archive);
	while((type = va_arg(ap, GType)) && (name = va_arg(ap, gchar*)))
	{
		*types_iter = type;
		g_free(*names_iter);
		*names_iter = g_strdup(name);
		types_iter++;
		names_iter++;
	}
}

void
lsq_archive_set_entry_property_typesv(LSQArchive *archive, GType *types, const gchar **names)
{
#ifdef DEBUG
	g_return_if_fail(archive);
#endif
	guint size = 0;
	GType *type_iter = types;
	const gchar **name_iter = names;
	while(type_iter && name_iter)
	{
		size++;
		type_iter++;
		name_iter++;
	}
	GType *types_iter = lsq_archive_get_entry_property_types(archive, size);
	gchar **names_iter = lsq_archive_get_entry_property_names(archive, size);
	type_iter = types;
	name_iter = names;
	while(type_iter && name_iter)
	{
		*types_iter = *type_iter;
		g_free(*names_iter);
		*names_iter = g_strdup(*name_iter);
		types_iter++;
		type_iter++;
		names_iter++;
		name_iter++;
	}
}

void
lsq_archive_clear_entry_property_types(LSQArchive *archive)
{
#ifdef DEBUG
	g_return_if_fail(archive);
#endif
	g_free(archive->entry_property_types);
	g_free(archive->entry_property_names);
	archive->entry_property_types = NULL;
	archive->entry_property_names = NULL;
	archive->entry_n_property = 0;
}
*/
