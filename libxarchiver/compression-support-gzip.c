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
#include "compression-support.h"
#include "compression-support-gzip.h"

#include "archive-support.h"

#include "internals.h"

#define _(String) gettext(String)

gboolean
lxa_compression_support_gzip_parse_output_compress(GIOChannel *ioc, GIOCondition cond, gpointer data);

gboolean
lxa_compression_support_gzip_parse_output_decompress(GIOChannel *ioc, GIOCondition cond, gpointer data);

void
lxa_compression_support_gzip_init(LXACompressionSupportGzip *support);
void
lxa_compression_support_gzip_class_init(LXACompressionSupportGzipClass *supportclass);

GType
lxa_compression_support_gzip_get_type ()
{
	static GType lxa_compression_support_gzip_type = 0;

 	if (!lxa_compression_support_gzip_type)
	{
 		static const GTypeInfo lxa_compression_support_gzip_info = 
		{
			sizeof (LXACompressionSupportGzipClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_compression_support_gzip_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXACompressionSupportGzip),
			0,
			(GInstanceInitFunc) lxa_compression_support_gzip_init,
		};

		lxa_compression_support_gzip_type = g_type_register_static (LXA_TYPE_COMPRESSION_SUPPORT, "LXACompressionSupportGzip", &lxa_compression_support_gzip_info, 0);
	}
	return lxa_compression_support_gzip_type;
}

gint
lxa_compression_support_gzip_compress(LXAArchive *archive)
{
	g_debug("Compressing gzip");
	if(!archive->tmp_file)
	{
		g_critical("compression tried but no tmp_file specified");
		return 1;
	}

	gchar **argvp;
	gint argcp;
	gchar *command = g_strconcat("gzip -c ", archive->tmp_file, NULL);
	gint child_pid;

	GIOChannel *ioc;
	gint out_fd;
	GError *error = NULL;

	g_unlink(archive->path);

	g_shell_parse_argv(command, &argcp, &argvp, NULL);
	if ( ! g_spawn_async_with_pipes (
			NULL,
			argvp,
			NULL,
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&child_pid,
			NULL,
			&out_fd,
			NULL,
			NULL) )
		return 1;
	ioc = g_io_channel_unix_new(out_fd);
	g_io_channel_set_encoding(ioc, NULL, &error);
	g_io_channel_set_flags(ioc, G_IO_FLAG_NONBLOCK, &error);
	g_io_add_watch(ioc, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, lxa_compression_support_gzip_parse_output_compress, archive);
	return 0;
}

gint
lxa_compression_support_gzip_decompress(LXAArchive *archive)
{
	g_debug("Decompressing bzip2");
	if(!archive->tmp_file)
	{
		g_critical("decompression tried but no tmp_file specified");
		return 1;
	}
	gchar **argvp;
	gint argcp;
	gchar *command = g_strconcat("gzip -dc ", archive->path, NULL);
	gint child_pid;

	GIOChannel *ioc;
	gint out_fd;
	GError *error = NULL;

	g_shell_parse_argv(command, &argcp, &argvp, NULL);
	if ( ! g_spawn_async_with_pipes (
			NULL,
			argvp,
			NULL,
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&child_pid,
			NULL,
			&out_fd,
			NULL,
			NULL) )
		return 1;
	ioc = g_io_channel_unix_new(out_fd);
	g_io_channel_set_encoding(ioc, NULL, &error);
	g_io_channel_set_flags(ioc, G_IO_FLAG_NONBLOCK, &error);
	g_io_add_watch(ioc, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, lxa_compression_support_gzip_parse_output_decompress, archive);
	return 0;
}

void
lxa_compression_support_gzip_init(LXACompressionSupportGzip *support)
{
	LXA_COMPRESSION_SUPPORT(support)->id = "gzip";
	LXA_COMPRESSION_SUPPORT(support)->type = LXA_COMPRESSIONTYPE_GZIP;
	LXA_COMPRESSION_SUPPORT(support)->compress = lxa_compression_support_gzip_compress;
	LXA_COMPRESSION_SUPPORT(support)->decompress = lxa_compression_support_gzip_decompress;
}

void
lxa_compression_support_gzip_class_init(LXACompressionSupportGzipClass *supportclass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXACompressionSupportGzipClass *klass = LXA_COMPRESSION_SUPPORT_GZIP_CLASS (supportclass);
}

LXACompressionSupport*
lxa_compression_support_gzip_new()
{
	LXACompressionSupportGzip* support;

	support = g_object_new(LXA_TYPE_COMPRESSION_SUPPORT_GZIP, NULL);
	
	return LXA_COMPRESSION_SUPPORT(support);
}

gboolean
lxa_compression_support_gzip_parse_output_decompress(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	GSList *find_result;
	LXAArchiveSupport *archive_support;
	FILE *out_file = NULL;
	LXAArchive *archive = data;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 0;
	GError *error = NULL;

	if(cond & (G_IO_PRI | G_IO_IN))
	{
		out_file = fopen(archive->tmp_file, "ab");
		if(!out_file)
			g_critical("Could not open file");

		while(g_io_channel_read_chars(ioc, buf, 1024, &read, &error) == G_IO_STATUS_NORMAL)
		{
			if(read)
			{
				fwrite(buf, 1, read, out_file);
			}
			read = 0;
		}
		fclose(out_file);
	}
	g_free(buf);
	if(cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_debug("shutting down ioc");
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		if(!(cond & G_IO_ERR))
		{
			find_result = g_slist_find_custom(lxa_archive_support_list, &(archive->type), lookup_archive_support);
			if(find_result)
			{
				archive_support = find_result->data;
				switch(archive->status)
				{
					case LXA_ARCHIVESTATUS_ADD:
						archive_support->add(archive);
						break;
					case LXA_ARCHIVESTATUS_EXTRACT:
						archive_support->extract(archive);
						break;
					case LXA_ARCHIVESTATUS_REMOVE:
						archive_support->remove(archive);
						break;
				}
			}
		}
		return FALSE;
	}
	return TRUE;
}

gboolean
lxa_compression_support_gzip_parse_output_compress(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	FILE *out_file = NULL;
	LXAArchive *archive = data;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 0;
	GError *error = NULL;

	if(cond & (G_IO_PRI | G_IO_IN))
	{
		out_file = fopen(archive->path, "ab");
		if(!out_file)
			g_critical("Could not open file");

		while(g_io_channel_read_chars(ioc, buf, 1024, &read, &error) == G_IO_STATUS_NORMAL)
		{
			if(read)
			{
				fwrite(buf, 1, read, out_file);
			}
			read = 0;
		}
		fclose(out_file);
	}
	g_free(buf);
	if(cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_debug("shutting down ioc");
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_IDLE);
		return FALSE;
	}
	return TRUE;
}
