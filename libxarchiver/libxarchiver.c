/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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


#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "libxarchiver.h"
#include "libxarchiver/archive-support-zip.h"
#include "libxarchiver/archive-support-rar.h"
#include "libxarchiver/archive-support-unrar.h"
#include "libxarchiver/archive-support-gnu-tar.h"


#include "internals.h"

void
lxa_init()
{
	lxa_tmp_dir = g_get_tmp_dir();
	lxa_mime_database = thunar_vfs_mime_database_get_default();

	lxa_register_support(lxa_archive_support_zip_new());
	lxa_register_support(lxa_archive_support_gnu_tar_new());
	/* TODO: Implement right commands in unrar
	lxa_register_support(lxa_archive_support_rar_new());
	lxa_register_support(lxa_archive_support_unrar_new());
	*/
}

void
lxa_destroy()
{
	g_slist_foreach(lxa_archive_support_list, (GFunc)g_object_unref, NULL);
	g_object_unref(lxa_mime_database);
}

/*
 * XAArchive* lxa_new_archive(gchar *path, LXAArchiveType type, gboolean overwrite)
 *
 */
gint
lxa_new_archive(gchar *path, gboolean overwrite, gchar *mime, LXAArchive **lp_archive)
{
	if(overwrite)
		g_unlink(path);

	if(g_file_test(path, G_FILE_TEST_EXISTS))
	{
		(*lp_archive) = NULL;
		return 1;
	}

	LXAArchive *archive = lxa_archive_new(path, mime);
	(*lp_archive) = archive;
	if(!archive)
		return 1;
	return 0;
}

/*
 *
 * XAArchive* lxa_open_archive(gchar *path)
 *
 */
gint
lxa_open_archive(gchar *path, LXAArchive **lp_archive)
{
	if(!g_file_test(path, G_FILE_TEST_EXISTS))
	{
		(*lp_archive) = NULL;
		return 1;
	}

	LXAArchive *archive = lxa_archive_new(path, NULL);
	(*lp_archive) = archive;
	if(!archive)
		return 1;
	return 0;
}

void
lxa_close_archive(LXAArchive *archive)
{
	g_object_unref(archive);
}
