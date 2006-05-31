/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
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
 * *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __LIBXARCHIVER_ARCHIVE_H__
#define __LIBXARCHIVER_ARCHIVE_H__

G_BEGIN_DECLS

typedef enum
{
	LXA_ARCHIVETYPE_UNKNOWN,
	LXA_ARCHIVETYPE_NONE,
	LXA_ARCHIVETYPE_RAR,
	LXA_ARCHIVETYPE_ZIP,
	LXA_ARCHIVETYPE_ARJ,
	LXA_ARCHIVETYPE_TAR,
	LXA_ARCHIVETYPE_RPM,
	LXA_ARCHIVETYPE_7ZIP,
	LXA_ARCHIVETYPE_ISO,
} LXAArchiveType;

typedef enum
{
	LXA_COMPRESSIONTYPE_NONE,
	LXA_COMPRESSIONTYPE_BZIP2,
	LXA_COMPRESSIONTYPE_GZIP,
} LXACompressionType;



#define LXA_ARCHIVE(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			lxa_archive_get_type(),      \
			LXAArchive))

#define LXA_IS_ARCHIVE(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_archive_get_type()))

#define LXA_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			lxa_archive_get_type(),      \
			LXAArchiveClass))

#define LXA_IS_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			lxa_archive_get_type()))


typedef struct _LXAArchive LXAArchive;


struct _LXAArchive
{
	GObject parent;
	LXAArchiveType type;
	LXACompressionType compression;
	gchar *path;
	gchar *passwd;
	gboolean has_passwd;
};

typedef struct _LXAArchiveClass LXAArchiveClass;

struct _LXAArchiveClass
{
	GObjectClass parent;
}; 

GType lxa_archive_get_type(void);
LXAArchive *lxa_archive_new(gchar *, LXAArchiveType, LXACompressionType);

gint lxa_archive_compress(LXAArchive *archive);
gint lxa_archive_decompress(LXAArchive *archive);

gint lxa_archive_set_compression(LXAArchive *archive, LXACompressionType compression);

gint lxa_archive_add(LXAArchive *archive, gchar **files);


G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVE_H__ */
