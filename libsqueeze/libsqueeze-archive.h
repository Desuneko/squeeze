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

#ifndef __LIBSQUEEZE_ARCHIVE_H__
#define __LIBSQUEEZE_ARCHIVE_H__ 
G_BEGIN_DECLS

enum
{
	LSQ_ARCHIVE_PROP_FILENAME = 0,
	LSQ_ARCHIVE_PROP_MIME_TYPE,
	LSQ_ARCHIVE_PROP_USER
};

typedef struct _LSQArchiveIter LSQArchiveIter;

#define LSQ_TYPE_ARCHIVE lsq_archive_get_type()

#define LSQ_ARCHIVE(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_ARCHIVE,      \
			LSQArchive))

#define LSQ_IS_ARCHIVE(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_ARCHIVE))

#define LSQ_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			LSQ_TYPE_ARCHIVE,      \
			LSQArchiveClass))

#define LSQ_IS_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			LSQ_TYPE_ARCHIVE))

typedef struct _LSQArchive LSQArchive;
typedef struct _LSQArchiveClass LSQArchiveClass;

GType lsq_archive_get_type(void);

const gchar    *lsq_archive_get_path(const LSQArchive *archive);
gchar          *lsq_archive_get_filename(const LSQArchive *archive);
const gchar    *lsq_archive_get_mimetype(const LSQArchive *archive);
gboolean        lsq_archive_exists(const LSQArchive *archive);

#ifdef DEBUG
LSQArchiveIter *_lsq_archive_iter_ref(LSQArchiveIter *iter, const gchar*, int);
void            _lsq_archive_iter_unref(LSQArchiveIter *iter, const gchar*, int);
/*
#define lsq_archive_iter_ref(iter) _lsq_archive_iter_ref(iter, __FILE__, __LINE__)
#define lsq_archive_iter_unref(iter) _lsq_archive_iter_unref(iter, __FILE__, __LINE__)
*/
#endif

LSQArchiveIter *lsq_archive_iter_ref(LSQArchiveIter *iter);
void            lsq_archive_iter_unref(LSQArchiveIter *iter);


G_END_DECLS

#endif /* __LIBSQUEEZE_ARCHIVE_H__ */
