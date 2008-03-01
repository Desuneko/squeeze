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

/*
 * temp extract
 * get mime type
 * copy from local
 * create dir
 * check modify
 */

void lsq_tempfs_clean_root_dir(LSQArchive *archive);

const gchar* lsq_tempfs_get_root_dir(LSQArchive *archive);

gboolean lsq_tempfs_make_root_dir(LSQArchive *archive);

gboolean lsq_tempfs_make_dir(LSQArchive *archive, const gchar *path, gint mode);

gboolean lsq_tempfs_chmod(LSQArchive *archive, const gchar *path, gint mode);

gboolean
lsq_tempfs_monitor_file(LSQArchive *archive, const gchar *path) G_GNUC_INTERNAL;

gboolean
lsq_tempfs_changed_file(LSQArchive *archive, const gchar *path) G_GNUC_INTERNAL;

gchar *lsq_archive_request_temp_file(LSQArchive *archive, const gchar *suffix);

