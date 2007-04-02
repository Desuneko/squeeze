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
#include "command-builder-compr.h"

#define LSQ_ARCHIVE_DEST_FILE "compr_dest_file"

static void
lsq_command_builder_compr_class_init(LSQCommandBuilderComprClass *);
static void
lsq_command_builder_compr_init(LSQCommandBuilderCompr *archive);
static void
lsq_command_builder_compr_dispose(GObject *object);
static void
lsq_command_builder_compr_finalize(GObject *object);

static LSQArchiveCommand *
lsq_command_builder_compr_build_add(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);
static LSQArchiveCommand *
lsq_command_builder_compr_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *files);
static LSQArchiveCommand *
lsq_command_builder_compr_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive);
static LSQArchiveCommand *
lsq_command_builder_compr_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);

static gboolean
lsq_command_builder_compr_refresh(LSQArchiveCommand *command);
static gboolean
lsq_command_builder_compr_compress_parse_output(LSQSpawnCommand *, gpointer);
static gboolean
lsq_command_builder_compr_decompress_parse_output(LSQSpawnCommand *, gpointer);

static GObjectClass *parent_class;

GType
lsq_command_builder_compr_get_type ()
{
	static GType lsq_command_builder_compr_type = 0;

	if (!lsq_command_builder_compr_type)
	{
		static const GTypeInfo lsq_command_builder_compr_info = 
		{
			sizeof (LSQCommandBuilderComprClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_command_builder_compr_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQCommandBuilderCompr),
			0,
			(GInstanceInitFunc) lsq_command_builder_compr_init,
			NULL
		};

		lsq_command_builder_compr_type = g_type_register_static (LSQ_TYPE_COMMAND_BUILDER, "LSQCommandBuilderCompr", &lsq_command_builder_compr_info, 0);
	}
	return lsq_command_builder_compr_type;
}

static void
lsq_command_builder_compr_class_init(LSQCommandBuilderComprClass *command_builder_compr_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(command_builder_compr_class);

	object_class->dispose = lsq_command_builder_compr_dispose;
	object_class->finalize = lsq_command_builder_compr_finalize;

	parent_class = g_type_class_peek(G_TYPE_OBJECT); 

}

static void
lsq_command_builder_compr_init(LSQCommandBuilderCompr *command_builder_compr)
{
	LSQCommandBuilder *command_builder = LSQ_COMMAND_BUILDER(command_builder_compr);

	command_builder->build_add = lsq_command_builder_compr_build_add;
	command_builder->build_extract = lsq_command_builder_compr_build_extract;
	command_builder->build_remove = lsq_command_builder_compr_build_remove;
	command_builder->build_refresh = lsq_command_builder_compr_build_refresh;

	command_builder->mime_types = g_new0(gchar *, 5);
	gint i = 0;
	if(g_find_program_in_path("compress"))
	{
		command_builder->mime_types[i] = "application/x-compress";
		i++;
	}
	if(g_find_program_in_path("gzip"))
	{
		command_builder->mime_types[i] = "application/x-gzip";
		i++;
	}
	if(g_find_program_in_path("bzip2"))
	{
		command_builder->mime_types[i] = "application/x-bzip";
		i++;
	}
	if(g_find_program_in_path("lzop"))
	{
		command_builder->mime_types[i] = "application/x-lzop";
	}
}

/**
 * lsq_command_builder_compr_dispose:
 *
 * @object: LSQCommandBuilderCompr object
 *
 */
static void
lsq_command_builder_compr_dispose(GObject *object)
{

	parent_class->dispose(object);
}

/**
 * lsq_command_builder_compr_finalize:
 *
 * @object: LSQCommandBuilderCompr object
 *
 */
static void
lsq_command_builder_compr_finalize(GObject *object)
{
	parent_class->finalize(object);
}

const gchar *
lsq_command_builder_compr_get_decompress_skeleton(LSQCommandBuilder *builder, LSQArchive *archive)
{
	const gchar *decompress_skeleton = NULL;

	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-compress"))
		decompress_skeleton = "uncompress -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-gzip"))
		decompress_skeleton = "gunzip -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-bzip"))
		decompress_skeleton = "bunzip2 -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-lzop"))
		decompress_skeleton = "lzop -dc %1$s";

	return decompress_skeleton;
}

const gchar *
lsq_command_builder_compr_get_compress_skeleton(LSQCommandBuilder *builder, LSQArchive *archive)
{
	const gchar *compress_skeleton = NULL;

	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-compress"))
		compress_skeleton = "compress -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-gzip"))
		compress_skeleton = "gzip -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-bzip"))
		compress_skeleton = "bzip2 -c %1$s";
	if(!g_strcasecmp(lsq_archive_get_mimetype(archive), "application/x-lzop"))
		compress_skeleton = "lzop -c %1$s";

	return compress_skeleton;
}


