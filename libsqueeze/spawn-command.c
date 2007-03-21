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
#include "archive-iter.h"
#include "archive.h"
#include "archive-command.h"
#include "spawn-command.h"

static void
lsq_spawn_command_class_init(LSQSpawnCommandClass *);
static void
lsq_spawn_command_init(LSQSpawnCommand *);
static void
lsq_spawn_command_dispose(GObject *object);
static void
lsq_spawn_command_finalize(GObject *object);

static gboolean
lsq_spawn_command_execute(LSQArchiveCommand *command);
static gboolean
lsq_spawn_command_stop(LSQArchiveCommand *command);

void
lsq_spawn_command_child_watch_func(GPid pid, gint status, gpointer data);

gboolean
lsq_spawn_command_parse_stdout(GIOChannel *, GIOCondition , gpointer );

//static gint lsq_archive_command_signals[0];

static GObjectClass *parent_class;

GType
lsq_spawn_command_get_type ()
{
	static GType lsq_spawn_command_type = 0;

 	if (!lsq_spawn_command_type)
	{
 		static const GTypeInfo lsq_spawn_command_info = 
		{
			sizeof (LSQSpawnCommandClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_spawn_command_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQSpawnCommand),
			0,
			(GInstanceInitFunc) lsq_spawn_command_init,
			NULL
		};

		lsq_spawn_command_type = g_type_register_static (LSQ_TYPE_ARCHIVE_COMMAND, "LSQSpawnCommand", &lsq_spawn_command_info, 0);
	}
	return lsq_spawn_command_type;
}

static void
lsq_spawn_command_class_init(LSQSpawnCommandClass *spawn_command_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(spawn_command_class);

	object_class->dispose = lsq_spawn_command_dispose;
	object_class->finalize = lsq_spawn_command_finalize;

	parent_class = g_type_class_peek(LSQ_TYPE_ARCHIVE_COMMAND); 

}

static void
lsq_spawn_command_init(LSQSpawnCommand *spawn_command)
{
	LSQArchiveCommand *command = LSQ_ARCHIVE_COMMAND(spawn_command);

	command->execute = lsq_spawn_command_execute;
	command->stop = lsq_spawn_command_stop;

	spawn_command->parse_stdout = NULL;
}

/**
 * lsq_spawn_command_dispose:
 *
 * @object: LSQSpawnCommand object
 *
 */
static void
lsq_spawn_command_dispose(GObject *object)
{
	parent_class->dispose(object);
}

/**
 * lsq_spawn_command_finalize:
 *
 * @object: LSQSpawnCommand object
 *
 */
static void
lsq_spawn_command_finalize(GObject *object)
{
	parent_class->finalize(object);
}


