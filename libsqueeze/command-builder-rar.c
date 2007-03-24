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
#include "command-builder-rar.h"

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

	command_builder->build_add = lsq_command_builder_rar_build_add;
	command_builder->build_extract = lsq_command_builder_rar_build_extract;
	command_builder->build_remove = lsq_command_builder_rar_build_remove;
	command_builder->build_refresh = lsq_command_builder_rar_build_refresh;

	command_builder->mime_types = g_new0(gchar *, 2);
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
	LSQArchiveCommand *spawn = lsq_spawn_command_new("Add", archive, "rar %3$s -r %1$s %2$s", files, NULL, NULL);
	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_rar_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *iter_files)
{
	gchar *files = lsq_concat_iter_filenames(iter_files);

	LSQArchiveCommand *spawn = lsq_spawn_command_new("Remove", archive, "rar -d %1$s %2$s", files, NULL, NULL);

	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_rar_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *filenames)
{
	gchar *files = lsq_concat_filenames(filenames);
	gchar *options = g_strconcat(" -d ", dest_path, NULL);

	LSQArchiveCommand *spawn = lsq_spawn_command_new("Extract", archive, "unrar -o %1$s %2$s %3$s", files, options, NULL);

	g_free(options);
	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_rar_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive)
{
	LSQArchiveCommand *spawn = lsq_spawn_command_new("Refresh", archive, "unrar -lv -qq %1$s", NULL, NULL, NULL);

	return spawn;
}

LSQCommandBuilder *
lsq_command_builder_rar_new()
{
	LSQCommandBuilder *builder;

	builder = g_object_new(lsq_command_builder_rar_get_type(), NULL);

	return builder;
}
