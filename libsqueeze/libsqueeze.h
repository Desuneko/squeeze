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

#include <libxfce4util/libxfce4util.h>

#include <libsqueeze/command-option.h>
#include <libsqueeze/support-template.h>
#include <libsqueeze/archive-iter-pool.h>
#include <libsqueeze/archive.h>
#include <libsqueeze/archive-iter.h>
#include <libsqueeze/datetime.h>

#include <libsqueeze/libsqueeze-view.h>

G_BEGIN_DECLS

/*
 * void
 * lsq_init()
 */
void lsq_init ( void );

/*
 * void
 * lsq_shutdown()
 */
void lsq_shutdown ( void );

/*
 * gint
 * lsq_new_archive(gchar *path,
 *				 LSQArchiveType type,
 *				 gboolean overwrite,
 *				 LSQArchive &&lp_archive)
 *
 * returns:
 * 0 -- success
 */
gint
lsq_new_archive (
        GFile *,
        gboolean overwrite,
        LSQArchive **lp_archive
    );

/*
 * gint 
 * lsq_open_archive(gchar *path,
 *				  LSQArchive **lp_archive)
 *
 * returns:
 * 0 -- success
 */
gint
lsq_open_archive (
        GFile *, 
        LSQArchive **lp_archive
    );

/*
 * void 
 * lsq_close_archive( LSQArchive **lp_archive )
 *
 */
void 
lsq_close_archive ( LSQArchive *archive );

GSList *
lsq_get_supported_mime_types ( LSQCommandType type );

gboolean
lsq_is_supported ( const gchar *filename );

gchar **
lsq_iter_list_to_strv ( GSList *list ) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* __LIBSQUEEZE_H__ */
