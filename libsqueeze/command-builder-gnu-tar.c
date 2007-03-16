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
#include "archive-tempfs.h"
#include "archive.h"
#include "archive-command.h"
#include "macro-command.h"
#include "spawn-command.h"
#include "dbus-command.h"
#include "command-builder.h"
#include "command-builder-gnu-tar.h"

#define LSQ_ARCHIVE_TEMP_FILE "gnu_tar_temp_file"

static void
lsq_command_builder_gnu_tar_class_init(LSQCommandBuilderGnuTarClass *);
static void
lsq_command_builder_gnu_tar_init(LSQCommandBuilderGnuTar *archive);
static void
lsq_command_builder_gnu_tar_dispose(GObject *object);
static void
lsq_command_builder_gnu_tar_finalize(GObject *object);

static LSQArchiveCommand *
lsq_command_builder_gnu_tar_build_add(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);
static LSQArchiveCommand *
lsq_command_builder_gnu_tar_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *files);
static LSQArchiveCommand *
lsq_command_builder_gnu_tar_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive);
static LSQArchiveCommand *
lsq_command_builder_gnu_tar_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);

static gboolean
lsq_command_builder_gnu_tar_refresh_parse_output(LSQArchiveCommand *archive_command, gpointer user_data);
static gboolean
lsq_command_builder_gnu_tar_compress_parse_output(LSQArchiveCommand *archive_command, gpointer user_data);
static gboolean
lsq_command_builder_gnu_tar_decompress_parse_output(LSQArchiveCommand *archive_command, gpointer user_data);

static GObjectClass *parent_class;

GType
lsq_command_builder_gnu_tar_get_type ()
{
	static GType lsq_command_builder_gnu_tar_type = 0;

	if (!lsq_command_builder_gnu_tar_type)
	{
		static const GTypeInfo lsq_command_builder_gnu_tar_info = 
		{
			sizeof (LSQCommandBuilderGnuTarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_command_builder_gnu_tar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQCommandBuilderGnuTar),
			0,
			(GInstanceInitFunc) lsq_command_builder_gnu_tar_init,
			NULL
		};

		lsq_command_builder_gnu_tar_type = g_type_register_static (G_TYPE_OBJECT, "LSQCommandBuilderGnuTar", &lsq_command_builder_gnu_tar_info, 0);
	}
	return lsq_command_builder_gnu_tar_type;
}

static void
lsq_command_builder_gnu_tar_class_init(LSQCommandBuilderGnuTarClass *command_builder_gnu_tar_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(command_builder_gnu_tar_class);

	object_class->dispose = lsq_command_builder_gnu_tar_dispose;
	object_class->finalize = lsq_command_builder_gnu_tar_finalize;

	parent_class = g_type_class_peek(G_TYPE_OBJECT); 

}

static void
lsq_command_builder_gnu_tar_init(LSQCommandBuilderGnuTar *command_builder_gnu_tar)
{
	LSQCommandBuilder *command_builder = LSQ_COMMAND_BUILDER(command_builder_gnu_tar);

	command_builder->build_add = lsq_command_builder_gnu_tar_build_add;
	command_builder->build_extract = lsq_command_builder_gnu_tar_build_extract;
	command_builder->build_remove = lsq_command_builder_gnu_tar_build_remove;
	command_builder->build_refresh = lsq_command_builder_gnu_tar_build_refresh;
}

/**
 * lsq_command_builder_gnu_tar_dispose:
 *
 * @object: LSQCommandBuilderGnuTar object
 *
 */
static void
lsq_command_builder_gnu_tar_dispose(GObject *object)
{

	parent_class->dispose(object);
}

/**
 * lsq_command_builder_gnu_tar_finalize:
 *
 * @object: LSQCommandBuilderGnuTar object
 *
 */
static void
lsq_command_builder_gnu_tar_finalize(GObject *object)
{
	parent_class->finalize(object);
}

static const gchar *
lsq_command_builder_gnu_tar_get_compress_skeleton(LSQCommandBuilder *builder, LSQArchive *archive)
{
	const gchar *decompress_skeleton = NULL;

	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-tarz"))
		decompress_skeleton = "uncompress -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-compressed-tar"))
		decompress_skeleton = "gunzip -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-bzip-compressed-tar"))
		decompress_skeleton = "bunzip2 -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-tzo"))
		decompress_skeleton = "lzop -dc %1$s";

	return decompress_skeleton;
}

static const gchar *
lsq_command_builder_gnu_tar_get_decompress_skeleton(LSQCommandBuilder *builder, LSQArchive *archive)
{
	const gchar *compress_skeleton = NULL;

	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-tarz"))
		compress_skeleton = "compress -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-compressed-tar"))
		compress_skeleton = "gzip -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-bzip-compressed-tar"))
		compress_skeleton = "bzip2 -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-tzo"))
		compress_skeleton = "lzop -c %1$s";

	return compress_skeleton;
}

