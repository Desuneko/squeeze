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
#include "command-builder-zip.h"


static void
lsq_command_builder_zip_class_init(LSQCommandBuilderZipClass *);
static void
lsq_command_builder_zip_init(LSQCommandBuilderZip *archive);
static void
lsq_command_builder_zip_dispose(GObject *object);
static void
lsq_command_builder_zip_finalize(GObject *object);

static LSQArchiveCommand *
lsq_command_builder_zip_build_add(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);
static LSQArchiveCommand *
lsq_command_builder_zip_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *files);
static LSQArchiveCommand *
lsq_command_builder_zip_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive);
static LSQArchiveCommand *
lsq_command_builder_zip_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);

static gboolean
lsq_command_builder_zip_refresh_parse_output(LSQSpawnCommand *, gpointer);

static GObjectClass *parent_class;

GType
lsq_command_builder_zip_get_type ()
{
	static GType lsq_command_builder_zip_type = 0;

	if (!lsq_command_builder_zip_type)
	{
		static const GTypeInfo lsq_command_builder_zip_info = 
		{
			sizeof (LSQCommandBuilderZipClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_command_builder_zip_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQCommandBuilderZip),
			0,
			(GInstanceInitFunc) lsq_command_builder_zip_init,
			NULL
		};

		lsq_command_builder_zip_type = g_type_register_static (LSQ_TYPE_COMMAND_BUILDER, "LSQCommandBuilderZip", &lsq_command_builder_zip_info, 0);
	}
	return lsq_command_builder_zip_type;
}

static void
lsq_command_builder_zip_class_init(LSQCommandBuilderZipClass *command_builder_zip_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(command_builder_zip_class);

	object_class->dispose = lsq_command_builder_zip_dispose;
	object_class->finalize = lsq_command_builder_zip_finalize;

	parent_class = g_type_class_peek(LSQ_TYPE_COMMAND_BUILDER); 

}

static void
lsq_command_builder_zip_init(LSQCommandBuilderZip *command_builder_zip)
{
	LSQCommandBuilder *command_builder = LSQ_COMMAND_BUILDER(command_builder_zip);

	command_builder->build_add = lsq_command_builder_zip_build_add;
	command_builder->build_extract = lsq_command_builder_zip_build_extract;
	command_builder->build_remove = lsq_command_builder_zip_build_remove;
	command_builder->build_refresh = lsq_command_builder_zip_build_refresh;

	command_builder->mime_types = g_new0(gchar *, 3);
	command_builder->mime_types[0] = "application/zip";
	command_builder->mime_types[1] = "application/x-zip";
}

/**
 * lsq_command_builder_zip_dispose:
 *
 * @object: LSQCommandBuilderZip object
 *
 */
static void
lsq_command_builder_zip_dispose(GObject *object)
{

	parent_class->dispose(object);
}

/**
 * lsq_command_builder_zip_finalize:
 *
 * @object: LSQCommandBuilderZip object
 *
 */
static void
lsq_command_builder_zip_finalize(GObject *object)
{
	parent_class->finalize(object);
}


static LSQArchiveCommand *
lsq_command_builder_zip_build_add(LSQCommandBuilder *builder, LSQArchive *archive, GSList *filenames)
{
	gchar *files = lsq_concat_filenames(filenames);
	LSQArchiveCommand *spawn = lsq_spawn_command_new("Add", archive, "zip %3$s -r %1$s %2$s", files, NULL, NULL);
	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_zip_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *filenames)
{
	gchar *files = lsq_concat_filenames(filenames);

	LSQArchiveCommand *spawn = lsq_spawn_command_new("Remove", archive, "zip -d %1$s %2$s", files, NULL, NULL);

	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_zip_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *filenames)
{
	gchar *files = lsq_concat_filenames(filenames);
	gchar *options = g_strconcat(" -d ", dest_path, NULL);

	LSQArchiveCommand *spawn = lsq_spawn_command_new("Extract", archive, "unzip -o %1$s %2$s %3$s", files, options, NULL);

	g_free(options);
	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_zip_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive)
{
	LSQArchiveCommand *spawn = lsq_spawn_command_new("Refresh", archive, "unzip -lv -qq %1$s", NULL, NULL, NULL);

	if(!lsq_spawn_command_set_parse_func(LSQ_SPAWN_COMMAND(spawn), 1, lsq_command_builder_zip_refresh_parse_output, NULL))
	{
		g_critical("Could not set refresh parse function");
	}

	return spawn;
}

static gboolean
lsq_command_builder_zip_refresh_parse_output(LSQSpawnCommand *spawn_command, gpointer user_data)
{
	gchar *line = NULL;
	gsize linesize = 0;
	GIOStatus status = G_IO_STATUS_NORMAL;
	LSQArchive *archive = lsq_archive_command_get_archive(LSQ_ARCHIVE_COMMAND(spawn_command));
	guint64 size;
	guint64 length;
	gpointer props[8]; 
	gint n = 0, a = 0, i = 0;
	gchar *temp_filename = NULL;

	LSQArchiveIter *entry;

	status = lsq_spawn_command_read_line(spawn_command, 1, &line, &linesize, NULL);
	if (line == NULL)
	{
		if(status == G_IO_STATUS_AGAIN)
			return TRUE;
		else
			return FALSE;
	}
	/* length, method , size, ratio, date, time, crc-32, filename*/
	for(n=0; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(TRUE)/*LSQ_ARCHIVE_SUPPORT_ZIP(support)->_view_length*/
	{
		line[n]='\0';
		length = g_ascii_strtoull(line + a, NULL, 0);
		props[i] = &length;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_ZIP(support)->_view_method*/
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_ZIP(support)->_view_size*/
	{
		line[n]='\0';
		size = g_ascii_strtoull(line + a, NULL, 0);
		props[i] = &size;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_ZIP(support)->_view_ratio*/
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_ZIP(support)->_view_date*/
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(TRUE) /* LSQ_ARCHIVE_SUPPORT_ZIP(support)->_view_time */
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(TRUE) /*LSQ_ARCHIVE_SUPPORT_ZIP(support)->_view_crc_32 */
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n+=2;

	line[linesize-1] = '\0';
	temp_filename = line+n; 
	props[i] = NULL;

	entry = lsq_archive_add_file(archive, temp_filename);
	lsq_archive_iter_set_propsv(entry, (gconstpointer*)props);
	lsq_archive_iter_unref(entry);
	g_free(line);

	return TRUE;
}

LSQCommandBuilder *
lsq_command_builder_zip_new()
{
	LSQCommandBuilder *builder;

	builder = g_object_new(lsq_command_builder_zip_get_type(), NULL);

	return builder;
}
