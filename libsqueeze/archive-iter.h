/*
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifndef __LIBSQUEEZE_ARCHIVE_ITER_H__
#define __LIBSQUEEZE_ARCHIVE_ITER_H__ 
G_BEGIN_DECLS


struct _LSQArchiveIter
{
    LSQArchive *archive;
    LSQArchiveEntry *entry;
    LSQArchiveIter *parent;
    guint ref_count;
};

void
lsq_archive_init_iter ( LSQArchive * );
void
lsq_archive_free_iter ( LSQArchive * );


void
lsq_archive_iter_remove (
        LSQArchiveIter *,
        gboolean
    );

#ifdef DEBUG
LSQArchiveIter *
_lsq_archive_iter_ref (
        LSQArchiveIter *iter,
        const gchar *
    );
void
_lsq_archive_iter_unref (
        LSQArchiveIter *iter,
        const gchar *
    );
/*
#define lsq_archive_iter_ref(iter) _lsq_archive_iter_ref(iter, G_STRLOC)
#define lsq_archive_iter_unref(iter) _lsq_archive_iter_unref(iter, G_STRLOC)
*/
#endif

LSQArchiveIter *
lsq_archive_iter_ref ( LSQArchiveIter *iter );
void
lsq_archive_iter_unref ( LSQArchiveIter *iter );

LSQArchiveIter *
lsq_archive_add_file (
        LSQArchive *archive,
        const gchar *path
    ) G_GNUC_WARN_UNUSED_RESULT;

LSQArchiveIter *
lsq_archive_iter_add_file (
        LSQArchiveIter *parent,
        const gchar *filename
    ) G_GNUC_WARN_UNUSED_RESULT;

void
lsq_archive_iter_set_prop_value (
        LSQArchiveIter *iter,
        guint n,
        const GValue *value
    );
void
lsq_archive_iter_set_prop (
        LSQArchiveIter *iter,
        guint n,
        gpointer value
    );
void
lsq_archive_iter_set_props (
        LSQArchiveIter *iter,
        ...
    ) G_GNUC_NULL_TERMINATED;
void
lsq_archive_iter_set_propsv (
        LSQArchiveIter *iter,
        gpointer *props
    );

G_END_DECLS

#endif /* __LIBSQUEEZE_ARCHIVE_ITER_H__ */