static LSQArchiveCommand *
lsq_command_builder_gnu_tar_build_add(LSQCommandBuilder *builder, LSQArchive *archive, GSList *filenames)
{
	gchar *options = NULL;
	gchar *tmp_file = NULL;
	gchar *files = lsq_concat_filenames(filenames);
	const gchar *add_skeleton = NULL;
	const gchar *compress_skeleton = NULL;
	const gchar *decompress_skeleton = NULL;
	LSQArchiveCommand *add_macro = NULL;
	
	compress_skeleton = lsq_command_builder_gnu_tar_get_compress_skeleton(builder, archive);
	decompress_skeleton = lsq_command_builder_gnu_tar_get_decompress_skeleton(builder, archive);

	if(decompress_skeleton)
		tmp_file = lsq_archive_request_temp_file(archive, ".tar");

	if(!lsq_archive_exists(archive))
	{
		add_skeleton = "tar %3$s -c -f %1$s %2$s";
	}
	else
	{
		add_skeleton = "tar %3$s -r -f %1$s %2$s";
	}
	LSQArchiveCommand *spawn = lsq_spawn_command_new(_("Adding files"),
	                                                 archive,
								                     add_skeleton,
	                                                 files,
	                                                 options,
                                                     tmp_file);

	if(decompress_skeleton)
	{
		LSQArchiveCommand *decompress = lsq_spawn_command_new(_("Decompressing"), 
		                                                      archive,
		                                                      decompress_skeleton,
		                                                      NULL,
		                                                      NULL,
		                                                      tmp_file);
		LSQArchiveCommand *compress = lsq_spawn_command_new(_("Compressing"),
		                                                    archive,
		                                                    compress_skeleton,
		                                                    NULL,
		                                                    NULL,
		                                                    tmp_file);
		add_macro = lsq_macro_command_new(NULL, archive);
		lsq_macro_command_append(LSQ_MACRO_COMMAND(add_macro), decompress);
		lsq_macro_command_append(LSQ_MACRO_COMMAND(add_macro), spawn);
		lsq_macro_command_append(LSQ_MACRO_COMMAND(add_macro), compress);
	}

	if(!add_macro)
		add_macro = spawn;

	g_free(files);
	return NULL;
}


static LSQArchiveCommand *
lsq_command_builder_gnu_tar_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *filenames)
{
	gchar *options = NULL;
	gchar *tmp_file = NULL;
	gchar *files = lsq_concat_filenames(filenames);
	const gchar *compress_skeleton = NULL;
	const gchar *decompress_skeleton = NULL;
	LSQArchiveCommand *remove_macro = NULL;
	
	compress_skeleton = lsq_command_builder_gnu_tar_get_compress_skeleton(builder, archive);
	decompress_skeleton = lsq_command_builder_gnu_tar_get_decompress_skeleton(builder, archive);

	if(decompress_skeleton)
		tmp_file = lsq_archive_request_temp_file(archive, ".tar");

	LSQArchiveCommand *spawn = lsq_spawn_command_new(_("Removing files"),
	                                                 archive,
	                                                 "tar %3$s -f %1$s --delete %2$s",
	                                                 files,
	                                                 options,
                                                     tmp_file);

	if(decompress_skeleton)
	{
		LSQArchiveCommand *decompress = lsq_spawn_command_new(_("Decompressing"), 
		                                                      archive,
		                                                      decompress_skeleton,
		                                                      NULL,
		                                                      NULL,
		                                                      NULL);

		g_object_set_data(G_OBJECT(decompress), LSQ_ARCHIVE_TEMP_FILE, tmp_file);

		LSQArchiveCommand *compress = lsq_spawn_command_new(_("Compressing"),
		                                                    archive,
		                                                    compress_skeleton,
		                                                    NULL,
		                                                    NULL,
		                                                    tmp_file);
		remove_macro = lsq_macro_command_new(NULL, archive);
		lsq_macro_command_append(LSQ_MACRO_COMMAND(remove_macro), decompress);
		lsq_macro_command_append(LSQ_MACRO_COMMAND(remove_macro), spawn);
		lsq_macro_command_append(LSQ_MACRO_COMMAND(remove_macro), compress);

		if(!lsq_spawn_command_set_parse_func(LSQ_SPAWN_COMMAND(compress), 1, lsq_command_builder_gnu_tar_compress_parse_output, NULL))
		{
			g_critical("Could not set compress parse function");
		}

		if(!lsq_spawn_command_set_parse_func(LSQ_SPAWN_COMMAND(decompress), 1, lsq_command_builder_gnu_tar_decompress_parse_output, NULL))
		{
			g_critical("Could not set decompress parse function");
		}
	}

	if(!remove_macro)
		remove_macro = spawn;

	g_free(files);
	return remove_macro;
}

