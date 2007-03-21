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

#include "libsqueeze.h"
#include "libsqueeze-module.h"
#include "archive-iter.h"
#include "archive-command.h"
#include "archive.h"
#include "command-builder.h"
#include "slist.h"
#include "archive-tempfs.h"

#include "vfs-mime.h"

#include "internals.h"

#ifndef LSQ_ENTRY_CHILD_BUFFER_SIZE
#define LSQ_ENTRY_CHILD_BUFFER_SIZE 500
#endif

#ifndef LSQ_MIME_DIRECTORY
#define LSQ_MIME_DIRECTORY "inode/directory"
#endif

struct _LSQArchiveClass
{
	GObjectClass parent;
};

static void
lsq_archive_class_init(LSQArchiveClass *archive_class);

static void
lsq_archive_init(LSQArchive *archive);

static void
lsq_archive_finalize(GObject *object);

static void
cb_archive_archive_command_terminated(LSQArchiveCommand *command, GError *error, LSQArchive *archive);

enum
{
	LSQ_ARCHIVE_SIGNAL_COMMAND_STARTED = 0,
	LSQ_ARCHIVE_SIGNAL_COMMAND_TERMINATED,
	LSQ_ARCHIVE_SIGNAL_REFRESHED,
	LSQ_ARCHIVE_SIGNAL_COUNT
};

static gint lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_COUNT];

GType
lsq_archive_get_type ()
{
	static GType lsq_archive_type = 0;

 	if (!lsq_archive_type)
	{
 		static const GTypeInfo lsq_archive_info = 
		{
			sizeof (LSQArchiveClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchive),
			0,
			(GInstanceInitFunc) lsq_archive_init,
			NULL
		};

		lsq_archive_type = g_type_register_static (G_TYPE_OBJECT, "LSQArchive", &lsq_archive_info, 0);
	}
	return lsq_archive_type;
}

static void
lsq_archive_class_init(LSQArchiveClass *archive_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(archive_class);

	object_class->finalize = lsq_archive_finalize;
	
	lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_COMMAND_STARTED] = g_signal_new("command-started",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
	lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_COMMAND_TERMINATED] = g_signal_new("command-terminated",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
	lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_REFRESHED] = g_signal_new("refreshed",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);
}

static void
lsq_archive_init(LSQArchive *archive)
{
	lsq_archive_init_iter(archive);
#ifdef LSQ_THREADSAFE
	g_static_rw_lock_init(&archive->rw_lock);
#endif /* LSQ_THREADSAFE */
}

/** static void
 * lsq_archive_finalize(GObject *object)
 *
 * 
 */
static void
lsq_archive_finalize(GObject *object)
{
	g_return_if_fail(LSQ_IS_ARCHIVE(object));
	LSQArchive *archive = (LSQArchive *)(object);
	if(archive->path)
		g_free(archive->path);
	if(archive->path_info)
		thunar_vfs_path_unref(archive->path_info);
	if(archive->file_info)
		thunar_vfs_info_unref(archive->file_info);
	if(archive->mime_info)
		thunar_vfs_mime_info_unref(archive->mime_info);

	lsq_archive_free_iter(archive);
	lsq_tempfs_clean_root_dir(archive);
	lsq_opened_archive_list = g_slist_remove(lsq_opened_archive_list, object);
}

LSQArchive *
lsq_archive_new(gchar *path, const gchar *mime)
{
	LSQArchive *archive;
	gchar *base = NULL;

	archive = g_object_new(lsq_archive_get_type(), NULL);

	if(path)
	{
		if(g_path_is_absolute(path))
			archive->path_info = thunar_vfs_path_new(path, NULL);
		else
			archive->path_info = thunar_vfs_path_relative(lsq_relative_base_path, path);
		archive->path = thunar_vfs_path_dup_string(archive->path_info);
	}
	else
		archive->path_info = NULL;


	archive->file_info = thunar_vfs_info_new_for_path(archive->path_info, NULL);
	if(archive->file_info)
	{
		archive->mime_info = archive->file_info->mime_info;
		thunar_vfs_mime_info_ref(archive->mime_info);
	}
	else
	{
		if(mime)
			archive->mime_info = thunar_vfs_mime_database_get_info(lsq_mime_database, mime);
		else
		{
			base = g_path_get_basename(path);
			archive->mime_info = thunar_vfs_mime_database_get_info_for_file(lsq_mime_database, path, base);
			g_free(base);
		}
	}
#ifdef DEBUG
	g_debug("%s\n", thunar_vfs_mime_info_get_name(archive->mime_info));
#endif

	archive->builder = lsq_archive_mime_get_default_builder(thunar_vfs_mime_info_get_name(archive->mime_info));

	if(!archive->builder) /* Then it is not supported */
	{
		g_object_unref(archive);
		archive = NULL;
	}
	
	return archive;
}

/********************
 * LSQArchive stuff *
 ********************/

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

/*
 * GType
 * lsq_archive_get_entry_property_type(LSQArchive *archive, guint i)
 *
 */
