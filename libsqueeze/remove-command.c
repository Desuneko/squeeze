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
#include "archive-command.h"
#include "remove-command.h"
#include "archive.h"

static void
lsq_remove_command_class_init(LSQRemoveCommandClass *);
static void
lsq_remove_command_init(LSQRemoveCommand *);
static void
lsq_remove_command_dispose(GObject *object);
static void
lsq_remove_command_finalize(GObject *object);

static gboolean
lsq_remove_command_execute(LSQArchiveCommand *command);
static gboolean
lsq_remove_command_stop(LSQArchiveCommand *command);

static GObjectClass *parent_class;

GType
lsq_remove_command_get_type ()
{
	static GType lsq_remove_command_type = 0;

 	if (!lsq_remove_command_type)
	{
 		static const GTypeInfo lsq_remove_command_info = 
		{
			sizeof (LSQRemoveCommandClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_remove_command_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQRemoveCommand),
			0,
			(GInstanceInitFunc) lsq_remove_command_init,
			NULL
		};

		lsq_remove_command_type = g_type_register_static (LSQ_TYPE_ARCHIVE_COMMAND, "LSQRemoveCommand", &lsq_remove_command_info, 0);
	}
	return lsq_remove_command_type;
}

static void
lsq_remove_command_class_init(LSQRemoveCommandClass *remove_command_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(remove_command_class);

	object_class->dispose = lsq_remove_command_dispose;
	object_class->finalize = lsq_remove_command_finalize;

	parent_class = g_type_class_peek(LSQ_TYPE_ARCHIVE_COMMAND); 

}

static void
lsq_remove_command_init(LSQRemoveCommand *remove_command)
{
	LSQArchiveCommand *command = LSQ_ARCHIVE_COMMAND(remove_command);

	command->execute = lsq_remove_command_execute;
	command->stop = lsq_remove_command_stop;
}

/**
 * lsq_remove_command_dispose:
 *
 * @object: LSQRemoveCommand object
 *
 */
static void
lsq_remove_command_dispose(GObject *object)
{
	GSList *iters = g_object_get_data(object, "entries");
	lsq_iter_slist_free(iters);
	g_object_set_data(object, "entries", NULL);

	parent_class->dispose(object);
}

/**
 * lsq_remove_command_finalize:
 *
 * @object: LSQRemoveCommand object
 *
 */
static void
lsq_remove_command_finalize(GObject *object)
{
	parent_class->finalize(object);
}


/**
 * lsq_remove_command_new:
 * @comment: a description, describing what the command does
 * @archive: the archive the command modifies
 * 
 * Returns: a new LSQRemoveCommand object
 */
LSQArchiveCommand *
lsq_remove_command_new(const gchar *comment, LSQArchive *archive, GSList *iters)
{
	LSQArchiveCommand *archive_command;

	archive_command = g_object_new(LSQ_TYPE_REMOVE_COMMAND, NULL);

	g_object_ref(G_OBJECT(archive));
	archive_command->archive = archive;

	if(comment)
		archive_command->comment = g_strdup(comment);

	g_object_set_data(G_OBJECT(archive_command), "entries", lsq_iter_slist_copy(iters));

	archive_command->execute = lsq_remove_command_execute;

	return archive_command;
}

static gboolean
lsq_remove_command_execute(LSQArchiveCommand *command)
{
	GSList *iters = g_object_get_data(G_OBJECT(command), "entries");
	GSList *iter;
	
	for(iter = iters; iter; iter = iter->next)
	{
		lsq_archive_iter_remove(iter->data, TRUE);
	}

	return TRUE;
}

static gboolean
lsq_remove_command_stop(LSQArchiveCommand *command)
{
	return TRUE;
}
