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
#include "libsqueeze-vfs-mime.h"
#include "vfs-mime.h"

#include "internals.h"

static void
lsq_archive_mime_add_builder(LSQArchiveMime *mime_info, const LSQCommandBuilder *builder);

gint
lsq_archive_mime_lookup(gconstpointer mime_info, gconstpointer mime)
{
	return strcmp(thunar_vfs_mime_info_get_name(((LSQArchiveMime *)mime_info)->mime_info), mime);
}

LSQArchiveMime *
lsq_archive_mime_register_command_builder(const LSQCommandBuilder *builder, const gchar *mime)
{
	LSQArchiveMime *mime_info = NULL;
	GSList *result = g_slist_find_custom(lsq_mime_info_list, mime, lsq_archive_mime_lookup);
	if(!result)
	{
		mime_info = lsq_archive_mime_new(mime);
		lsq_mime_info_list = g_slist_prepend(lsq_mime_info_list, mime_info);
	}
	else
	{
		mime_info = result->data;
	}
	
	lsq_archive_mime_add_builder(mime_info, builder);
	return mime_info;
}

void
lsq_command_builder_register(const LSQCommandBuilder *builder)
{
	int i = 1;
	for(i = 0; builder->mime_types[i]; ++i)
		lsq_archive_mime_register_command_builder(builder, builder->mime_types[i]);
}

LSQArchiveMime *
lsq_archive_mime_new(const gchar *mime)
{
	LSQArchiveMime *archive_mime = g_new0(LSQArchiveMime, 1);

	archive_mime->mime_info = thunar_vfs_mime_database_get_info(lsq_mime_database, mime);

	return archive_mime;
}

static void
lsq_archive_mime_add_builder(LSQArchiveMime *mime_info, const LSQCommandBuilder *builder)
{
	mime_info->command_builders = g_slist_prepend(mime_info->command_builders, (LSQCommandBuilder *)builder);
}

LSQCommandBuilder *
lsq_archive_mime_get_default_builder(const gchar *mime)
{
	LSQArchiveMime *mime_info = NULL;
	GSList *result = g_slist_find_custom(lsq_mime_info_list, mime, lsq_archive_mime_lookup);
	if(result)
	{
		mime_info = result->data;
		if(mime_info->command_builders)
			return mime_info->command_builders->data;
	}
	return NULL;	
}

const gchar *
lsq_archive_mime_get_comment(LSQArchiveMime *mime)
{
	return thunar_vfs_mime_info_get_comment(mime->mime_info);
}

const gchar *
lsq_archive_mime_get_name(LSQArchiveMime *mime)
{
	return thunar_vfs_mime_info_get_name(mime->mime_info);
}
