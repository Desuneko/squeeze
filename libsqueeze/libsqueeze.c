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


#include <config.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gettext.h>

#include "libsqueeze.h"
#include "libsqueeze/archive-support-zip.h"
#include "libsqueeze/archive-support-gnu-tar.h"

#include "internals.h"

void
lsq_init()
{
	lsq_tmp_dir = g_get_tmp_dir();

	lsq_mime_init();

	lsq_register_support(lsq_archive_support_gnu_tar_new());
	lsq_register_support(lsq_archive_support_zip_new());

/*
	TODO: Implement right commands in unrar
	lsq_register_support(lsq_archive_support_rar_new());
	lsq_register_support(lsq_archive_support_unrar_new());
	*/
}

void
lsq_destroy()
{
	g_slist_foreach(lsq_archive_support_list, (GFunc)g_object_unref, NULL);

	lsq_mime_destroy();
}

/*
 * XAArchive* lsq_new_archive(gchar *path, LSQArchiveType type, gboolean overwrite)
 *
 */
gint
lsq_new_archive(gchar *path, gboolean overwrite, gchar *mime, LSQArchive **lp_archive)
{
	if(overwrite)
		g_unlink(path);

	if(g_file_test(path, G_FILE_TEST_EXISTS))
	{
		(*lp_archive) = NULL;
		return 1;
	}

	LSQArchive *archive = lsq_archive_new(path, mime);
	(*lp_archive) = archive;
	if(!archive)
		return 1;
	return 0;
}

/*
 *
 * XAArchive* lsq_open_archive(gchar *path)
 *
 */
gint
lsq_open_archive(gchar *path, LSQArchive **lp_archive)
{
	if(!g_file_test(path, G_FILE_TEST_EXISTS))
	{
		(*lp_archive) = NULL;
		return 1;
	}

	LSQArchive *archive = lsq_archive_new(path, NULL);
	(*lp_archive) = archive;
	if(!archive)
		return 1;
	return 0;
}

void
lsq_close_archive(LSQArchive *archive)
{
	g_object_unref(archive);
}
