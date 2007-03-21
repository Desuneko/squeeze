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
#include "dbus-command.h"
#include "archive.h"

static void
lsq_dbus_command_class_init(LSQDBusCommandClass *);
static void
lsq_dbus_command_init(LSQDBusCommand *);
static void
lsq_dbus_command_dispose(GObject *object);
static void
lsq_dbus_command_finalize(GObject *object);

static gboolean
lsq_dbus_command_execute(LSQArchiveCommand *command);

//static gint lsq_archive_command_signals[0];

static GObjectClass *parent_class;

GType
lsq_dbus_command_get_type ()
{
	static GType lsq_dbus_command_type = 0;

 	if (!lsq_dbus_command_type)
	{
 		static const GTypeInfo lsq_dbus_command_info = 
		{
			sizeof (LSQDBusCommandClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_dbus_command_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQDBusCommand),
			0,
			(GInstanceInitFunc) lsq_dbus_command_init,
			NULL
		};

		lsq_dbus_command_type = g_type_register_static (LSQ_TYPE_ARCHIVE_COMMAND, "LSQDBusCommand", &lsq_dbus_command_info, 0);
	}
	return lsq_dbus_command_type;
}

static void
lsq_dbus_command_class_init(LSQDBusCommandClass *dbus_command_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(dbus_command_class);

	object_class->dispose = lsq_dbus_command_dispose;
	object_class->finalize = lsq_dbus_command_finalize;

	parent_class = g_type_class_peek(LSQ_TYPE_ARCHIVE_COMMAND); 

}

static void
lsq_dbus_command_init(LSQDBusCommand *dbus_command)
{
	LSQArchiveCommand *command = LSQ_ARCHIVE_COMMAND(dbus_command);
	command->execute = lsq_dbus_command_execute;
}

/**
 * lsq_dbus_command_dispose:
 *
 * @object: LSQDBusCommand object
 *
 */
static void
lsq_dbus_command_dispose(GObject *object)
{
	parent_class->dispose(object);
}

/**
 * lsq_dbus_command_finalize:
 *
 * @object: LSQDBusCommand object
 *
 */
static void
lsq_dbus_command_finalize(GObject *object)
{
	parent_class->finalize(object);
}


/**
 * lsq_dbus_command_new:
 * @comment: a description, describing what the command does
 * @archive: the archive the command modifies
 * 
 * Returns: a new LSQDBusCommand object
 */
LSQArchiveCommand *
lsq_dbus_command_new(const gchar *comment, LSQArchive *archive)
{
	LSQArchiveCommand *archive_command;

	archive_command = g_object_new(lsq_dbus_command_get_type(), NULL);

	return archive_command;
}

static gboolean
lsq_dbus_command_execute(LSQArchiveCommand *command)
{
	g_critical("DBUS COMMAND NOT IMPLEMENTED");
	return FALSE;
}
