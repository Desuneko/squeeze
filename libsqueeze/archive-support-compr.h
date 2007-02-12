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
#ifndef __LIBSQUEEZE_ARCHIVE_SUPPORT_COMPR_H__
#define __LIBSQUEEZE_ARCHIVE_SUPPORT_COMPR_H__

G_BEGIN_DECLS


#define LSQ_TYPE_ARCHIVE_SUPPORT_COMPR lsq_archive_support_compr_get_type()

#define LSQ_ARCHIVE_SUPPORT_COMPR(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT_COMPR,      \
			LSQArchiveSupportCompr))

#define LSQ_IS_ARCHIVE_SUPPORT_COMPR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT_COMPR))

#define LSQ_ARCHIVE_SUPPORT_COMPR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_ARCHIVE_SUPPORT_COMPR,      \
			LSQArchiveSupportComprClass))

#define LSQ_IS_ARCHIVE_SUPPORT_COMPR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_ARCHIVE_SUPPORT_COMPR))

typedef struct _LSQArchiveSupportCompr LSQArchiveSupportCompr;

struct _LSQArchiveSupportCompr
{
	LSQArchiveSupport parent;
};

typedef struct _LSQArchiveSupportComprClass LSQArchiveSupportComprClass;

struct _LSQArchiveSupportComprClass
{
	LSQArchiveSupportClass parent;
}; 

GType                lsq_archive_support_compr_get_type(void) G_GNUC_INTERNAL;
LSQArchiveSupport *  lsq_archive_support_compr_new() G_GNUC_INTERNAL;

G_END_DECLS

#endif /* __LIBSQUEEZE_ARCHIVE_SUPPORT_COMPR_H__ */
