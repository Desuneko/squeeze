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
#include "archive-iter.h"
#include "archive-tempfs.h"
#include "archive.h"
#include "builder-settings.h"

static void
lsq_command_builder_class_init(LSQCommandBuilderClass *);
static void
lsq_command_builder_init(LSQCommandBuilder *archive);
static void
lsq_command_builder_dispose(GObject *object);
static void
lsq_command_builder_finalize(GObject *object);

static LSQArchiveCommand *
lsq_command_builder_build_open(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files);

static GObjectClass *parent_class;

GType
lsq_command_builder_get_type ()
{
	static GType lsq_command_builder_type = 0;

	if (!lsq_command_builder_type)
	{
		static const GTypeInfo lsq_command_builder_info = 
		{
			sizeof (LSQCommandBuilderClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_command_builder_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQCommandBuilder),
			0,
			(GInstanceInitFunc) lsq_command_builder_init,
			NULL
		};

		lsq_command_builder_type = g_type_register_static (G_TYPE_OBJECT, "LSQCommandBuilder", &lsq_command_builder_info, 0);
	}
	return lsq_command_builder_type;
}

static void
lsq_command_builder_class_init(LSQCommandBuilderClass *command_builder_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(command_builder_class);

	object_class->dispose = lsq_command_builder_dispose;
	object_class->finalize = lsq_command_builder_finalize;

	parent_class = g_type_class_peek(G_TYPE_OBJECT); 

}

static void
lsq_command_builder_init(LSQCommandBuilder *command_builder)
{
	command_builder->settings = lsq_builder_settings_new();
	command_builder->build_open = lsq_command_builder_build_open;
}

/**
 * lsq_command_builder_dispose:
 *
 * @object: LSQCommandBuilder object
 *
 */
static void
lsq_command_builder_dispose(GObject *object)
{

	parent_class->dispose(object);
}

/**
 * lsq_command_builder_finalize:
 *
 * @object: LSQCommandBuilder object
 *
 */
static void
lsq_command_builder_finalize(GObject *object)
{
	parent_class->finalize(object);
}


static LSQArchiveCommand *
lsq_command_builder_build_open(LSQCommandBuilder *builder, LSQArchive *archive, GSList *files)
{
	LSQArchiveCommand *extract = builder->build_extract(builder, archive, lsq_tempfs_get_root_dir(archive), files);
	LSQArchiveCommand *launch = lsq_xfce_launch_command_new(_("Execute"), 
	                                                 archive, lsq_tempfs_get_root_dir(archive), files);
	LSQArchiveCommand *macro = lsq_macro_command_new(archive);

	lsq_macro_command_append(LSQ_MACRO_COMMAND(macro), extract);
	lsq_macro_command_append(LSQ_MACRO_COMMAND(macro), launch);

	g_object_unref(extract);
	g_object_unref(launch);
	return macro;
}

LSQBuilderSettings *
lsq_command_builder_get_settings(LSQCommandBuilder *builder)
{
	g_object_ref(builder->settings);
	return builder->settings;
}