GType
lsq_archive_get_entry_property_type(LSQArchive *archive, guint i)
{
#ifdef DEBUG /* n_property + 2, filename and MIME */
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

/*
 * const gchar *
 * lsq_archive_get_entry_property_name(LSQArchive *, guint)
 *
 */
const gchar *
lsq_archive_get_entry_property_name(LSQArchive *archive, guint i)
{
#ifdef DEBUG /* n_property + 2, filename and MIME */
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

/*
 * void
 * lsq_archive_set_entry_property_type(LSQArchive *archive, guint i, GType *, const gchar *)
 *
 */
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

/*
 * void
 * lsq_archive_set_entry_property_types(LSQArchive *archive, ...)
 *
 */
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

/*
 * void
 * lsq_archive_set_entry_property_typesv(LSQArchive *archive, GType *)
 *
 */
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

guint
lsq_archive_n_entry_properties(LSQArchive *archive)
{
#ifdef DEBUG
	g_return_val_if_fail(archive, 0);
#endif
	return archive->entry_n_property + LSQ_ARCHIVE_PROP_USER;
}

gchar *
lsq_archive_get_filename(const LSQArchive *archive)
{
	return g_path_get_basename(archive->path);
}

const gchar *
lsq_archive_get_path(const LSQArchive *archive)
{
	return archive->path;
}

const gchar *
lsq_archive_get_mimetype(const LSQArchive *archive)
{
	return thunar_vfs_mime_info_get_name(archive->mime_info);
}

gboolean
lsq_archive_exists(const LSQArchive *archive)
{
	if(archive->file_info)
		return TRUE;
	
	if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
	{
		if(!g_file_test(archive->path, G_FILE_TEST_IS_DIR))
			return TRUE;
			/* TODO: should file_info be created */
	}

	return FALSE;
}

gboolean
lsq_archive_can_stop(const LSQArchive *archive)
{
	if(archive->command)
		return archive->command->safe;
	else
		return TRUE;
}

gboolean
lsq_archive_stop(const LSQArchive *archive)
{
	if(archive->command)
		return lsq_archive_command_stop(archive->command);
	else
		return FALSE;
}

const gchar *
lsq_archive_get_status(const LSQArchive *archive)
{
	if(archive->command)
		return lsq_archive_command_get_comment(archive->command);
	else
		return _("idle");
}

void
lsq_archive_refreshed(const LSQArchive *archive)
{
	g_signal_emit(G_OBJECT(archive), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_REFRESHED], 0, NULL);
}

gboolean
lsq_archive_add(LSQArchive *archive, GSList *files)
{
	g_return_val_if_fail(archive->builder, FALSE);
	LSQCommandBuilder *builder = archive->builder;
	if(archive->command)
		return FALSE;

	archive->command = builder->build_add(builder, archive, files);
	g_signal_connect(archive->command, "terminated", G_CALLBACK(cb_archive_archive_command_terminated), archive);
	if(!lsq_archive_command_execute(archive->command))
	{
		g_object_unref(archive->command);
		archive->command = NULL;
	}
	else
		g_object_unref(archive->command);
	return FALSE;
}

gboolean
lsq_archive_extract(LSQArchive *archive, const gchar *dest_path, GSList *files)
{
	g_return_val_if_fail(archive->builder, FALSE);
	LSQCommandBuilder *builder = archive->builder;
	if(archive->command)
		return FALSE;
	
	archive->command = builder->build_extract(builder, archive, dest_path, files);
	g_signal_connect(archive->command, "terminated", G_CALLBACK(cb_archive_archive_command_terminated), archive);
	if(!lsq_archive_command_execute(archive->command))
	{
		g_object_unref(archive->command);
		archive->command = NULL;
	}
	else
		g_object_unref(archive->command);
	return FALSE;
}

gboolean
lsq_archive_remove(LSQArchive *archive, GSList *files)
{
	g_return_val_if_fail(archive->builder, FALSE);
	LSQCommandBuilder *builder = archive->builder;
	if(archive->command)
		return FALSE;

	archive->command = builder->build_remove(builder, archive, files);
	g_signal_connect(archive->command, "terminated", G_CALLBACK(cb_archive_archive_command_terminated), archive);
	if(!lsq_archive_command_execute(archive->command))
	{
		g_object_unref(archive->command);
		archive->command = NULL;
	}
	else
		g_object_unref(archive->command);
	return FALSE;
}

gboolean
lsq_archive_refresh(LSQArchive *archive)
{
	g_return_val_if_fail(archive->builder, FALSE);
	LSQCommandBuilder *builder = archive->builder;
	if(archive->command)
		return FALSE;

	archive->command = builder->build_refresh(builder, archive);
	if(archive->command)
	{
		g_signal_connect(archive->command, "terminated", G_CALLBACK(cb_archive_archive_command_terminated), archive);
		if(!lsq_archive_command_execute(archive->command))
		{
			g_object_unref(archive->command);
			archive->command = NULL;
			return FALSE;
		}
		else
			g_object_unref(archive->command);
		return TRUE;
	}
	return FALSE;
}

gboolean
lsq_archive_view(LSQArchive *archive, GSList *files)
{
	g_return_val_if_fail(archive->builder, FALSE);
	LSQCommandBuilder *builder = archive->builder;
	if(archive->command)
		return FALSE;

	archive->command = builder->build_open(builder, archive, files);
	g_signal_connect(archive->command, "terminated", G_CALLBACK(cb_archive_archive_command_terminated), archive);
	if(!lsq_archive_command_execute(archive->command))
	{
		g_object_unref(archive->command);
		archive->command = NULL;
	}
	else
		g_object_unref(archive->command);
	return FALSE;
}

static void
cb_archive_archive_command_terminated(LSQArchiveCommand *command, GError *error, LSQArchive *archive)
{
	archive->command = NULL;
#ifdef DEBUG
	g_debug("COMMAND TERMINATED");
#endif
}
