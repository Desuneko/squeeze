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
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze-archive.h"
#include "libsqueeze-view.h"
#include "libsqueeze-module.h"
#include "archive-iter.h"
#include "archive-command.h"
#include "archive.h"
#include "command-builder.h"

#include "internals.h"

#define __USE_GNU

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

static gint
lsq_opened_archives_lookup_archive(gconstpointer open_archive, gconstpointer path);

gchar *
lsq_concat_filenames(GSList *filenames)
{
	GSList *_filenames = filenames;
	gchar *concat_str = g_strdup(" "), *_concat_str;

	while(_filenames)
	{
		_concat_str = concat_str;
		concat_str = g_strconcat(concat_str, " ", g_shell_quote(_filenames->data) , NULL);
		_filenames = _filenames->next;
		g_free(_concat_str);
	}
	return concat_str;
}

gchar *
lsq_concat_iter_filenames(GSList *file_iters)
{
	GSList *_file_iters = file_iters;
	gchar *concat_str = g_strdup(" "), *_concat_str;

	while(_file_iters)
	{
		gchar *path = lsq_archive_iter_get_path(_file_iters->data);
		_concat_str = concat_str;
		concat_str = g_strconcat(_concat_str, " ", g_shell_quote(path) , NULL);
		_file_iters = _file_iters->next;
		g_free(_concat_str);
		g_free(path);
	}
	return concat_str;
}

LSQArchive *
lsq_opened_archive_get_archive(gchar *path)
{
	GSList *result = g_slist_find_custom(lsq_opened_archive_list, path, lsq_opened_archives_lookup_archive);
	if(result)
	{
		g_object_ref(result->data);
		return result->data;
	}
	return NULL;
}


static gint
lsq_opened_archives_lookup_archive(gconstpointer open_archive, gconstpointer path)
{
#ifdef DEBUG
	g_return_val_if_fail(open_archive, 1);
#endif
	ThunarVfsPath *path_info = NULL;
	if(g_path_is_absolute(path))
		path_info = thunar_vfs_path_new(path, NULL);
	else
		path_info = thunar_vfs_path_relative(lsq_relative_base_path, path);

	if(thunar_vfs_path_equal(((LSQArchive *)open_archive)->path_info, path_info))
	{
		if(path_info)
			thunar_vfs_path_unref(path_info);
		return 0;
	}
	else
	{
		if(path_info)
			thunar_vfs_path_unref(path_info);
		return 1;
	}
}