static LSQArchiveCommand *
lsq_command_builder_gnu_tar_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *filenames)
{
	gchar *files = lsq_concat_filenames(filenames);
	gchar *options = g_strconcat(" -C ", dest_path, NULL);

	LSQArchiveCommand *spawn = lsq_spawn_command_new("Extract", archive, "tar %3$s -x -f %1$s %2$s", files, options, NULL);

	g_free(options);
	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_gnu_tar_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive)
{
	LSQArchiveCommand *spawn = lsq_spawn_command_new("Refresh", archive, "tar -tvvf %1$s", NULL, NULL, NULL);

	if(!lsq_spawn_command_set_parse_func(LSQ_SPAWN_COMMAND(spawn), 1, lsq_command_builder_gnu_tar_refresh_parse_output, NULL))
	{
		g_critical("Could not set refresh parse function");
	}

	return spawn;
}


static gboolean
lsq_command_builder_gnu_tar_refresh_parse_output(LSQArchiveCommand *archive_command, gpointer user_data)
{
	gchar *line = NULL;
	gsize linesize = 0;
	GIOStatus status = G_IO_STATUS_NORMAL;
	LSQArchive *archive = lsq_archive_command_get_archive(archive_command);
	guint64 size;
	gpointer props[6];
	gint n = 0, a = 0, i = 0;
	gchar *temp_filename = NULL;

	LSQArchiveIter *entry;

	status = lsq_archive_command_read_line(archive_command, 1, &line, &linesize, NULL);
	if (line == NULL)
	{
		if(status == G_IO_STATUS_AGAIN)
			return TRUE;
		else
			return FALSE;
	}

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_GNU_TAR(support)->_view_rights*/
	{
		line[10] = '\0';
		props[i] = line;
		i++;
	}
	for(n=13; n < linesize; ++n)
		if(line[n] == ' ') break;

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_GNU_TAR(support)->_view_owner*/
	{
		line[n] = '\0';
		props[i] = line+11;
		i++;
	}

	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9') break;

	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ') break;

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_GNU_TAR(support)->_view_size*/
	{
		line[n] = '\0';
		size = g_ascii_strtoull(line + a, NULL, 0);
		props[i] = &size;
		i++;
	}

	a = ++n;

	for(; n < linesize; n++) // DATE
		if(line[n] == ' ') break;

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_GNU_TAR(support)->_view_date*/
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}

	a = ++n;
	for (; n < linesize; n++) // TIME
		if (line[n] == ' ') break;

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_GNU_TAR(support)->_view_time*/
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	props[i] = NULL;
	gchar *temp = g_strrstr (&line[n],"->"); 
	if (temp ) 
	{ 
		temp[0] = '\0';
	}
	else
	{
		line[linesize-1] = '\0';
	}
	if(line[0] == 'd')
	{
		/* 1: Work around for gtar, which does not output 
		 * trailing slashes with directories. */
		/* 2: The line includes the newline character, this 
		 * would probably break on platforms that use 
		 * more then one character to indicate a line-end (\r\n) */
		if(line[linesize-2] != '/')
			temp_filename = g_strconcat(line + n, "/", NULL); 
		else
			temp_filename = g_strdup(line + n); 

		entry = lsq_archive_add_file(archive, temp_filename);
		g_free(temp_filename);
	}
	else
	{
		temp_filename = line + n; 

		entry = lsq_archive_add_file(archive, temp_filename);
	}

	lsq_archive_iter_set_propsv(entry, (gconstpointer*)props);
	lsq_archive_iter_unref(entry);
	g_free(line);
	return TRUE;
}

static gboolean
lsq_command_builder_gnu_tar_decompress_parse_output(LSQArchiveCommand *archive_command, gpointer user_data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 0;
	GError *error = NULL;
	FILE *out_file;

	const gchar *out_filename = g_object_get_data(G_OBJECT(archive_command), LSQ_ARCHIVE_TEMP_FILE);

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

static gboolean
lsq_command_builder_gnu_tar_compress_parse_output(LSQArchiveCommand *archive_command, gpointer user_data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	gchar *buf = g_new0(gchar, 1024);
	LSQArchive *archive = lsq_archive_command_get_archive(archive_command);
	guint read = 0;
	GError *error = NULL;
	FILE *out_file;

	const gchar *out_filename = lsq_archive_get_path(archive);
	gboolean remove = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(archive_command), "compressing"));
	if(remove == FALSE)
	{
		g_object_set_data(G_OBJECT(archive_command), "compressing", GUINT_TO_POINTER(TRUE));
		g_unlink(out_filename);
	}

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
