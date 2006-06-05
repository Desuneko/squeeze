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

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glib.h>
#include <glib-object.h>
#include <libintl.h>
#include "archive.h"
#include "archive-support.h"
#include "archive-support-gnu-tar.h"

#include "compression-support.h"

#include "internals.h"

#define _(String) gettext(String)

gint
lxa_archive_support_gnu_tar_add(LXAArchive *archive);

gint
lxa_archive_support_gnu_tar_extract(LXAArchive *archive);

gint
lxa_archive_support_gnu_tar_remove(LXAArchive *archive);

void
lxa_archive_support_gnu_tar_child_watch_func(GPid pid, gint status, gpointer data);

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
	LXA_ARCHIVE_SUPPORT(support)->id = "Gnu Tar";
	LXA_ARCHIVE_SUPPORT(support)->type = LXA_ARCHIVETYPE_TAR;

	LXA_ARCHIVE_SUPPORT(support)->add = lxa_archive_support_gnu_tar_add;
	LXA_ARCHIVE_SUPPORT(support)->extract = lxa_archive_support_gnu_tar_extract;
	LXA_ARCHIVE_SUPPORT(support)->remove = lxa_archive_support_gnu_tar_remove;
}

void
lxa_archive_support_gnu_tar_class_init(LXAArchiveSupportGnuTarClass *supportclass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXAArchiveSupportGnuTarClass *klass = LXA_ARCHIVE_SUPPORT_GNU_TAR_CLASS (supportclass);
}

LXAArchiveSupport*
lxa_archive_support_gnu_tar_new()
{
	LXAArchiveSupportGnuTar *support;

	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR, NULL);
	
	return LXA_ARCHIVE_SUPPORT(support);
}

gint
lxa_archive_support_gnu_tar_add(LXAArchive *archive)
{
	g_debug("Adding to tar archive");
	gchar *command;
	GSList *files = archive->tmp_data;

	if(archive->compression == LXA_COMPRESSIONTYPE_NONE)
	{
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar -rf ", archive->path, " ", files->data, NULL);
		else
			command = g_strconcat("tar -cf ", archive->path, " ", files->data, NULL);
	}
	else
	{
		if(g_file_test(archive->tmp_file, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar -rf ", archive->tmp_file, " ", files->data, NULL);
		else
			command = g_strconcat("tar -cf ", archive->tmp_file, " ", files->data, NULL);
	}

	g_debug("EXECUTING: %s\n", command);
	if(lxa_archive_support_execute_with_child_watch(command, archive, lxa_archive_support_gnu_tar_child_watch_func))
		return 1;

	return 0;
}

gint
lxa_archive_support_gnu_tar_extract(LXAArchive *archive)
{
	g_debug("Extracting tar archive");
	gchar *command;
	GSList *files = archive->tmp_data;

	if(archive->compression == LXA_COMPRESSIONTYPE_NONE)
	{
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar -xf ", archive->path, " -C ", files->data, NULL);
		else
			return 1;
	}
	else
	{
		if(g_file_test(archive->tmp_file, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar -xf ", archive->tmp_file, " -C ", files->data, NULL);
		else
			return 1;
	}

	g_debug("EXECUTING: %s\n", command);
	if(lxa_archive_support_execute_with_child_watch(command, archive, lxa_archive_support_gnu_tar_child_watch_func))
		return 1;
	return 0;
}

gint
lxa_archive_support_gnu_tar_remove(LXAArchive *archive)
{
	g_debug("Removing from tar archive");
	gchar *command;

	GSList *files = archive->tmp_data;

	if(archive->compression == LXA_COMPRESSIONTYPE_NONE)
	{
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar --delete -f ", archive->path, " ", files->data, NULL);
		else
			return 2;
	}
	else
	{
		if(g_file_test(archive->tmp_file, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar --delete -f ", archive->tmp_file, " ", files->data, NULL);
		else
			return 2;
	}

	g_debug("EXECUTING: %s\n", command);
	if(lxa_archive_support_execute_with_child_watch(command, archive, lxa_archive_support_gnu_tar_child_watch_func))
		return 1;

	return 0;

}

void
lxa_archive_support_gnu_tar_child_watch_func(GPid pid, gint status, gpointer data)
{
	GSList *find_result;
	LXACompressionSupport *compression_support;
	LXAArchive *archive = data;
	if((archive->compression != LXA_COMPRESSIONTYPE_NONE) && (archive->status != LXA_ARCHIVESTATUS_EXTRACT))
	{
		find_result = g_slist_find_custom(lxa_compression_support_list, &(archive->compression), lookup_compression_support);
		if(find_result)
		{
			compression_support = find_result->data;
			compression_support->compress(archive);
		}
	} else
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_IDLE);

	archive->child_pid = 0;
}
