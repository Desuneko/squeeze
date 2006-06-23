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
#include "archive-support.h"
#include "archive-support-gnu-tar.h"

#include "compression-support.h"
#include "compression-support-gzip.h"
#include "compression-support-bzip2.h"

#include "internals.h"

void
lxa_init()
{
	lxa_archive_support_list = g_slist_alloc();
	lxa_archive_support_list = g_slist_prepend(lxa_archive_support_list, lxa_archive_support_gnu_tar_new());

	lxa_compression_support_list = g_slist_alloc();
	lxa_compression_support_list = g_slist_prepend(lxa_compression_support_list, lxa_compression_support_gzip_new());
	lxa_compression_support_list = g_slist_prepend(lxa_compression_support_list, lxa_compression_support_bzip2_new());

	lxa_tmp_dir = g_get_tmp_dir();
#ifdef DEBUG
	g_debug("lxa_tmp_dir: %s\n", lxa_tmp_dir);
	g_debug("lxa_cmp_list_length: %d\n", g_slist_length(lxa_compression_support_list));
#endif
}

int
lxa_destroy()
{
	g_slist_foreach(lxa_tmp_files_list,(void *)g_unlink, NULL);
	g_slist_foreach(lxa_tmp_files_list,(void *)g_free, NULL);
	g_slist_free(lxa_tmp_files_list);
	return 0;
}

/*
 * XAArchive* lxa_new_archive(gchar *path, LXAArchiveType type, gboolean overwrite)
 *
 */
gint
lxa_new_archive(gchar *path, LXAArchiveType type, LXACompressionType compression, gboolean overwrite, LXAArchive **lp_archive)
{
	if(overwrite)
		g_unlink(path);

	if(g_file_test(path, G_FILE_TEST_EXISTS))
	{
		(*lp_archive) = NULL;
		return 1;
	}

	LXAArchive *archive = lxa_archive_new(path, type, compression);
	(*lp_archive) = archive;
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

	LXAArchive *archive = lxa_archive_new(path, LXA_ARCHIVETYPE_UNKNOWN, LXA_COMPRESSIONTYPE_UNKNOWN);
	(*lp_archive) = archive;
	return 0;
}

void
lxa_close_archive(LXAArchive *archive)
{
	g_unlink(archive->tmp_file);
}
