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

const gchar            *lsq_tmp_dir;
GSList                 *lsq_archive_support_list;
GSList                 *lsq_opened_archive_list;
ThunarVfsPath          *lsq_relative_base_path;

/*
 * gint
 * lsq_execute(gchar *command)
 *
 * general function for executing child-apps
 */
gint 
lsq_execute(
            gchar *command, 
            LSQArchive *archive, 
            GChildWatchFunc function, 
            GIOFunc f_in, 
            GIOFunc f_out, 
            GIOFunc f_err) G_GNUC_INTERNAL; 

gchar *
lsq_concat_filenames(GSList *filenames) G_GNUC_INTERNAL;
gchar *
lsq_concat_iter_filenames(GSList *file_iters) G_GNUC_INTERNAL;

LSQArchive *
lsq_opened_archive_get_archive(gchar *path) G_GNUC_INTERNAL;

void
lsq_archive_support_view_prepared(LSQArchive *archive, GSList *, gpointer user_data) G_GNUC_INTERNAL;

ThunarVfsMimeDatabase  *lsq_mime_database;
