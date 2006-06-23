#include <glib.h>
#include <glib-object.h>

#include "archive.h"
#include "archive-support.h"
#include "compression-support.h"

#include "internals.h"

gint
lookup_archive_support( gconstpointer support , gconstpointer type)
{
	if(support == 0)
		return 1;
	if(((const LXAArchiveSupport *)support)->type == *(LXAArchiveType *)type)
		return 0;
	else
		return 1;
}

gint
lookup_compression_support( gconstpointer support , gconstpointer type)
{
	if(support == 0)
		return 1;
	if(((const LXACompressionSupport *)support)->type == *(LXACompressionType *)type)
		return 0;
	else
		return 1;
}

void
lxa_default_child_watch_func(GPid pid, gint status, gpointer data)
{
	LXAArchive *archive = data;
	archive->child_pid = 0;
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

	g_debug("Executing '%s'", command);
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
		g_debug("Adding watch to child");
		g_child_watch_add(archive->child_pid, function, archive);
	}
	else
	{
		g_debug("Adding default watch to child");
		g_child_watch_add(archive->child_pid, lxa_default_child_watch_func, archive);
	}
	if(f_in)
	{
		g_debug("Adding watch to stdin");
		ioc_in = g_io_channel_unix_new(fd_in);
		g_io_channel_set_encoding (ioc_in, "ISO8859-1" , NULL);
		g_io_channel_set_flags ( ioc_in, G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc_in, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_in, archive);
	}
	if(f_out)
	{
		g_debug("Adding watch to stdout");
		ioc_out = g_io_channel_unix_new(fd_out);
		g_io_channel_set_encoding (ioc_out, NULL, NULL);
		g_io_channel_set_flags ( ioc_out , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_out, archive);
	}
	if(f_err)
	{
		g_debug("Adding watch to stderr");
		ioc_err = g_io_channel_unix_new(fd_out);
//  g_io_channel_set_encoding (ioc_err, "ISO8859-1" , NULL);
		g_io_channel_set_flags ( ioc_err , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc_err, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_err, archive);
	}
	return 0;
}
