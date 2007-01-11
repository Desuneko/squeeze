
/*
 *  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
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
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>

#include "archive.h"
#include "archive-support.h"

#include "internals.h"

#define __USE_GNU

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

static gint
lsq_opened_archives_lookup_archive(gconstpointer open_archive, gconstpointer path);


void
lsq_default_child_watch_func(GPid pid, gint status, gpointer data)
{
	LSQArchive *archive = data;
	archive->child_pid = 0;
	if(archive->old_status == LSQ_ARCHIVESTATUS_REFRESH && archive->status == LSQ_ARCHIVESTATUS_USERBREAK)
		g_object_unref(archive);
	else
	{
		g_spawn_close_pid(pid);
		if(WIFEXITED(status))
		{
			if(WEXITSTATUS(status))
			{
				lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_ERROR);
			}
			else
			{
				if(archive->status == LSQ_ARCHIVESTATUS_ADD)
				{
					if(!archive->file_info)
					{
						archive->file_info = thunar_vfs_info_new_for_path(archive->path_info, NULL);
						if(archive->file_info)
						{
							thunar_vfs_mime_info_unref(archive->mime_info);
							archive->mime_info = archive->file_info->mime_info;
							thunar_vfs_mime_info_ref(archive->mime_info);
						}
					}
				}
				if(archive->status != LSQ_ARCHIVESTATUS_REFRESH)
					lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_IDLE);
			}
		}
		else
			lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_IDLE);
	}
}

gint
lsq_execute(gchar *command, LSQArchive *archive, GChildWatchFunc function, GIOFunc f_in, GIOFunc f_out, GIOFunc f_err)
{
	gchar **argvp;
	gint argcp;
	gint fd_in, fd_out, fd_err;
	if (archive->child_pid)
		return 1;

#ifdef DEBUG
	g_debug("Executing '%s'", command);
#endif
	g_shell_parse_argv(command, &argcp, &argvp, NULL);
	if ( ! g_spawn_async_with_pipes (
			NULL,
			argvp,
			NULL,
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&(archive->child_pid),
			&fd_in,
			&fd_out,
			&fd_err,
			NULL) )
		return 1;
	if(function)
	{
		g_child_watch_add(archive->child_pid, function, archive);
	}
	else
	{
		g_child_watch_add(archive->child_pid, lsq_default_child_watch_func, archive);
	}
	if(f_in)
	{
		archive->ioc_in = g_io_channel_unix_new(fd_in);
		g_io_channel_set_encoding (archive->ioc_in, "ISO8859-1" , NULL);
		g_io_channel_set_flags ( archive->ioc_in, G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (archive->ioc_in, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_in, archive);
	}
	if(f_out)
	{
		archive->ioc_out = g_io_channel_unix_new(fd_out);
		g_io_channel_set_encoding (archive->ioc_out, NULL, NULL);
		g_io_channel_set_flags (archive->ioc_out , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (archive->ioc_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_out, archive);
	}
	if(f_err)
	{
		archive->ioc_err = g_io_channel_unix_new(fd_out);
//  g_io_channel_set_encoding (ioc_err, "ISO8859-1" , NULL);
		g_io_channel_set_flags (archive->ioc_err , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (archive->ioc_err, G_IO_IN|G_IO_PRI|G_IO_NVAL, f_err, archive);
	}
	return 0;
}

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
