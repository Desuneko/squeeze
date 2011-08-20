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
#include <gio/gio.h>

#include "libsqueeze-archive.h"
#include "libsqueeze-command.h"
#include "archive-iter.h"
#include "archive-command.h"
#include "command-builder.h"
#include "archive.h"

static void
lsq_archive_command_class_init(LSQArchiveCommandClass *);
static void
lsq_archive_command_init(LSQArchiveCommand *archive);
static void
lsq_archive_command_dispose(GObject *object);
static void
lsq_archive_command_finalize(GObject *object);

static GObjectClass *parent_class;

enum
{
	LSQ_ARCHIVE_COMMAND_SIGNAL_TERMINATED = 0,
	LSQ_ARCHIVE_COMMAND_SIGNAL_COUNT
};

static gint lsq_archive_command_signals[LSQ_ARCHIVE_COMMAND_SIGNAL_COUNT];

GType
lsq_archive_command_get_type (void)
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

	lsq_archive_command_signals[LSQ_ARCHIVE_COMMAND_SIGNAL_TERMINATED] = g_signal_new("terminated",
			G_TYPE_FROM_CLASS(archive_command_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
}

static void
lsq_archive_command_init(LSQArchiveCommand *archive_command)
{
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
	LSQArchiveCommand *command = LSQ_ARCHIVE_COMMAND(object);
	g_signal_emit(object, lsq_archive_command_signals[LSQ_ARCHIVE_COMMAND_SIGNAL_TERMINATED], 0, command->error, NULL);
	if(command->archive)
	{
		g_object_unref(command->archive);
		command->archive = NULL;
	}
	parent_class->dispose(object);
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
 * lsq_archive_command_executr:
 * @command: the archive_command to be executed
 *
 * Returns: true on success
 */
gboolean
lsq_archive_command_execute(LSQArchiveCommand *command)
{
#ifdef DEBUG
	g_return_val_if_fail(command->archive, FALSE);
	g_return_val_if_fail(LSQ_IS_ARCHIVE(command->archive), FALSE);
#endif /* DEBUG */

	return command->execute(command);
}

/**
 * lsq_archive_command_stop
 * @command:
 *
 * Returns: TRUE on success, FALSE if the command is not running
 */
gboolean
lsq_archive_command_stop(LSQArchiveCommand *command)
{
	return command->stop(command);
}

/**
 * lsq_archive_command_get_archive
 * @command:
 *
 * Returns: the associated archive
 */
LSQArchive *
lsq_archive_command_get_archive(LSQArchiveCommand *command)
{
	return command->archive;
}



/**
 * lsq_archive_command_get_comment
 * @command:
 *
 * Returns: the command comment describing what it is actually doing
 */
const gchar *
lsq_archive_command_get_comment(LSQArchiveCommand *archive_command)
{
	return archive_command->comment;
}

LSQArchiveCommand *
lsq_archive_command_new(const gchar *comment,
						LSQArchive *archive,
						LSQCommandFunc exec_command)
{
	LSQArchiveCommand *archive_command;

#ifdef DEBUG
	g_return_val_if_fail(archive, NULL);
	g_return_val_if_fail(LSQ_IS_ARCHIVE(archive), NULL);
#endif

	archive_command = g_object_new(LSQ_TYPE_ARCHIVE_COMMAND, NULL);

	g_object_ref(G_OBJECT(archive));
	archive_command->archive = archive;
	if(comment)
		archive_command->comment = g_strdup(comment);

	archive_command->execute = exec_command;

	return archive_command;
}
