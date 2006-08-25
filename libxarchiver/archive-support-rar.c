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
 *  GNU Library General Public License for more details.
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
#include "archive-support-rar.h"

#include "internals.h"

void
lxa_archive_support_rar_init(LXAArchiveSupportRar *support);
void
lxa_archive_support_rar_class_init(LXAArchiveSupportRarClass *supportclass);

gint
lxa_archive_support_rar_add(LXAArchive *archive, GSList *filenames);
gint
lxa_archive_support_rar_extract(LXAArchive *archive, gchar *dest_path, GSList *filenames);
gint
lxa_archive_support_rar_remove(LXAArchive *archive, GSList *filenames);

GType
lxa_archive_support_rar_get_type ()
{
	static GType lxa_archive_support_rar_type = 0;

 	if (!lxa_archive_support_rar_type)
	{
 		static const GTypeInfo lxa_archive_support_rar_info = 
		{
			sizeof (LXAArchiveSupportRarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archive_support_rar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchiveSupportRar),
			0,
			(GInstanceInitFunc) lxa_archive_support_rar_init,
		};

		lxa_archive_support_rar_type = g_type_register_static (LXA_TYPE_ARCHIVE_SUPPORT, "LXAArchiveSupportRar", &lxa_archive_support_rar_info, 0);
	}
	return lxa_archive_support_rar_type;
}

void
lxa_archive_support_rar_init(LXAArchiveSupportRar *support)
{
	LXAArchiveSupport *archive_support = LXA_ARCHIVE_SUPPORT(support);

	archive_support->id = "Rar";

	lxa_archive_support_add_mime(archive_support, "application/x-rar");

	archive_support->add = lxa_archive_support_rar_add;
	archive_support->extract = lxa_archive_support_rar_extract;
	archive_support->remove = lxa_archive_support_rar_remove;
}

void
lxa_archive_support_rar_class_init(LXAArchiveSupportRarClass *supportclass)
{
	/*
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXAArchiveSupportRarClass *klass = LXA_ARCHIVE_SUPPORT_RAR_CLASS (supportclass);
	*/
}

LXAArchiveSupport*
lxa_archive_support_rar_new()
{
	LXAArchiveSupportRar *support = NULL;

	gchar *abs_path = g_find_program_in_path("rar");
	if(abs_path)
	{
		support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_RAR, NULL);
		g_free(abs_path);
	}
	
	return LXA_ARCHIVE_SUPPORT(support);
}

gint
lxa_archive_support_rar_add(LXAArchive *archive, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not rar");
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
			command = g_strconcat("rar a -o+ -ep1 -idp", archive->path, " ", files, NULL);
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}

gint
lxa_archive_support_rar_extract(LXAArchive *archive, gchar *dest_path, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not Rar");
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
				command = g_strconcat("rar x -o+ -idp ", archive->path, " ", files, " ", dest_path, " ", NULL);
				g_debug("Extracting archive '%s' to '%s'", archive->path, dest_path);
				lxa_execute(command, archive, NULL, NULL, NULL, NULL);
			}	
		} else
			return 1;
	}
	return 0;
}

gint
lxa_archive_support_rar_remove(LXAArchive *archive, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not rar");
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
			command = g_strconcat("rar -d ", archive->path, " ", files, NULL);
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}
