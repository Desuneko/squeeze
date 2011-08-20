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

#ifndef __ARCHIVE_ITER_POOL_H__
#define __ARCHIVE_ITER_POOL_H__ 
G_BEGIN_DECLS

typedef struct _LSQArchiveIter LSQArchiveIter;
typedef struct _LSQArchiveEntry LSQArchiveEntry;
typedef struct _LSQArchiveIterPool LSQArchiveIterPool;

LSQArchiveIterPool *lsq_archive_iter_pool_new(void);
void				lsq_archive_iter_pool_free(LSQArchiveIterPool *pool);

gint				lsq_archive_iter_pool_get_size(LSQArchiveIterPool *);
gint				lsq_archive_iter_pool_get_reserved(LSQArchiveIterPool *);
LSQArchiveIter	 *lsq_archive_iter_pool_get_iter(LSQArchiveIterPool *, gint index);

gboolean
lsq_archive_iter_pool_find_iter(LSQArchiveIterPool *ipool, LSQArchiveEntry *entry, LSQArchiveIter **ret_iter, guint *ret_pos);
void
lsq_archive_iter_pool_insert_iter(LSQArchiveIterPool *ipool, LSQArchiveIter *iter, guint pos);
void
lsq_archive_iter_pool_remove_iter(LSQArchiveIterPool *ipool, LSQArchiveIter *iter);

G_END_DECLS

#endif /* __ARCHIVE_H__ */
