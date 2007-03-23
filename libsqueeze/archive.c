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

	archive->settings = lsq_command_builder_get_settings(archive->builder);
	
	return archive;
}

/*
 * lsq_archive_n_entry_properties:
 *
 * @archive: LSQArchive object
 *
 */
guint
lsq_archive_n_entry_properties(const LSQArchive *archive)
{
#ifdef DEBUG
	g_return_val_if_fail(archive, 0);
#endif
	return lsq_builder_settings_get_n_properties(archive->settings) + LSQ_ARCHIVE_PROP_USER;
}

/*
 * lsq_archive_get_entry_property_type:
 *
 * @archive: LSQArchive object
 *
 */
GType
lsq_archive_get_entry_property_type(const LSQArchive *archive, guint n)
{
	switch(n)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			return G_TYPE_STRING;
			break;
		default:
			return lsq_builder_settings_get_property_type(archive->settings, n - LSQ_ARCHIVE_PROP_USER);
			break;
	}
}

/*
 * lsq_archive_get_entry_property_name:
 *
 * @archive: LSQArchive object
 *
 */
const gchar *
lsq_archive_get_entry_property_name(const LSQArchive *archive, guint n)
{
	switch(n)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
			return _("Name");
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			return _("Mime type");
			break;
		default:
			return lsq_builder_settings_get_property_name(archive->settings, n - LSQ_ARCHIVE_PROP_USER);
			break;
	}
}

/*
 * lsq_archive_get_filename:
 *
 * @archive: LSQArchive object
 */
gchar *
lsq_archive_get_filename(const LSQArchive *archive)
{
	return g_path_get_basename(archive->path);
}

/*
 * lsq_archive_get_path:
 *
 * @archive: LSQArchive object
 */
const gchar *
lsq_archive_get_path(const LSQArchive *archive)
{
	return archive->path;
}

/*
 * lsq_archive_get_mimetype:
 *
 * @archive: LSQArchive object
 */
const gchar *
lsq_archive_get_mimetype(const LSQArchive *archive)
{
	return thunar_vfs_mime_info_get_name(archive->mime_info);
}

/*
 * lsq_archive_exists:
 *
 * @archive: LSQArchive object
 */
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
	lsq_archive_refreshed(archive);
	g_signal_emit(G_OBJECT(archive), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_COMMAND_TERMINATED], 0, error, NULL);
}
