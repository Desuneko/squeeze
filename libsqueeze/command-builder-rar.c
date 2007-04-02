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
#include "archive-command.h"
#include "spawn-command.h"
#include "command-builder-rar.h"

enum
{
	REFRESH_STATUS_INIT,
	REFRESH_STATUS_FILES,
	REFRESH_STATUS_FINISH
};

#define LSQ_ARCHIVE_RAR_STATUS "lsq_archive_rar_status"
#define LSQ_ARCHIVE_RAR_LAST_ENTRY "lsq_archive_rar_last_entry"

static void
lsq_command_builder_rar_class_init(LSQCommandBuilderRarClass *);
static void
lsq_command_builder_rar_init(LSQCommandBuilderRar *archive);
static void
lsq_command_builder_rar_dispose(GObject *object);
static void
lsq_command_builder_rar_finalize(GObject *object);

static LSQArchiveCommand *
lsq_command_builder_rar_build_add(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);
static LSQArchiveCommand *
lsq_command_builder_rar_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *files);
static LSQArchiveCommand *
lsq_command_builder_rar_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive);
static LSQArchiveCommand *
lsq_command_builder_rar_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);

static gboolean
lsq_command_builder_rar_refresh_parse_output(LSQSpawnCommand *spawn_command, gpointer user_data);

static GObjectClass *parent_class;

GType
lsq_command_builder_rar_get_type ()
{
	static GType lsq_command_builder_rar_type = 0;

	if (!lsq_command_builder_rar_type)
	{
		static const GTypeInfo lsq_command_builder_rar_info = 
		{
			sizeof (LSQCommandBuilderRarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_command_builder_rar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQCommandBuilderRar),
			0,
			(GInstanceInitFunc) lsq_command_builder_rar_init,
			NULL
		};

		lsq_command_builder_rar_type = g_type_register_static (LSQ_TYPE_COMMAND_BUILDER, "LSQCommandBuilderRar", &lsq_command_builder_rar_info, 0);
	}
	return lsq_command_builder_rar_type;
}

static void
lsq_command_builder_rar_class_init(LSQCommandBuilderRarClass *command_builder_rar_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(command_builder_rar_class);

	object_class->dispose = lsq_command_builder_rar_dispose;
	object_class->finalize = lsq_command_builder_rar_finalize;

	parent_class = g_type_class_peek(G_TYPE_OBJECT); 

}

static void
lsq_command_builder_rar_init(LSQCommandBuilderRar *command_builder_rar)
{
	LSQCommandBuilder *command_builder = LSQ_COMMAND_BUILDER(command_builder_rar);

	gchar *rar_path = g_find_program_in_path("rar");
	gchar *unrar_path = g_find_program_in_path("unrar");

	if(rar_path)
	{
		command_builder->build_add = lsq_command_builder_rar_build_add;
		command_builder->build_remove = lsq_command_builder_rar_build_remove;
	}
	if(unrar_path)
	{
		command_builder->build_extract = lsq_command_builder_rar_build_extract;
		command_builder->build_refresh = lsq_command_builder_rar_build_refresh;
	}

	command_builder->mime_types = g_new0(gchar *, 2);
	if(rar_path || unrar_path)
		command_builder->mime_types[0] = "application/x-rar";

	lsq_builder_settings_set_property_types(command_builder->settings, 
	                                        _("Compressed"), /* length */
	                                        G_TYPE_UINT64,
	                                        _("Size"), /* size */
	                                        G_TYPE_UINT64,
	                                        _("Ratio"), /* ratio */
	                                        G_TYPE_STRING,
	                                        _("Date"), /* date */
	                                        G_TYPE_STRING,
	                                        _("Time"), /* time */
	                                        G_TYPE_STRING,
	                                        _("Rights"), /* rights */
	                                        G_TYPE_STRING,
	                                        "CRC-32", /* crc-32 */
	                                        G_TYPE_STRING,
	                                        _("Method"), /* method */
	                                        G_TYPE_STRING,
	                                        _("Version"), /* version*/
	                                        G_TYPE_STRING,
                                          NULL);
	g_free(rar_path);
	g_free(unrar_path);
}

/**
 * lsq_command_builder_rar_dispose:
 *
 * @object: LSQCommandBuilderRar object
 *
 */
static void
lsq_command_builder_rar_dispose(GObject *object)
{

	parent_class->dispose(object);
}

/**
 * lsq_command_builder_rar_finalize:
 *
 * @object: LSQCommandBuilderRar object
 *
 */
static void
lsq_command_builder_rar_finalize(GObject *object)
{
	parent_class->finalize(object);
}


static LSQArchiveCommand *
lsq_command_builder_rar_build_add(LSQCommandBuilder *builder, LSQArchive *archive, GSList *filenames)
{
	gchar *files = lsq_concat_filenames(filenames);
	LSQArchiveCommand *spawn = lsq_spawn_command_new(_("Adding"), archive, "rar a %1$s %2$s", files, NULL, NULL);
	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_rar_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *iter_files)
{
	gchar *files = lsq_concat_iter_filenames(iter_files);

	LSQArchiveCommand *spawn = lsq_spawn_command_new(_("Removing"), archive, "rar d %1$s %2$s", files, NULL, NULL);

	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_rar_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *file_iters)
{
	gchar *files = lsq_concat_iter_filenames(file_iters);
	gchar *options = g_strconcat(dest_path, NULL);

	LSQArchiveCommand *spawn = lsq_spawn_command_new(_("Extracting"), archive, "unrar x -y %1$s %2$s %3$s", files, options, NULL);

	g_free(options);
	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_rar_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive)
{
	LSQArchiveCommand *spawn = lsq_spawn_command_new(_("Refresh"), archive, "unrar v %1$s", NULL, NULL, NULL);

	if(!lsq_spawn_command_set_parse_func(LSQ_SPAWN_COMMAND(spawn), 1, lsq_command_builder_rar_refresh_parse_output, NULL))
	{
		g_critical("Could not set refresh parse function");
	}

	return spawn;
}

