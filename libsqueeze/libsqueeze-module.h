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

#ifndef __LIBSQUEEZE_MODULE_H__
#define __LIBSQUEEZE_MODULE_H__ 
G_BEGIN_DECLS

void            lsq_archive_iter_set_prop_value(LSQArchiveIter *, guint, const GValue *);
void            lsq_archive_iter_set_prop(LSQArchiveIter *, guint, gconstpointer);
void            lsq_archive_iter_set_props(LSQArchiveIter *, ...);
void            lsq_archive_iter_set_propsv(LSQArchiveIter *, gconstpointer *);

LSQArchiveIter *lsq_archive_iter_add_file(LSQArchiveIter *, const gchar *);

LSQArchiveIter *lsq_archive_add_file(LSQArchive *, const gchar *);

void            lsq_archive_clear_entry_property_types(LSQArchive *);
void            lsq_archive_set_entry_property_type(LSQArchive *, guint, GType, const gchar *);
void            lsq_archive_set_entry_property_types(LSQArchive *, ...);
void            lsq_archive_set_entry_property_typesv(LSQArchive *, GType *, const gchar **);

G_END_DECLS

#endif /* __LIBSQUEEZE_MODULE_H__ */
