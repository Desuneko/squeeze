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

#include "libsqueeze-archive.h"
#include "archive-iter.h"
#include "archive-command.h"
#include "archive.h"

static void
lsq_archive_command_class_init(LSQArchiveCommandClass *);
static void
lsq_archive_command_init(LSQArchiveCommand *archive);
static void
lsq_archive_command_dispose(GObject *object);
static void
lsq_archive_command_finalize(GObject *object);

void
lsq_archive_command_child_watch_func(GPid pid, gint status, gpointer data);

gboolean
lsq_archive_command_parse_stdout(GIOChannel *ioc, GIOCondition cond, gpointer data);

//static gint lsq_archive_command_signals[0];

static GObjectClass *parent_class;

GType
lsq_archive_command_get_type ()
{
	static GType lsq_archive_command_type = 0;

 	if (!lsq_archive_command_type)
	{
 		static const GTypeInfo lsq_archive_command_info = 
		{
			sizeof (LSQArchiveCommandClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_command_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchiveCommand),
			0,
			(GInstanceInitFunc) lsq_archive_command_init,
			NULL
		};

		lsq_archive_command_type = g_type_register_static (G_TYPE_OBJECT, "LSQArchiveCommand", &lsq_archive_command_info, 0);
	}
	return lsq_archive_command_type;
}

static void
lsq_archive_command_class_init(LSQArchiveCommandClass *archive_command_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(archive_command_class);

	object_class->dispose = lsq_archive_command_dispose;
	object_class->finalize = lsq_archive_command_finalize;

	parent_class = g_type_class_peek(G_TYPE_OBJECT); 

}

static void
lsq_archive_command_init(LSQArchiveCommand *archive_command)
{
	archive_command->parse_stdout = NULL;
	archive_command->domain = g_quark_from_string("Command");
}

/**
 * lsq_archive_command_dispose:
 *
 * @object: LSQArchiveCommand object
 *
 */
static void
lsq_archive_command_dispose(GObject *object)
{
	LSQArchiveCommand *archive_command = LSQ_ARCHIVE_COMMAND(object);
	LSQArchiveCommand *next_archive_command = NULL;
	if(archive_command->archive)
	{
		lsq_archive_dequeue_command(archive_command->archive, archive_command);

		next_archive_command = lsq_archive_get_front_command(archive_command->archive);
		if(archive_command->refresh)
			lsq_archive_refreshed(archive_command->archive);
		lsq_archive_command_terminated(archive_command->archive, archive_command->error);
		archive_command->archive = NULL;
		if(next_archive_command)
		{
			lsq_archive_command_run(next_archive_command);
			g_object_unref(next_archive_command);
		}
	}
}

/**
 * lsq_archive_command_finalize:
 *
 * @object: LSQArchiveCommand object
 *
 */
static void
lsq_archive_command_finalize(GObject *object)
{
	LSQArchiveCommand *command = LSQ_ARCHIVE_COMMAND(object);
	if(command->error)
		g_error_free(command->error);
}


/**
 * lsq_archive_command_new:
 * @comment: a description, describing what the command does
 * @archive: the archive the command modifies
 * @command: a formatted string defining the command to be executed.
 * @safe: is it safe to terminate this child premature?
 *
 *
 * %%1$s is the application to be executed.
 *
 * %%2$s are the files to be appended
 *
 * %%3$s are any additional options
 * 
 * Returns: a new LSQArchiveCommand object
 */
LSQArchiveCommand *
lsq_archive_command_new(const gchar *comment, const gchar *command, gboolean safe, gboolean refresh)
{
	LSQArchiveCommand *archive_command;

	archive_command = g_object_new(lsq_archive_command_get_type(), NULL);

	archive_command->command = g_strdup(command);
	archive_command->safe = safe;
	archive_command->refresh = refresh;

	return archive_command;
}

/**
 * lsq_archive_command_run:
 * @archive_command: the archive_command to be run
 *
 * Returns: true on success
 */