LSQCommandBuilder *
lsq_command_builder_rar_new()
{
	LSQCommandBuilder *builder;

	builder = g_object_new(lsq_command_builder_rar_get_type(), NULL);

	return builder;
}

static gboolean
lsq_command_builder_rar_refresh_parse_output(LSQSpawnCommand *spawn_command, gpointer user_data)
{
	gchar *line = NULL;
	gsize linesize = 0;
	GIOStatus status = G_IO_STATUS_NORMAL;
	LSQArchive *archive = lsq_archive_command_get_archive(LSQ_ARCHIVE_COMMAND(spawn_command));
	
	guint64 size;
	guint64 length;
	gpointer props[10]; 
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

	switch(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spawn_command), LSQ_ARCHIVE_RAR_STATUS)))
	{
		case REFRESH_STATUS_INIT:
			if(line[0] == '-')
			{
				g_object_set_data(G_OBJECT(spawn_command), LSQ_ARCHIVE_RAR_STATUS, GINT_TO_POINTER(REFRESH_STATUS_FILES));
			}
		break;
		case REFRESH_STATUS_FILES:
			entry = g_object_get_data(G_OBJECT(spawn_command), LSQ_ARCHIVE_RAR_LAST_ENTRY);

			if(line[0] == '-')
			{
#ifdef DEBUG
				if(G_UNLIKELY(entry))
					g_critical("And entry misses properties");
#endif
				g_object_set_data(G_OBJECT(spawn_command), LSQ_ARCHIVE_RAR_STATUS, GINT_TO_POINTER(REFRESH_STATUS_FINISH));
			}
			else if(!entry)
			{
				/* strip first ' ' and last '\n' */
				temp_filename = line+1;
				line[linesize - 1] = '\0';
				entry = lsq_archive_add_file(archive, temp_filename);
				g_object_set_data(G_OBJECT(spawn_command), LSQ_ARCHIVE_RAR_LAST_ENTRY, entry);
			}
			else
			{
				g_object_set_data(G_OBJECT(spawn_command), LSQ_ARCHIVE_RAR_LAST_ENTRY, NULL);

				/* length, size, ratio, date, time, rights, crc-32, method , version*/
				for(n=0; n < linesize && line[n] == ' '; n++);
				a = n;
				for(; n < linesize && line[n] != ' '; n++);

				if(TRUE)/*LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_length*/
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

				if(TRUE)/*LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_size*/
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

				if(TRUE)/*LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_ratio)*/
				{
					line[n] = '\0';
					props[i] = line + a;
					i++;
				}
				n++;

				for(; n < linesize && line[n] == ' '; n++);
				a = n;
				for(; n < linesize && line[n] != ' '; n++);

				if(TRUE)/*LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_date*/
				{
					line[n] = '\0';
					props[i] = line + a;
					i++;
				}
				n++;

				for(; n < linesize && line[n] == ' '; n++);
				a = n;
				for(; n < linesize && line[n] != ' '; n++);

				if(TRUE)/*LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_time*/
				{
					line[n] = '\0';
					props[i] = line + a;
					i++;
				}
				n++;

				for(; n < linesize && line[n] == ' '; n++);
				a = n;
				for(; n < linesize && line[n] != ' '; n++);

				if(TRUE)/*LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_method*/
				{
					line[n] = '\0';
					props[i] = line + a;
					i++;
				}
				n++;

				for(; n < linesize && line[n] == ' '; n++);
				a = n;
				for(; n < linesize && line[n] != ' '; n++);

				if(TRUE)/*LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_crc_32*/
				{
					line[n] = '\0';
					props[i] = line + a;
					i++;
				}
				n++;

				for(; n < linesize && line[n] == ' '; n++);
				a = n;
				for(; n < linesize && line[n] != ' '; n++);

				if(TRUE)/*LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_method*/
				{
					line[n] = '\0';
					props[i] = line + a;
					i++;
				}
				n++;

				for(; n < linesize && line[n] == ' '; n++);
				a = n;
				for(; n < linesize && line[n] != ' ' && line[n] != '\n'; n++);

				if(TRUE)/*LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_version*/
				{
					line[n] = '\0';
					props[i] = line + a;
					i++;
				}
				n++;

				props[i] = NULL;

				lsq_archive_iter_set_propsv(entry, (gconstpointer*)props);
				lsq_archive_iter_unref(entry);
			}
		break;
		case REFRESH_STATUS_FINISH:
			g_free(line);
			status = lsq_spawn_command_read_line(spawn_command, 1, &line, &linesize,NULL);
		break;
	}
	g_free(line);

	return TRUE;
}

