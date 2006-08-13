
/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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

#define EXO_API_SUBJECT_TO_CHANGE

#include <glib.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>

#include "archive.h"
#include "archive-support.h"

#include "internals.h"

void
lxa_default_child_watch_func(GPid pid, gint status, gpointer data)
{
	LXAArchive *archive = data;
	archive->child_pid = 0;
	lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_IDLE);
}

gint
lxa_execute(gchar *command, LXAArchive *archive, GChildWatchFunc function, GIOFunc f_in, GIOFunc f_out, GIOFunc f_err)
{
	gchar **argvp;
	gint argcp;
	gint fd_in, fd_out, fd_err;
	GIOChannel *ioc_in, *ioc_out, *ioc_err;
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
#ifdef DEBUG
		g_debug("Adding watch to child");
#endif
		g_child_watch_add(archive->child_pid, function, archive);
	}
	else
	{
#ifdef DEBUG
		g_debug("Adding default watch to child");
#endif
		g_child_watch_add(archive->child_pid, lxa_default_child_watch_func, archive);
	}
	if(f_in)
	{
#ifdef DEBUG
		g_debug("Adding watch to stdin");
#endif
		ioc_in = g_io_channel_unix_new(fd_in);
		g_io_channel_set_encoding (ioc_in, "ISO8859-1" , NULL);
		g_io_channel_set_flags ( ioc_in, G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc_in, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_in, archive);
	}
	if(f_out)
	{
#ifdef DEBUG
		g_debug("Adding watch to stdout");
#endif
		ioc_out = g_io_channel_unix_new(fd_out);
		g_io_channel_set_encoding (ioc_out, NULL, NULL);
		g_io_channel_set_flags ( ioc_out , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_out, archive);
	}
	if(f_err)
	{
#ifdef DEBUG
		g_debug("Adding watch to stderr");
#endif
		ioc_err = g_io_channel_unix_new(fd_out);
//  g_io_channel_set_encoding (ioc_err, "ISO8859-1" , NULL);
		g_io_channel_set_flags ( ioc_err , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc_err, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_err, archive);
	}
	return 0;
}

gchar *
lxa_concat_filenames(GSList *filenames)
{
	GSList *_filenames = filenames;
	gchar *concat_str = " ";

	while(_filenames)
	{
		if(g_file_test(_filenames->data, G_FILE_TEST_EXISTS))
		{
			concat_str = g_strconcat(concat_str, " ", _filenames->data,  NULL);
		}
		_filenames = _filenames->next;
	}
	return concat_str;
}

