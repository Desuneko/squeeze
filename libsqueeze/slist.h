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

#ifndef __LSQ_SLIST_H__
#define __LSQ_SLIST_H__

typedef struct _LSQSList LSQSList;

struct _LSQSList {
	LSQArchiveEntry *entry;
	LSQSList *next;
};

LSQSList *
lsq_slist_insert_sorted_single(LSQSList *list, LSQArchiveEntry *entry, GCompareFunc) G_GNUC_INTERNAL;

guint
lsq_slist_length(LSQSList *list) G_GNUC_INTERNAL;

void
lsq_slist_free(LSQSList *list) G_GNUC_INTERNAL;

#endif /* __LSQ_SLIST_H__ */
