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
#include "libsqueeze-command.h"
#include "libsqueeze-module.h"
#include "archive-iter.h"
#include "command.h"
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
 * lsq_archive_command_run:
 * @command: the archive_command to be run
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
