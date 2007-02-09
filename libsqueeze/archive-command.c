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

#include "archive.h"
#include "archive-command.h"

static void
lsq_archive_command_class_init(LSQArchiveCommandClass *);
static void
lsq_archive_command_init(LSQArchiveCommand *archive);
static void
lsq_archive_command_dispose(GObject *object);

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

	parent_class = g_type_class_peek(G_TYPE_OBJECT); 

}

static void
lsq_archive_command_init(LSQArchiveCommand *archive_command)
{
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
	lsq_archive_dequeue(archive_command->archive, archive_command);
}

/**
 * lsq_archive_command_new:
 * @comment: a description, describing what the command does
 * @archive: the archive the command modifies
 * @command: a formatted string defining the command to be executed.
 *
 *
 * %%1$s is the application to be executed.
 *
 * %%2$s are the files to be appended
 * 
 * Returns: a new LSQArchiveCommand object
 */
LSQArchiveCommand *
lsq_archive_command_new(const gchar *comment, LSQArchive *archive, const gchar *command)
{
	LSQArchiveCommand *archive_command;

	archive_command = g_object_new(lsq_archive_command_get_type(), NULL);

	archive_command->command = g_strdup(command);
	archive_command->archive = archive;

	lsq_archive_enqueue(archive, archive_command);

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
	const gchar *files = g_object_get_data(G_OBJECT(archive_command), "files");
	const gchar *options = g_object_get_data(G_OBJECT(archive_command), "options");

	if(files == NULL)
		files = "";
	if(options == NULL)
		options = "";

	gchar *command = g_strdup_printf(archive_command->command, archive_command->archive->path, files, options);

	g_free(command);

	return TRUE;
}

gboolean
lsq_archive_command_stop(LSQArchiveCommand *command)
{
	return TRUE;
}
