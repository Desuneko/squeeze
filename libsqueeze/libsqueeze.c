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
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze.h"
#include "libsqueeze/libsqueeze-module.h"
#include "libsqueeze/libsqueeze-vfs-mime.h"
#include "libsqueeze/archive-iter.h"
#include "libsqueeze/archive-command.h"
#include "libsqueeze/archive.h"
#include "libsqueeze/command-builder.h"
#include "libsqueeze/command-builder-zip.h"
#include "libsqueeze/command-builder-rar.h"
#include "libsqueeze/command-builder-compr.h"
#include "libsqueeze/command-builder-gnu-tar.h"

#include "vfs-mime.h"

#include "internals.h"

static gint
lsq_lookup_mime(gconstpointer a, gconstpointer b);

void
lsq_init()
{
	LSQCommandBuilder *builder = NULL;
	gchar *current_dir = g_get_current_dir();

	lsq_mime_database = thunar_vfs_mime_database_get_default();

	builder = lsq_command_builder_zip_new();
	if(builder)
		lsq_command_builder_register(builder);

	builder = lsq_command_builder_gnu_tar_new();
	if(builder)
		lsq_command_builder_register(builder);

	builder = lsq_command_builder_rar_new();
	if(builder)
		lsq_command_builder_register(builder);

	builder = lsq_command_builder_compr_new();
	if(builder)
		lsq_command_builder_register(builder);

	lsq_relative_base_path = thunar_vfs_path_new(current_dir, NULL);
	lsq_opened_archive_list = NULL;
	g_free(current_dir);
}

void
lsq_shutdown()
{
	g_slist_foreach(lsq_opened_archive_list, (GFunc)lsq_close_archive, NULL);

	g_object_unref(lsq_mime_database);
	thunar_vfs_path_unref(lsq_relative_base_path);
}

/*
 * XAArchive* lsq_new_archive(gchar *path, LSQArchiveType type, gboolean overwrite)
 *
 */
gint
lsq_new_archive(gchar *path, gboolean overwrite, gchar *mime, LSQArchive **lp_archive)
{
	if(overwrite)
		g_unlink(path);

	if(g_file_test(path, G_FILE_TEST_EXISTS))
	{
		(*lp_archive) = NULL;
		return 1;
	}

	LSQArchive *archive = lsq_archive_new(path, mime);
	(*lp_archive) = archive;
	if(!archive)
		return 1;
	return 0;
}

/*
 *
 * XAArchive* lsq_open_archive(gchar *path)
 *
 */
gint
lsq_open_archive(gchar *path, LSQArchive **lp_archive)
{
	if(!g_file_test(path, G_FILE_TEST_EXISTS))
	{
		(*lp_archive) = NULL;
		return 1;
	}

	LSQArchive *archive = lsq_opened_archive_get_archive(path);
	if(!archive)
	{
		archive = lsq_archive_new(path, NULL);
		if(archive)
			lsq_opened_archive_list = g_slist_prepend(lsq_opened_archive_list, archive);
	}
	(*lp_archive) = archive;
	if(!archive)
		return 1;
	return 0;
}


GSList *
lsq_get_supported_mime_types(LSQOperationSupportType types)
{
	GSList *m_types = g_slist_copy(lsq_mime_info_list);
	if(types &= LSQ_OPERATION_SUPPORT_ADD)
	{
		GSList *_types = m_types;
		while(_types)
		{
			LSQArchiveMime *mime = _types->data;
			LSQCommandBuilder *builder = mime->command_builders->data;
			if(!builder->build_add)
				m_types = g_slist_remove(m_types, mime);
			_types = g_slist_next(_types);
		}
	}
	
	return m_types;
}

static gint
lsq_lookup_mime(gconstpointer a, gconstpointer b)
{
	return !thunar_vfs_mime_info_equal((((LSQArchiveMime *)a)->mime_info), b);
}

gboolean
lsq_is_supported(const gchar *filename)
{
	ThunarVfsMimeInfo *mime_info = thunar_vfs_mime_database_get_info_for_name(lsq_mime_database, filename);
	GSList *result = g_slist_find_custom(lsq_mime_info_list, mime_info, lsq_lookup_mime);
	thunar_vfs_mime_info_unref(mime_info);
	if(result)
	{
		return TRUE;
	}
	return FALSE;
}

