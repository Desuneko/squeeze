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
#include "archive-support-gnu-tar.h"

#include "internals.h"

void
lxa_archive_support_gnu_tar_init(LXAArchiveSupportGnuTar *support);
void
lxa_archive_support_gnu_tar_class_init(LXAArchiveSupportGnuTarClass *supportclass);

GType
lxa_archive_support_gnu_tar_get_type ()
{
	static GType lxa_archive_support_gnu_tar_type = 0;

 	if (!lxa_archive_support_gnu_tar_type)
	{
 		static const GTypeInfo lxa_archive_support_gnu_tar_info = 
		{
			sizeof (LXAArchiveSupportGnuTarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archive_support_gnu_tar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchiveSupportGnuTar),
			0,
			(GInstanceInitFunc) lxa_archive_support_gnu_tar_init,
		};

		lxa_archive_support_gnu_tar_type = g_type_register_static (LXA_TYPE_ARCHIVE_SUPPORT, "LXAArchiveSupportGnuTar", &lxa_archive_support_gnu_tar_info, 0);
	}
	return lxa_archive_support_gnu_tar_type;
}

void
lxa_archive_support_gnu_tar_init(LXAArchiveSupportGnuTar *support)
{
	LXAArchiveSupport *archive_support = LXA_ARCHIVE_SUPPORT(support);

	archive_support->id = "Gnu Tar";

	if(g_find_program_in_path("gtar"))
		support->app_name = "gtar";
	else
		support->app_name = "tar";

	lxa_archive_support_add_mime(archive_support, "application/x-tar");
	/* Check for existence of compress -- required for x-tarz */
	if(g_find_program_in_path("compress"))
		lxa_archive_support_add_mime(archive_support, "application/x-tarz");
	/* Check for existence of gzip -- required for x-compressed-tar*/
	if(g_find_program_in_path("gzip"))
		lxa_archive_support_add_mime(archive_support, "application/x-compressed-tar");
	/* Check for existence of bzip2 -- required for x-bzip-compressed-tar */
	if(g_find_program_in_path("bzip2"))
		lxa_archive_support_add_mime(archive_support, "application/x-bzip-compressed-tar");

	archive_support->add = lxa_archive_support_gnu_tar_add;
	archive_support->extract = lxa_archive_support_gnu_tar_extract;
	archive_support->remove = lxa_archive_support_gnu_tar_remove;
}

void
lxa_archive_support_gnu_tar_class_init(LXAArchiveSupportGnuTarClass *supportclass)
{
	/* TODO
	 * Implement properties.
	 *
	 */
}

LXAArchiveSupport*
lxa_archive_support_gnu_tar_new()
{
	LXAArchiveSupportGnuTar *support;

	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR, NULL);

	return LXA_ARCHIVE_SUPPORT(support);
}

gint
lxa_archive_support_gnu_tar_add(LXAArchiveSupport *support, LXAArchive *archive, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_GNU_TAR(support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -rf ", archive->path, " ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tarz"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -Zrf ", archive->path, " ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -zrf ", archive->path, " ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-bzip-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -jrf ", archive->path, " ", files, NULL);
		} else
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -cf ", archive->path, " ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tarz"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -Zcf ", archive->path, " ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -zcf ", archive->path, " ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-bzip-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -jcf ", archive->path, " ", files, NULL);
		}
		if(command)
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
	}
	return 0;
}

gint
lxa_archive_support_gnu_tar_extract(LXAArchiveSupport *support, LXAArchive *archive, gchar *dest_path, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_GNU_TAR(support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -xf ", archive->path, " -C ", dest_path, " ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tarz"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -Zxf ", archive->path, " -C ", dest_path, " ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -zxf ", archive->path, " -C ", dest_path, " ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-bzip-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -jxf ", archive->path, " -C ", dest_path, " ", files, NULL);
		} else
			return 1;
		if(command)
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
	}
	return 0;
}

gint
lxa_archive_support_gnu_tar_remove(LXAArchiveSupport *support, LXAArchive *archive, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_GNU_TAR(support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -f ", archive->path, " --delete ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tarz"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -Zf ", archive->path, " --delete ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -zf ", archive->path, " --delete ", files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-bzip-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(support)->app_name, " -jf ", archive->path, " --delete ", files, NULL);
		} else
			return 1;
		if(command)
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
	}
	return 0;
}
