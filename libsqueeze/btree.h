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

#ifndef __LSQ_BTREE_H__
#define __LSQ_BTREE_H__

typedef struct _LSQBTree LSQBTree;

struct _LSQBTree {
    LSQArchiveEntry *entry;
    LSQBTree *next;
    LSQBTree *left;
    LSQBTree *right;
    gint balance;
};

LSQBTree *
lsq_btree_insert_sorted_single (
        LSQBTree *list,
        LSQArchiveEntry *entry,
        GCompareFunc ) G_GNUC_INTERNAL;

guint
lsq_btree_length ( LSQBTree *list ) G_GNUC_INTERNAL;

void
lsq_btree_free ( LSQBTree *list ) G_GNUC_INTERNAL;

LSQBTree *
lsq_btree_flatten ( LSQBTree *list ) G_GNUC_INTERNAL;

#endif /* __LSQ_BTREE_H__ */
