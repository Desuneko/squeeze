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

GSList                 *lsq_archive_support_list;
GSList                 *lsq_opened_archive_list;
ThunarVfsPath          *lsq_relative_base_path;

/*
 * gint
 * lsq_execute(gchar *command)
 *
 * general function for executing child-apps
 */
ThunarVfsMimeDatabase  *lsq_mime_database;

gchar *
lsq_concat_filenames(GSList *filenames);

LSQArchive *
lsq_opened_archive_get_archive(gchar *path);
