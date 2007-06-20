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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__ 
G_BEGIN_DECLS

typedef struct _LSQArchiveEntry LSQArchiveEntry;

struct _LSQArchive
{
	GObject parent;
	gchar              *path;
	ThunarVfsPath      *path_info;
	ThunarVfsInfo      *file_info;
	ThunarVfsMimeInfo  *mime_info;
	LSQArchiveEntry    *root_entry;
	LSQMimeSupport     *support;
	struct {
		guint64 archive_size;
		guint64 content_size;
		guint64 n_files;
		guint64 n_directories;
	} props;
	gchar *temp_dir;
	GSList *monitor_list;
	LSQArchiveIterPool *pool;
};

LSQArchive         *lsq_archive_new(gchar *, const gchar *) G_GNUC_INTERNAL;
void                lsq_archive_state_changed(const LSQArchive *archive) G_GNUC_INTERNAL;

void                lsq_archive_add_children(GSList *files);

G_END_DECLS

#endif /* __ARCHIVE_H__ */
