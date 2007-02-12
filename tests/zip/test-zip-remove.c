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
#include <glib.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>

int main()
{
	g_type_init();

	LSQArchive *archive = NULL;
	gchar *current_dir = g_get_current_dir();
	gchar *path = g_strconcat(current_dir, "/data/test.zip", NULL);

	thunar_vfs_init();
	lsq_init();

	lsq_new_archive(path, TRUE, "application/zip", &archive);


	lsq_close_archive(archive);

	lsq_shutdown();
	thunar_vfs_shutdown();

	g_free(path);
	g_free(current_dir);
	return 0;
}
