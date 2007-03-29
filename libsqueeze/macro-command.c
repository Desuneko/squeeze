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
#include "macro-command.h"
#include "archive.h"

static void
lsq_macro_command_class_init(LSQMacroCommandClass *);
static void
lsq_macro_command_init(LSQMacroCommand *);
static void
lsq_macro_command_dispose(GObject *object);
static void
lsq_macro_command_finalize(GObject *object);

static gboolean
lsq_macro_command_execute(LSQArchiveCommand *command);
static gboolean
lsq_macro_command_stop(LSQArchiveCommand *command);

static void
cb_macro_command_sub_command_terminated(LSQArchiveCommand *, GError *, LSQMacroCommand *);

static GObjectClass *parent_class;

GType
lsq_macro_command_get_type ()
{
	static GType lsq_macro_command_type = 0;

 	if (!lsq_macro_command_type)
	{
 		static const GTypeInfo lsq_macro_command_info = 
		{
			sizeof (LSQMacroCommandClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_macro_command_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQMacroCommand),
			0,
			(GInstanceInitFunc) lsq_macro_command_init,
			NULL
		};

		lsq_macro_command_type = g_type_register_static (LSQ_TYPE_ARCHIVE_COMMAND, "LSQMacroCommand", &lsq_macro_command_info, 0);
	}
	return lsq_macro_command_type;
}

static void
lsq_macro_command_class_init(LSQMacroCommandClass *macro_command_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(macro_command_class);

	object_class->dispose = lsq_macro_command_dispose;
	object_class->finalize = lsq_macro_command_finalize;

	parent_class = g_type_class_peek(LSQ_TYPE_ARCHIVE_COMMAND); 

}

static void
lsq_macro_command_init(LSQMacroCommand *macro_command)
{
	LSQArchiveCommand *command = LSQ_ARCHIVE_COMMAND(macro_command);

	command->execute = lsq_macro_command_execute;
	command->stop = lsq_macro_command_stop;
}

/**
 * lsq_macro_command_dispose:
 *
 * @object: LSQMacroCommand object
 *
 */
static void
lsq_macro_command_dispose(GObject *object)
{
	parent_class->dispose(object);
}

/**
 * lsq_macro_command_finalize:
 *
 * @object: LSQMacroCommand object
 *
 */
static void
lsq_macro_command_finalize(GObject *object)
{
	parent_class->finalize(object);
}


/**
 * lsq_macro_command_new:
 * @archive: the archive the command modifies
 * 
 * Returns: a new LSQMacroCommand object
 */
LSQArchiveCommand *
lsq_macro_command_new(LSQArchive *archive)
{
	LSQArchiveCommand *archive_command;

	archive_command = g_object_new(LSQ_TYPE_MACRO_COMMAND, NULL);

	archive_command->archive = archive;

	return archive_command;
}

static gboolean
lsq_macro_command_execute(LSQArchiveCommand *command)
{
	g_object_ref(command);
	LSQMacroCommand *macro_command = LSQ_MACRO_COMMAND(command);
	g_signal_connect(macro_command->command_queue->data, "terminated", G_CALLBACK(cb_macro_command_sub_command_terminated), command);

	gboolean retval = lsq_archive_command_execute(macro_command->command_queue->data);

	g_object_unref(macro_command->command_queue->data);

	return retval;
}

static gboolean
lsq_macro_command_stop(LSQArchiveCommand *command)
{
	return TRUE;
}

void
lsq_macro_command_append(LSQMacroCommand *command, LSQArchiveCommand *sub_command)
{
	if(!command->command_queue)
		LSQ_ARCHIVE_COMMAND(command)->comment = sub_command->comment;
	g_object_ref(sub_command);
	command->command_queue = g_slist_append(command->command_queue, sub_command);
}

static void
cb_macro_command_sub_command_terminated(LSQArchiveCommand *sub_command, GError *error, LSQMacroCommand *macro_command)
{
	macro_command->command_queue = g_slist_remove(macro_command->command_queue, sub_command);
	if(macro_command->command_queue)
	{
		LSQArchiveCommand *command = macro_command->command_queue->data;

		g_signal_connect(command, "terminated", G_CALLBACK(cb_macro_command_sub_command_terminated), macro_command);

		LSQ_ARCHIVE_COMMAND(macro_command)->comment = command->comment;

		lsq_archive_state_changed(LSQ_ARCHIVE_COMMAND(macro_command)->archive);

		if(!lsq_archive_command_execute(command))
		{
			g_signal_handlers_disconnect_by_func(command, cb_macro_command_sub_command_terminated, macro_command);
			/* And here i kind of have no idea if this works */	
			GSList *termination_queue = macro_command->command_queue;
			macro_command->command_queue = NULL;
			g_slist_foreach(termination_queue, (GFunc)g_object_unref, NULL);
			g_slist_free(termination_queue);
			g_object_unref(macro_command);
		}
		else
		{
			g_object_unref(command);
		}
	}
	else
	{
		g_object_unref(macro_command);
	}
}
