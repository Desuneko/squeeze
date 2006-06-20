/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
 *
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glib.h>
#include <glib-object.h>
#include <libintl.h>
#include "archive.h"
#include "archive-support.h"

#define _(String) gettext(String)

gint
lxa_archive_support_dummy(LXAArchive *archive);


void
lxa_archive_support_init(LXAArchiveSupport *support);
void
lxa_archive_support_class_init(LXAArchiveSupportClass *supportclass);

static guint lxa_archive_support_signals[2];

GType
lxa_archive_support_get_type ()
{
	static GType lxa_archive_support_type = 0;

 	if (!lxa_archive_support_type)
	{
 		static const GTypeInfo lxa_archive_support_info = 
		{
			sizeof (LXAArchiveSupportClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archive_support_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchiveSupport),
			0,
			(GInstanceInitFunc) lxa_archive_support_init,
		};

		lxa_archive_support_type = g_type_register_static (G_TYPE_OBJECT, "LXAArchiveSupport", &lxa_archive_support_info, 0);
	}
	return lxa_archive_support_type;
}

gint
lxa_archive_support_dummy(LXAArchive *archive)
{
	return -1;
}

void
lxa_archive_support_init(LXAArchiveSupport *support)
{
	support->add     = lxa_archive_support_dummy;
	support->extract = lxa_archive_support_dummy;
	support->remove  = lxa_archive_support_dummy;
	support->view    = lxa_archive_support_dummy;
}

void
lxa_archive_support_class_init(LXAArchiveSupportClass *supportclass)
{
	/*
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXAArchiveSupportClass *klass = LXA_ARCHIVE_SUPPORT_CLASS (supportclass);
	*/

	lxa_archive_support_signals[0] = g_signal_new("lxa_add_complete",
			G_TYPE_FROM_CLASS(supportclass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
	lxa_archive_support_signals[1] = g_signal_new("lxa_extract_complete",
			G_TYPE_FROM_CLASS(supportclass),
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

LXAArchiveSupport*
lxa_archive_support_new()
{
	LXAArchiveSupport*support;

	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT, NULL);
	
	return support;
}

void
lxa_archive_support_emit_signal(LXAArchiveSupport *support, guint signal_id, LXAArchive *archive)
{
	g_signal_emit(G_OBJECT(support), lxa_archive_support_signals[signal_id], 0, archive);
}

gint
lxa_archive_support_execute(gchar *command, LXAArchive *archive, GChildWatchFunc function, GIOFunc f_in, GIOFunc f_out, GIOFunc f_err)
{
	gchar **argvp;
	gint argcp;
	gint fd_in, fd_out, fd_err;
	GIOChannel *ioc_in, *ioc_out, *ioc_err;
	if (archive->child_pid)
		return 1;

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
	g_child_watch_add(archive->child_pid, function, archive);
	if(f_in)
	{
		ioc_in = g_io_channel_unix_new(fd_in);
		g_io_channel_set_encoding (ioc_in, "ISO8859-1" , NULL);
		g_io_channel_set_flags ( ioc_in, G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc_in, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_in, archive);
	}
	if(f_out)
	{
		ioc_out = g_io_channel_unix_new(fd_out);
		g_io_channel_set_encoding (ioc_out, "ISO8859-1" , NULL);
		g_io_channel_set_flags ( ioc_out , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_out, archive);
	}
	if(f_err)
	{
		ioc_err = g_io_channel_unix_new(fd_out);
		g_io_channel_set_encoding (ioc_err, "ISO8859-1" , NULL);
		g_io_channel_set_flags ( ioc_err , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc_err, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, f_err, archive);
	}
	return 0;
}
