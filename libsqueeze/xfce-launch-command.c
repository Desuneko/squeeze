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
#include "libsqueeze-view.h"
#include "archive-iter.h"
#include "archive-command.h"
#include "xfce-launch-command.h"
#include "archive.h"

static void
lsq_xfce_launch_command_class_init(LSQXfceLaunchCommandClass *);
static void
lsq_xfce_launch_command_init(LSQXfceLaunchCommand *);
static void
lsq_xfce_launch_command_dispose(GObject *object);
static void
lsq_xfce_launch_command_finalize(GObject *object);

static gboolean
lsq_xfce_launch_command_execute(LSQArchiveCommand *command);

//static gint lsq_archive_command_signals[0];

static GObjectClass *parent_class;

GType
lsq_xfce_launch_command_get_type ()
{
	static GType lsq_xfce_launch_command_type = 0;

 	if (!lsq_xfce_launch_command_type)
	{
 		static const GTypeInfo lsq_xfce_launch_command_info = 
		{
			sizeof (LSQXfceLaunchCommandClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_xfce_launch_command_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQXfceLaunchCommand),
			0,
			(GInstanceInitFunc) lsq_xfce_launch_command_init,
			NULL
		};

		lsq_xfce_launch_command_type = g_type_register_static (LSQ_TYPE_ARCHIVE_COMMAND, "LSQXfceLaunchCommand", &lsq_xfce_launch_command_info, 0);
	}
	return lsq_xfce_launch_command_type;
}

static void
lsq_xfce_launch_command_class_init(LSQXfceLaunchCommandClass *xfce_launch_command_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(xfce_launch_command_class);

	object_class->dispose = lsq_xfce_launch_command_dispose;
	object_class->finalize = lsq_xfce_launch_command_finalize;

	parent_class = g_type_class_peek(LSQ_TYPE_ARCHIVE_COMMAND); 

}

static void
lsq_xfce_launch_command_init(LSQXfceLaunchCommand *xfce_launch_command)
{
	LSQArchiveCommand *command = LSQ_ARCHIVE_COMMAND(xfce_launch_command);
	command->execute = lsq_xfce_launch_command_execute;
}

/**
 * lsq_xfce_launch_command_dispose:
 *
 * @object: LSQXfceLaunchCommand object
 *
 */
static void
lsq_xfce_launch_command_dispose(GObject *object)
{
	parent_class->dispose(object);
}

/**
 * lsq_xfce_launch_command_finalize:
 *
 * @object: LSQXfceLaunchCommand object
 *
 */
static void
lsq_xfce_launch_command_finalize(GObject *object)
{
	parent_class->finalize(object);
}


/**
 * lsq_xfce_launch_command_new:
 * @comment: a description, describing what the command does
 * @archive: the archive the command modifies
 * 
 * Returns: a new LSQXfceLaunchCommand object
 */
LSQArchiveCommand *
lsq_xfce_launch_command_new(const gchar *comment, LSQArchive *archive, const gchar *prefix, GSList *files)
{
	GError *error = NULL;
	LSQArchiveCommand *archive_command;

	archive_command = g_object_new(lsq_xfce_launch_command_get_type(), NULL);

	LSQXfceLaunchCommand	*xfce_launch_command = LSQ_XFCE_LAUNCH_COMMAND(archive_command);

	xfce_launch_command->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

	xfce_launch_command->proxy = dbus_g_proxy_new_for_name(xfce_launch_command->connection, 
														   "org.xfce.FileManager",
														   "/org/xfce/FileManager",
														   "org.xfce.FileManager");
	
	g_object_ref(G_OBJECT(archive));
	archive_command->archive = archive;
	if(comment)
		archive_command->comment = g_strdup(comment);

	xfce_launch_command->files = g_strconcat(prefix, lsq_archive_iter_get_path(files->data), NULL);

	return archive_command;
}

static gboolean
lsq_xfce_launch_command_execute(LSQArchiveCommand *command)
{
	LSQXfceLaunchCommand *xfce_launch_command = LSQ_XFCE_LAUNCH_COMMAND(command);
	
	/* args */
	dbus_g_proxy_call(xfce_launch_command->proxy, "Launch", NULL, G_TYPE_STRING, xfce_launch_command->files, G_TYPE_STRING, "",  NULL);
	return TRUE;
}