static LSQArchiveCommand *
lsq_command_builder_compr_build_add(LSQCommandBuilder *builder, LSQArchive *archive, GSList *filenames)
{
#ifdef DEBUG
	g_return_val_if_fail(filenames, NULL);
#endif

	const gchar *compress_skeleton = lsq_command_builder_compr_get_compress_skeleton(builder, archive);
	LSQArchiveCommand *compress = lsq_spawn_command_new(_("Compressing"), 
	                                                      archive,
	                                                      compress_skeleton,
	                                                      NULL,
	                                                      NULL,
	                                                      filenames->data);

	if(!lsq_spawn_command_set_parse_func(LSQ_SPAWN_COMMAND(compress), 1, lsq_command_builder_compr_compress_parse_output, NULL))
	{
		g_critical("Could not set compress parse function");
	}
	return compress;
}

static LSQArchiveCommand *
lsq_command_builder_compr_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *filenames)
{
	return NULL;
}

static LSQArchiveCommand *
lsq_command_builder_compr_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *filenames)
{
	const gchar *decompress_skeleton = lsq_command_builder_compr_get_decompress_skeleton(builder, archive);
	LSQArchiveCommand *decompress = lsq_spawn_command_new(_("Decompressing"), 
	                                                      archive,
	                                                      decompress_skeleton,
	                                                      NULL,
	                                                      NULL,
	                                                      NULL);

	g_object_set_data(G_OBJECT(decompress), LSQ_ARCHIVE_DEST_FILE, g_strdup(dest_path));

	if(!lsq_spawn_command_set_parse_func(LSQ_SPAWN_COMMAND(decompress), 1, lsq_command_builder_compr_decompress_parse_output, NULL))
	{
		g_critical("Could not set decompress parse function");
	}
	return decompress;
}

static LSQArchiveCommand *
lsq_command_builder_compr_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive)
{
	LSQArchiveCommand *command = lsq_archive_command_new(_("Refresh"), archive, lsq_command_builder_compr_refresh);

	return command;
}

static gboolean
lsq_command_builder_compr_refresh(LSQArchiveCommand *command)
{
	LSQArchiveIter *entry;
	LSQArchive *archive = lsq_archive_command_get_archive(command);
	gchar *filename = lsq_archive_get_filename(archive);
	gint length = strlen(filename);
	if(g_str_has_suffix(filename, ".gz"))
		filename[length-3] = '\0';
	if(g_str_has_suffix(filename, ".bz"))
		filename[length-3] = '\0';
	if(g_str_has_suffix(filename, ".bz2"))
		filename[length-4] = '\0';
	if(g_str_has_suffix(filename, ".lzo"))
		filename[length-4] = '\0';
	if(g_str_has_suffix(filename, ".Z"))
		filename[length-2] = '\0';

	entry = lsq_archive_add_file(archive, filename);
	lsq_archive_iter_unref(entry);
	g_free(filename);

	return TRUE;
}

static gboolean
lsq_command_builder_compr_decompress_parse_output(LSQSpawnCommand *spawn_command, gpointer user_data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 0;
	GError *error = NULL;
	FILE *out_file;

	const gchar *out_filename = g_object_get_data(G_OBJECT(spawn_command), LSQ_ARCHIVE_DEST_FILE);

	out_file = fopen(out_filename, "ab");
	if(!out_file)
		return FALSE; 
	
	status = lsq_spawn_command_read_bytes(spawn_command, 1, buf, 1024, (gsize *)&read, &error);
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
lsq_command_builder_compr_compress_parse_output(LSQSpawnCommand *spawn_command, gpointer user_data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	gchar *buf = g_new0(gchar, 1024);
	LSQArchive *archive = lsq_archive_command_get_archive(LSQ_ARCHIVE_COMMAND(spawn_command));
	guint read = 0;
	GError *error = NULL;
	FILE *out_file;

	const gchar *out_filename = lsq_archive_get_path(archive);
	gboolean remove = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(spawn_command), "compressing"));
	if(remove == FALSE)
	{
		g_object_set_data(G_OBJECT(spawn_command), "compressing", GUINT_TO_POINTER(TRUE));
		g_unlink(out_filename);
	}

	out_file = fopen(out_filename, "ab");
	if(!out_file)
		return FALSE; 
	
	status = lsq_spawn_command_read_bytes(spawn_command, 1, buf, 1024, (gsize *)&read, &error);
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

LSQCommandBuilder *
lsq_command_builder_compr_new()
{
	LSQCommandBuilder *builder;

	builder = g_object_new(LSQ_TYPE_COMMAND_BUILDER_COMPR, NULL);

	return builder;
}
