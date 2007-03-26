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

#ifndef __LIBSQUEEZE_H__
#define __LIBSQUEEZE_H__

#include <libsqueeze/libsqueeze-archive.h>
#include <libsqueeze/libsqueeze-vfs-mime.h>
#include <libsqueeze/libsqueeze-view.h>


G_BEGIN_DECLS

/*
 * void
 * lsq_init()
 */
void lsq_init();

/*
 * void
 * lsq_shutdown()
 */
void lsq_shutdown();

/*
 * gint
 * lsq_new_archive(gchar *path,
 *                 LSQArchiveType type,
 *                 gboolean overwrite,
 *                 LSQArchive &&lp_archive)
 *
 * returns:
 * 0 -- success
 */
gint
lsq_new_archive( gchar *path,
                 gboolean overwrite,
                 gchar *mime,
                 LSQArchive **lp_archive);

/*
 * gint 
 * lsq_open_archive(gchar *path,
 *                  LSQArchive **lp_archive)
 *
 * returns:
 * 0 -- success
 */
gint
lsq_open_archive( gchar *path, 
                  LSQArchive **lp_archive);

/*
 * void 
 * lsq_close_archive( LSQArchive **lp_archive )
 *
 */
void 
lsq_close_archive( LSQArchive *archive );

GSList *
lsq_get_supported_mime_types();

G_END_DECLS

#endif /* __LIBSQUEEZE_H__ */
