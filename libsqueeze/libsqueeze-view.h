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

#ifndef __LIBSQUEEZE_VIEW_H__
#define __LIBSQUEEZE_VIEW_H__ 
G_BEGIN_DECLS

const gchar *       lsq_archive_get_filename(LSQArchive *archive);
const gchar *       lsq_archive_get_mimetype(LSQArchive *archive);

gboolean            lsq_archive_iter_is_real(const LSQArchiveIter *);
LSQArchiveIter *    lsq_archive_iter_get_real_parent(LSQArchiveIter *);

gboolean            lsq_archive_iter_is_directory(const LSQArchiveIter *);
guint               lsq_archive_iter_n_children(const LSQArchiveIter *);
LSQArchiveIter     *lsq_archive_iter_nth_child(LSQArchiveIter *, guint);
LSQArchiveIter     *lsq_archive_iter_get_child(LSQArchiveIter *, const gchar *);

gboolean            lsq_archive_iter_get_prop_value(const LSQArchiveIter *iter, guint n, GValue *value);
const gchar        *lsq_archive_iter_get_filename(const LSQArchiveIter *);

LSQArchiveIter     *lsq_archive_get_iter(LSQArchive *archive, const gchar *path);

GType               lsq_archive_get_entry_property_type(LSQArchive *archive, guint n);
const gchar        *lsq_archive_get_entry_property_name(LSQArchive *archive, guint n);
guint               lsq_archive_n_entry_properties(LSQArchive *archive);

G_END_DECLS

#endif /* __LIBSQUEEZE_VIEW_H__ */