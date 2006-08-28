/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Libunrary General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define EXO_API_SUBJECT_TO_CHANGE

#include <glib.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>

#include "archive.h"
#include "archive-support.h"
#include "archive-support-unrar.h"

#include "internals.h"

void
lxa_archive_support_unrar_init(LXAArchiveSupportUnrar *support);
void
lxa_archive_support_unrar_class_init(LXAArchiveSupportUnrarClass *supportclass);

GType
lxa_archive_support_unrar_get_type ()
{
	static GType lxa_archive_support_unrar_type = 0;

 	if (!lxa_archive_support_unrar_type)
	{
 		static const GTypeInfo lxa_archive_support_unrar_info = 
		{
			sizeof (LXAArchiveSupportUnrarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archive_support_unrar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchiveSupportUnrar),
			0,
			(GInstanceInitFunc) lxa_archive_support_unrar_init,
		};

		lxa_archive_support_unrar_type = g_type_register_static (LXA_TYPE_ARCHIVE_SUPPORT, "LXAArchiveSupportUnrar", &lxa_archive_support_unrar_info, 0);
	}
	return lxa_archive_support_unrar_type;
}

void
lxa_archive_support_unrar_init(LXAArchiveSupportUnrar *support)
{
	LXAArchiveSupport *archive_support = LXA_ARCHIVE_SUPPORT(support);

	archive_support->id = "Unrar";

	lxa_archive_support_add_mime(archive_support, "application/x-rar");

	archive_support->extract = lxa_archive_support_unrar_extract;
	archive_support->remove = lxa_archive_support_unrar_remove;
}

void
lxa_archive_support_unrar_class_init(LXAArchiveSupportUnrarClass *supportclass)
{
	/*
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXAArchiveSupportUnrarClass *klass = LXA_ARCHIVE_SUPPORT_UNRAR_CLASS (supportclass);
	*/
}

LXAArchiveSupport*
lxa_archive_support_unrar_new()
{
	LXAArchiveSupportUnrar *support = NULL;

	gchar *abs_path = g_find_program_in_path("unrar");
	if(abs_path)
	{
		support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_UNRAR, NULL);
		g_free(abs_path);
	}
	
	return LXA_ARCHIVE_SUPPORT(support);
}


gint
lxa_archive_support_unrar_extract(LXAArchive *archive, gchar *dest_path, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_UNRAR(archive->support))
	{
		g_critical("Support is not Unrar");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-rar"))
			{
				/* TODO: Fix commandline issues
				command = g_strconcat("unrar x -o+ -idp ", archive->path, " ", files, " ", dest_path, " ", NULL);
				*/
				g_debug("Extracting archive '%s' to '%s'", archive->path, dest_path);
				lxa_execute(command, archive, NULL, NULL, NULL, NULL);
			}	
		} else
			return 1;
	}
	return 0;
}

gint
lxa_archive_support_unrar_remove(LXAArchive *archive, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_UNRAR(archive->support))
	{
		g_critical("Support is not unrar");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(!g_strcasecmp((gchar *)archive->mime, "application/x-rar"))
		{
			/* TODO: Fix commandline issues
			command = g_strconcat("unrar -d ", archive->path, " ", files, NULL);
			*/
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}
