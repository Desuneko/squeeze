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
#ifndef __LIBXARCHIVER_H__
#define __LIBXARCHIVER_H__

#include <libxarchiver/archive.h>

G_BEGIN_DECLS

void lxa_init();

int  lxa_destroy();

/*
 * XAArchive*
 * lxa_new_archive(gchar *path, LXAArchiveType type, gboolean overwrite)
 *
 */
gint
lxa_new_archive(gchar *path, LXAArchiveType, LXACompressionType, gboolean overwrite, LXAArchive **lp_archive);

/*
 *
 * XAArchive*
 * lxa_open_archive(gchar *path)
 *   path: path to archive to open
 *
 * returns:
 * archive object when archive exists.
 * NULL when archive does not exist.
 *
 */
gint
lxa_open_archive(gchar *path, LXAArchive **lp_archive);

void lxa_close_archive(LXAArchive *archive);

G_END_DECLS

#endif /* __LIBXARCHIVER_H__ */
