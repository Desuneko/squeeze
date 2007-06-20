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

#include "libsqueeze-archive.h"
#include "libsqueeze-vfs-mime.h"
#include "vfs-mime.h"

#include "internals.h"

gint
lsq_archive_mime_lookup(gconstpointer mime_info, gconstpointer mime)
{
	return strcmp(thunar_vfs_mime_info_get_name(((LSQArchiveMime *)mime_info)->mime_info), mime);
}

LSQArchiveMime *
lsq_archive_mime_new(const gchar *mime)
{
	LSQArchiveMime *archive_mime = g_new0(LSQArchiveMime, 1);

	archive_mime->mime_info = thunar_vfs_mime_database_get_info(lsq_mime_database, mime);

	return archive_mime;
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
