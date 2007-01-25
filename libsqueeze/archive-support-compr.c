/*
 *  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>

#include "archive.h"
#include "archive-support.h"
#include "archive-support-compr.h"

#include "internals.h"

#define LSQ_ARCHIVE_DEST_FILE "compr_dest_file"

static void
lsq_archive_support_compr_init(LSQArchiveSupportCompr *support);
static void
lsq_archive_support_compr_class_init(LSQArchiveSupportComprClass *supportclass);

void
lsq_archive_support_compr_passive_watch(GPid pid, gint status, gpointer data);

gboolean
lsq_archive_support_compr_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);

static void
lsq_archive_support_compr_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
lsq_archive_support_compr_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gint lsq_archive_support_compr_extract(LSQArchive *, const gchar *, GSList *);
static gint lsq_archive_support_compr_refresh(LSQArchive *);

static gboolean
lsq_archive_support_compr_decompress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);

GType
lsq_archive_support_compr_get_type ()
{
	static GType lsq_archive_support_compr_type = 0;

 	if (!lsq_archive_support_compr_type)
	{
 		static const GTypeInfo lsq_archive_support_compr_info = 
		{
			sizeof (LSQArchiveSupportComprClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_support_compr_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchiveSupportCompr),
			0,
			(GInstanceInitFunc) lsq_archive_support_compr_init,
		};

		lsq_archive_support_compr_type = g_type_register_static (LSQ_TYPE_ARCHIVE_SUPPORT, "LSQArchiveSupportCompr", &lsq_archive_support_compr_info, 0);
	}
	return lsq_archive_support_compr_type;
}

static void
lsq_archive_support_compr_init(LSQArchiveSupportCompr *support)
{
	LSQArchiveSupport *archive_support = LSQ_ARCHIVE_SUPPORT(support);

	archive_support->id = "Compr";
	archive_support->max_n_files = 1;

	lsq_archive_support_add_mime(archive_support, "application/x-gzip");
	lsq_archive_support_add_mime(archive_support, "application/x-bzip");

	archive_support->extract = lsq_archive_support_compr_extract;
	archive_support->refresh = lsq_archive_support_compr_refresh;
}

static void
lsq_archive_support_compr_class_init(LSQArchiveSupportComprClass *supportclass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (supportclass);

	object_class->set_property = lsq_archive_support_compr_set_property;
	object_class->get_property = lsq_archive_support_compr_get_property;
}

LSQArchiveSupport*
lsq_archive_support_compr_new()
{
	LSQArchiveSupportCompr *support;

	support = g_object_new(LSQ_TYPE_ARCHIVE_SUPPORT_COMPR,
												 NULL);
	
	return LSQ_ARCHIVE_SUPPORT(support);
}

/*
static gint
lsq_archive_support_compr_add(LSQArchive *archive, GSList *filenames)
{
	gchar *command = NULL;

	if(!LSQ_IS_ARCHIVE_SUPPORT_COMPR(archive->support))
	{
		g_critical("Support is not compr");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *file_path = g_shell_quote(filenames->data);

		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-gzip"))
			command = g_strconcat("gzip -c ", file_path, NULL);
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-bzip"))
			command = g_strconcat("bzip2 -c ", file_path, NULL);

		lsq_execute(command, archive, NULL, NULL, lsq_archive_support_compr_compress_parse_output, NULL);

		g_free(file_path);
	}
	return 0;
}
*/

static gint
lsq_archive_support_compr_extract(LSQArchive *archive, const gchar *extract_path, GSList *filenames)
{
	gchar *command = NULL;

	if(!LSQ_IS_ARCHIVE_SUPPORT_COMPR(archive->support))
	{
		g_critical("Support is not Compr");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *archive_path = g_shell_quote(archive->path);
		gchar *file_path;
		if(filenames)
			file_path = g_strconcat(extract_path, "/", filenames->data, NULL);
		else
		{
			gchar *filename = g_strdup(lsq_archive_get_filename(archive));
			gint len = strlen(filename);
			if(g_str_has_suffix(lsq_archive_get_filename(archive), ".gz"))
			{
				filename[len-3] = '\0';
			}
			if(g_str_has_suffix(lsq_archive_get_filename(archive), ".bz"))
			{
				filename[len-3] = '\0';
			}
			if(g_str_has_suffix(lsq_archive_get_filename(archive), ".bz2"))
			{
				filename[len-4] = '\0';
			}
			file_path = g_strconcat(extract_path, "/", filename, NULL);
			g_free(filename);
		}

		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-gzip"))
			command = g_strconcat("gunzip -c ", archive_path, NULL);
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-bzip"))
			command = g_strconcat("bunzip2 -c ", archive_path, NULL);

		g_unlink(file_path);
		g_object_set_data(G_OBJECT(archive), LSQ_ARCHIVE_DEST_FILE, file_path);
		lsq_execute(command, archive, lsq_archive_support_compr_passive_watch, NULL, lsq_archive_support_compr_decompress_parse_output, NULL);

		g_free(archive_path);
	}
	return 0;
}

void
lsq_archive_support_compr_passive_watch(GPid pid, gint status, gpointer data)
{
	LSQArchive *archive = data;
	archive->child_pid = 0;
}

static gint
lsq_archive_support_compr_refresh(LSQArchive *archive)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_COMPR(archive->support))
	{
		g_critical("Support is not Compr");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *filename = g_strdup(lsq_archive_get_filename(archive));
		gint len = strlen(filename);
		if(g_str_has_suffix(lsq_archive_get_filename(archive), ".gz"))
		{
			filename[len-3] = '\0';
		}
		if(g_str_has_suffix(lsq_archive_get_filename(archive), ".bz"))
		{
			filename[len-3] = '\0';
		}
		if(g_str_has_suffix(lsq_archive_get_filename(archive), ".bz2"))
		{
			filename[len-4] = '\0';
		}
		lsq_archive_add_file(archive, filename);
		g_free(filename);
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_IDLE);
	}
	return 0;
}

static gboolean
lsq_archive_support_compr_decompress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	FILE *out_file = NULL;
	LSQArchive *archive = data;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 0;
	GError *error = NULL;

	if(cond & (G_IO_PRI | G_IO_IN))
	{
		out_file = fopen(g_object_get_data(G_OBJECT(archive), LSQ_ARCHIVE_DEST_FILE), "ab");
		if(!out_file)
			g_critical("Could not open file");

		while(g_io_channel_read_chars(ioc, buf, 1024, (gsize *)&read, &error) == G_IO_STATUS_NORMAL)
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
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);

		if(!(cond & G_IO_ERR))
			lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_IDLE);
		else
			lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_ERROR);
		return FALSE; 
	}
	return TRUE;
}

static void
lsq_archive_support_compr_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}

static void
lsq_archive_support_compr_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}
