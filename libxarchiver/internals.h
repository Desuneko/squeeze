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

const gchar            *lxa_tmp_dir;
ThunarVfsMimeDatabase  *lxa_mime_database;
GSList                 *lxa_archive_support_list;

/*
 * gint
 * lxa_execute(gchar *command)
 *
 * general function for executing child-apps
 */
gint 
lxa_execute(
            gchar *command, 
            LXAArchive *archive, 
            GChildWatchFunc function, 
            GIOFunc f_in, 
            GIOFunc f_out, 
            GIOFunc f_err);

gchar *
lxa_concat_filenames(GSList *filenames);

