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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __LIBXARCHIVER_ARCHIVE_SUPPORT_ZIP_H__
#define __LIBXARCHIVER_ARCHIVE_SUPPORT_ZIP_H__

G_BEGIN_DECLS


#define LXA_TYPE_ARCHIVE_SUPPORT_ZIP lxa_archive_support_zip_get_type()

#define LXA_ARCHIVE_SUPPORT_ZIP(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LXA_TYPE_ARCHIVE_SUPPORT_ZIP,      \
			LXAArchiveSupportZip))

#define LXA_IS_ARCHIVE_SUPPORT_ZIP(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LXA_TYPE_ARCHIVE_SUPPORT_ZIP))

#define LXA_ARCHIVE_SUPPORT_ZIP_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LXA_TYPE_ARCHIVE_SUPPORT_ZIP,      \
			LXAArchiveSupportZipClass))

#define LXA_IS_ARCHIVE_SUPPORT_ZIP_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LXA_TYPE_ARCHIVE_SUPPORT_ZIP))

typedef struct _LXAArchiveSupportZip LXAArchiveSupportZip;

struct _LXAArchiveSupportZip
{
	LXAArchiveSupport parent;
	gboolean  _extr_overwrite;
	gchar    *_extr_password;
};

typedef struct _LXAArchiveSupportZipClass LXAArchiveSupportZipClass;

struct _LXAArchiveSupportZipClass
{
	LXAArchiveSupportClass parent;
}; 

GType                lxa_archive_support_zip_get_type(void);
LXAArchiveSupport *  lxa_archive_support_zip_new();

gint                 lxa_archive_support_zip_add(LXAArchive *, GSList *);
gint                 lxa_archive_support_zip_extract(LXAArchive *, gchar *, GSList *);
gint                 lxa_archive_support_zip_remove(LXAArchive *, GSList *);

G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVE_SUPPORT_ZIP_H__ */
