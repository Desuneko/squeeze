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
#ifdef HAVE_THUNAR_VFS
#include <thunar-vfs/thunar-vfs.h>
#else
#include <gettext.h>
#endif /* HAVE_THUNAR_VFS */

#include "archive.h"
#include "archive-support.h"
#include "archive-support-rar.h"

#include "internals.h"

void
lsq_archive_support_rar_init(LSQArchiveSupportRar *support);
void
lsq_archive_support_rar_class_init(LSQArchiveSupportRarClass *supportclass);

gint
lsq_archive_support_rar_add(LSQArchive *archive, GSList *filenames);
gint
lsq_archive_support_rar_extract(LSQArchive *archive, gchar *dest_path, GSList *filenames);
gint
lsq_archive_support_rar_remove(LSQArchive *archive, GSList *filenames);

GType
lsq_archive_support_rar_get_type ()
{
	static GType lsq_archive_support_rar_type = 0;

 	if (!lsq_archive_support_rar_type)
	{
 		static const GTypeInfo lsq_archive_support_rar_info = 
		{
			sizeof (LSQArchiveSupportRarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_support_rar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchiveSupportRar),
			0,
			(GInstanceInitFunc) lsq_archive_support_rar_init,
		};

		lsq_archive_support_rar_type = g_type_register_static (LSQ_TYPE_ARCHIVE_SUPPORT, "LSQArchiveSupportRar", &lsq_archive_support_rar_info, 0);
	}
	return lsq_archive_support_rar_type;
}

void
lsq_archive_support_rar_init(LSQArchiveSupportRar *support)
{
	LSQArchiveSupport *archive_support = LSQ_ARCHIVE_SUPPORT(support);

	archive_support->id = "Rar";

	lsq_archive_support_add_mime(archive_support, "application/x-rar");

	archive_support->add = lsq_archive_support_rar_add;
	archive_support->extract = lsq_archive_support_rar_extract;
	archive_support->remove = lsq_archive_support_rar_remove;
}

void
lsq_archive_support_rar_class_init(LSQArchiveSupportRarClass *supportclass)
{
	/*
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LSQArchiveSupportRarClass *klass = LSQ_ARCHIVE_SUPPORT_RAR_CLASS (supportclass);
	*/
}

LSQArchiveSupport*
lsq_archive_support_rar_new()
{
	LSQArchiveSupportRar *support = NULL;

	gchar *abs_path = g_find_program_in_path("rar");
	if(abs_path)
	{
		support = g_object_new(LSQ_TYPE_ARCHIVE_SUPPORT_RAR, NULL);
		g_free(abs_path);
	}
	
	return LSQ_ARCHIVE_SUPPORT(support);
}

gint
lsq_archive_support_rar_add(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not rar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		if(!g_strcasecmp((gchar *)archive->mime, "application/x-rar"))
		{
			command = g_strconcat("rar a -o+ -ep1 -idp", archive->path, " ", files, NULL);
			lsq_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}

gint
lsq_archive_support_rar_extract(LSQArchive *archive, gchar *dest_path, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not Rar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-rar"))
			{
				command = g_strconcat("rar x -o+ -idp ", archive->path, " ", files, " ", dest_path, " ", NULL);
				g_debug("Extracting archive '%s' to '%s'", archive->path, dest_path);
				lsq_execute(command, archive, NULL, NULL, NULL, NULL);
			}	
		} else
			return 1;
	}
	return 0;
}

gint
lsq_archive_support_rar_remove(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not rar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		if(!g_strcasecmp((gchar *)archive->mime, "application/x-rar"))
		{
			command = g_strconcat("rar -d ", archive->path, " ", files, NULL);
			lsq_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}