gboolean
lsq_archive_command_run(LSQArchiveCommand *archive_command)
{
	gchar **argvp;
	gint argcp;
	gint fd_in, fd_out, fd_err;
	gchar *escaped_archive_path;

	g_return_val_if_fail(archive_command->child_pid == 0, FALSE);

	const gchar *files = g_object_get_data(G_OBJECT(archive_command), "files");
	const gchar *options = g_object_get_data(G_OBJECT(archive_command), "options");
	const gchar *archive_path = g_object_get_data(G_OBJECT(archive_command), "archive");

	if(files == NULL)
		files = "";
	if(options == NULL)
		options = "";
	
	if(archive_path)
	{
		escaped_archive_path = g_shell_quote(archive_path);
	}
	else
		escaped_archive_path = g_shell_quote(archive_command->archive->path);

	gchar *command = g_strdup_printf(archive_command->command, escaped_archive_path, files, options);

#ifdef DEBUG
	g_debug("%s\n", command);
#endif
	g_shell_parse_argv(command, &argcp, &argvp, NULL);
	if ( ! g_spawn_async_with_pipes (
			NULL,
			argvp,
			NULL,
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&(archive_command->child_pid),
			&fd_in,
			&fd_out,
			&fd_err,
			NULL) )
	{
		g_object_unref(archive_command);
		return FALSE;
	}

	g_object_ref(archive_command);
	g_child_watch_add(archive_command->child_pid, lsq_archive_command_child_watch_func, archive_command);

	if(archive_command->parse_stdout != NULL)
	{
		g_object_ref(archive_command);
		archive_command->ioc_out = g_io_channel_unix_new(fd_out);
		g_io_channel_set_encoding (archive_command->ioc_out, NULL, NULL);
		g_io_channel_set_flags (archive_command->ioc_out , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (archive_command->ioc_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, lsq_archive_command_parse_stdout, archive_command);
	}
	lsq_archive_command_started(archive_command->archive, archive_command->comment);
	g_free(escaped_archive_path);
	g_free(command);
	return TRUE;
}

/**
 * lsq_archive_command_stop
 * @archive_command:
 *
 *
 * Returns: TRUE on success, FALSE if the command is not running
 */
gboolean
lsq_archive_command_stop(LSQArchiveCommand *archive_command)
{
	if(archive_command->child_pid != 0)
		kill ( archive_command->child_pid , SIGHUP);
	else
		return FALSE; /* archive_command isn't running */
	return TRUE;
}

/**
 * lsq_archive_command_child_watch_func:
 * @pid:
 * @status:
 * @data:
 */
void
lsq_archive_command_child_watch_func(GPid pid, gint status, gpointer data)
{
	LSQArchiveCommand *command = LSQ_ARCHIVE_COMMAND(data);
	if(WIFEXITED(status))
	{
		if(WEXITSTATUS(status))
		{
			if(!command->error)
			{
				command->error = g_error_new(command->domain, status, _("Command exited with status %d."), status);
			}
		}
	}
	if(WIFSIGNALED(status))
	{
		switch(WTERMSIG(status))
		{
			case SIGHUP:
				if(!command->error)
					command->error = g_error_new_literal(command->domain, status, _("Command interrupted by user"));
				break;
			case SIGSEGV:
				if(!command->error)
					command->error = g_error_new_literal(command->domain, status, _("Command received SIGSEGV"));
				break;
			case SIGKILL:
			case SIGINT:
				if(!command->error)
					command->error = g_error_new_literal(command->domain, status, _("Command Terminated"));
				break;
		}
	}
	g_spawn_close_pid(pid);
	g_object_unref(G_OBJECT(data));
}

/**
 * lsq_archive_command_parse_stdout:
 * @ioc:
 * @cond:
 * @data:
 *
 * Returns:
 */
gboolean
lsq_archive_command_parse_stdout(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gint i = 0;
	LSQArchiveCommand *archive_command = LSQ_ARCHIVE_COMMAND(data);

	if(cond & (G_IO_PRI | G_IO_IN))
	{
		for(; i < 500; i++)
		{
			/* If parse_stdout returns FALSE, something seriously went wrong and we should cancel right away */
			if(archive_command->parse_stdout(archive_command) == FALSE)
			{
				cond |= G_IO_ERR;
			}
		}
	}
	if(cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		g_object_unref(archive_command);
		return FALSE; 
	}
	return TRUE;
}

gboolean
lsq_archive_command_set_parse_func(LSQArchiveCommand *archive_command, guint fd, LSQParseFunc func)
{
	switch(fd)
	{
		case 1:
			archive_command->parse_stdout = func;
		default:
			break;
	}
	return TRUE;
}

GIOStatus
lsq_archive_command_read_line(LSQArchiveCommand *archive_command, guint fd, gchar **line, gsize *length, GError **error)
{
	GIOStatus status = G_IO_STATUS_EOF;
	switch(fd)
	{
		case 1:
			status = g_io_channel_read_line(archive_command->ioc_out, line, length, NULL, error);
			break;
		default:
			break;
	}
	return status;
}

GIOStatus
lsq_archive_command_read_bytes(LSQArchiveCommand *archive_command, guint fd, gchar *buf, gsize max_length, gsize *length, GError **error)
{
	GIOStatus status = G_IO_STATUS_EOF;
	switch(fd)
	{
		case 1:
			status = g_io_channel_read_chars(archive_command->ioc_out, buf, max_length, length, error);
			break;
		default:
			break;
	}
	return status;
}

const gchar *
lsq_archive_command_get_comment(LSQArchiveCommand *archive_command)
{
	return archive_command->comment;
}
