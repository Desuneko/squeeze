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

		lsq_command_builder_compr_type = g_type_register_static (G_TYPE_OBJECT, "LSQCommandBuilderCompr", &lsq_command_builder_compr_info, 0);
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

	command_builder->mime_types = g_new0(gchar *, 4);
	command_builder->mime_types[0] = "application/x-gzip";
	command_builder->mime_types[1] = "application/x-bzip";
	command_builder->mime_types[2] = "application/x-lzop";
	command_builder->mime_types[3] = "application/x-compress";
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


static LSQArchiveCommand *
lsq_command_builder_compr_build_add(LSQCommandBuilder *builder, LSQArchive *archive, GSList *filenames)
{
	gchar *files = lsq_concat_filenames(filenames);
	LSQArchiveCommand *spawn = lsq_spawn_command_new("Add", archive, "compr %3$s -r %1$s %2$s", files, NULL, NULL);
	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_compr_build_remove(LSQCommandBuilder *builder, LSQArchive *archive, GSList *filenames)
{
	return NULL;
}

static LSQArchiveCommand *
lsq_command_builder_compr_build_extract(LSQCommandBuilder *builder, LSQArchive *archive, const gchar *dest_path, GSList *filenames)
{
	gchar *files = lsq_concat_filenames(filenames);
	gchar *options = g_strconcat(" -d ", dest_path, NULL);

	LSQArchiveCommand *spawn = lsq_spawn_command_new("Extract", archive, "uncompr -o %1$s %2$s %3$s", files, options, NULL);

	g_free(options);
	g_free(files);
	return spawn;
}

static LSQArchiveCommand *
lsq_command_builder_compr_build_refresh(LSQCommandBuilder *builder, LSQArchive *archive)
{
	return NULL;
}