/**
 * lsq_spawn_command_new:
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
lsq_spawn_command_new(const gchar *comment, 
                      LSQArchive *archive, 
                      const gchar *command, 
                      const gchar *files, 
                      const gchar *options, 
                      const gchar *archive_path)
{
	LSQArchiveCommand *archive_command;

#ifdef DEBUG
	g_return_val_if_fail(archive, NULL);
	g_return_val_if_fail(LSQ_IS_ARCHIVE(archive), NULL);
#endif

	archive_command = g_object_new(lsq_spawn_command_get_type(), NULL);

	LSQ_SPAWN_COMMAND(archive_command)->command = g_strdup(command);
	if(files)
		LSQ_SPAWN_COMMAND(archive_command)->files = g_strdup(files);
	if(options)
		LSQ_SPAWN_COMMAND(archive_command)->options = g_strdup(options);
	if(archive_path)
		LSQ_SPAWN_COMMAND(archive_command)->archive_path = g_strdup(archive_path);
	else
		LSQ_SPAWN_COMMAND(archive_command)->archive_path = g_strdup(lsq_archive_get_path(archive));

	archive_command->archive = archive;
	if(comment)
		archive_command->comment = g_strdup(comment);

	return archive_command;
}


static gboolean
lsq_spawn_command_execute(LSQArchiveCommand *command)
{
	gchar **argvp;
	gint argcp;
	gint fd_in, fd_out, fd_err;
	LSQSpawnCommand *spawn_command = LSQ_SPAWN_COMMAND(command);

	gchar *escaped_archive_path = g_shell_quote(spawn_command->archive_path);

	gchar *cmd = g_strdup_printf(spawn_command->command, escaped_archive_path, spawn_command->files, spawn_command->options);


	g_shell_parse_argv(cmd, &argcp, &argvp, NULL);
	if ( ! g_spawn_async_with_pipes (
			NULL,
			argvp,
			NULL,
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&(spawn_command->child_pid),
			&fd_in,
			&fd_out,
			&fd_err,
			NULL) )
	{
		g_object_unref(spawn_command);
		return FALSE;
	}
	LSQ_ARCHIVE_COMMAND(command)->running = TRUE;

	g_child_watch_add(spawn_command->child_pid, lsq_spawn_command_child_watch_func, spawn_command);

	if(spawn_command->parse_stdout != NULL)
	{
		g_object_ref(spawn_command);
		spawn_command->ioc_out = g_io_channel_unix_new(fd_out);
		g_io_channel_set_encoding (spawn_command->ioc_out, NULL, NULL);
		g_io_channel_set_flags (spawn_command->ioc_out , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (spawn_command->ioc_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, lsq_spawn_command_parse_stdout, spawn_command);
	}
	g_free(escaped_archive_path);
	g_free(cmd);
	return TRUE;
}

static gboolean
lsq_spawn_command_stop(LSQArchiveCommand *command)
{
	LSQSpawnCommand *spawn_command = LSQ_SPAWN_COMMAND(command);
	if(command->running)
	{
		if(spawn_command->child_pid != 0)
			kill ( spawn_command->child_pid , SIGHUP);
		else
			return FALSE; /* command isn't running */
	}
	else
		return FALSE; /* command isn't running */
	return TRUE;
}

/**
 * lsq_spawn_command_child_watch_func:
 * @pid:
 * @status:
 * @data:
 */
void
lsq_spawn_command_child_watch_func(GPid pid, gint status, gpointer data)
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
 * lsq_spawn_command_parse_stdout:
 * @ioc:
 * @cond:
 * @data:
 *
 * Returns:
 */
gboolean
lsq_spawn_command_parse_stdout(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gint i = 0;
	LSQArchiveCommand *archive_command = LSQ_ARCHIVE_COMMAND(data);
	LSQSpawnCommand *spawn_command = LSQ_SPAWN_COMMAND(data);

	if(cond & (G_IO_PRI | G_IO_IN))
	{
		for(; i < 500; i++)
		{
			/* If parse_stdout returns FALSE, something seriously went wrong and we should cancel right away */
			if(spawn_command->parse_stdout(spawn_command, archive_command->user_data) == FALSE)
			{
				cond |= G_IO_ERR;
			}
		}
	}
	if(cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		g_object_unref(spawn_command);
		return FALSE; 
	}
	return TRUE;
}

gboolean
lsq_spawn_command_set_parse_func(LSQSpawnCommand *spawn_command, guint fd, LSQParseFunc func, gpointer user_data)
{
	LSQArchiveCommand *archive_command = LSQ_ARCHIVE_COMMAND(spawn_command);
	switch(fd)
	{
		case 1:
			spawn_command->parse_stdout = func;
			archive_command->user_data = user_data;
		default:
			break;
	}
	return TRUE;
}

GIOStatus
lsq_spawn_command_read_line(LSQSpawnCommand *spawn_command, guint fd, gchar **line, gsize *length, GError **error)
{
	GIOStatus status = G_IO_STATUS_EOF;
	switch(fd)
	{
		case 1:
			status = g_io_channel_read_line(spawn_command->ioc_out, line, length, NULL, error);
			break;
		default:
			break;
	}
	return status;
}

GIOStatus
lsq_spawn_command_read_bytes(LSQSpawnCommand *spawn_command, guint fd, gchar *buf, gsize max_length, gsize *length, GError **error)
{
	GIOStatus status = G_IO_STATUS_EOF;
	switch(fd)
	{
		case 1:
			status = g_io_channel_read_chars(spawn_command->ioc_out, buf, max_length, length, error);
			break;
		default:
			break;
	}
	return status;
}
