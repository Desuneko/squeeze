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
#include <thunar-vfs/thunar-vfs.h>

#include <libsqueeze/libsqueeze-module.h>

#include "archive-support-compr.h"

#define LSQ_ARCHIVE_DEST_FILE "compr_dest_file"

static void
lsq_archive_support_compr_init(LSQArchiveSupportCompr *support);
static void
lsq_archive_support_compr_class_init(LSQArchiveSupportComprClass *supportclass);

void
lsq_archive_support_compr_passive_watch(GPid pid, gint status, gpointer data);

static void
lsq_archive_support_compr_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
lsq_archive_support_compr_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gint lsq_archive_support_compr_extract(LSQArchiveSupport *, LSQArchive *, const gchar *, GSList *);
static gint lsq_archive_support_compr_refresh(LSQArchiveSupport *, LSQArchive *);

static gboolean
lsq_archive_support_compr_decompress_parse_output(LSQArchiveCommand *archive_command, gpointer user_data);

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
	lsq_archive_support_add_mime(archive_support, "application/x-lzop");
	lsq_archive_support_add_mime(archive_support, "application/x-compress");

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
lsq_archive_support_compr_extract(LSQArchiveSupport *support, LSQArchive *archive, const gchar *extract_path, GSList *filenames)
{
	gchar *command_skeleton = NULL;

	if(!LSQ_IS_ARCHIVE_SUPPORT_COMPR(support))
	{
		g_critical("Support is not Compr");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(support, lsq_archive_get_mimetype(archive)))
	{
		return 1;
	}
	else
	{
		gchar *archive_path = g_shell_quote(lsq_archive_get_filename(archive));
		gchar *file_path;
		if(filenames)
			file_path = g_strconcat(extract_path, "/", filenames->data, NULL);
		else
		{
			gchar *filename = g_path_get_basename(lsq_archive_get_filename(archive));
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
			if(g_str_has_suffix(lsq_archive_get_filename(archive), ".lzo"))
			{
				filename[len-4] = '\0';
			}
			if(g_str_has_suffix(lsq_archive_get_filename(archive), ".Z"))
			{
				filename[len-2] = '\0';
			}
			file_path = g_strconcat(extract_path, "/", filename, NULL);
			g_free(filename);
		}

		if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-gzip"))
			command_skeleton = g_strdup("gunzip -c %1$s");
		if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-bzip"))
			command_skeleton = g_strdup("bunzip2 -c %1$s");
		if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-compress"))
			command_skeleton = g_strdup("uncompress -c %1$s");
		if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-lzop"))
			command_skeleton = g_strdup("lzop -dc  %1$s");

		g_unlink(file_path);
		LSQArchiveCommand *archive_command = lsq_archive_command_new("", command_skeleton, FALSE, TRUE);
		lsq_archive_command_set_parse_func(archive_command, 1, lsq_archive_support_compr_decompress_parse_output, support);
		g_object_set_data(G_OBJECT(archive_command), LSQ_ARCHIVE_DEST_FILE, file_path);
		lsq_archive_enqueue_command(archive, archive_command);
		
		g_object_unref(archive_command);

		g_free(command_skeleton);
		g_free(archive_path);
	}
	return 0;
}

static gint
lsq_archive_support_compr_refresh(LSQArchiveSupport *support, LSQArchive *archive)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_COMPR(support))
	{
		g_critical("Support is not Compr");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(support, lsq_archive_get_mimetype(archive)))
	{
		return 1;
	}
	else
	{
		gchar *filename = g_path_get_basename(lsq_archive_get_filename(archive));
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
		lsq_archive_refreshed(archive);
		g_free(filename);
	}
	return 0;
}

static gboolean
lsq_archive_support_compr_decompress_parse_output(LSQArchiveCommand *archive_command, gpointer user_data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 0;
	GError *error = NULL;
	FILE *out_file;

	const gchar *out_filename = g_object_get_data(G_OBJECT(archive_command), LSQ_ARCHIVE_DEST_FILE);

	out_file = fopen(out_filename, "ab");
	if(!out_file)
		return FALSE; 
	
	status = lsq_archive_command_read_bytes(archive_command, 1, buf, 1024, (gsize *)&read, &error);
	if(status == G_IO_STATUS_EOF)
	{
		fclose(out_file);
		return TRUE;
	}

	if(read)
	{
		fwrite(buf, 1, read, out_file);
	}
	fclose(out_file);
	g_free(buf);

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
