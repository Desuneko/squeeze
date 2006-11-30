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


#define LSQ_TYPE_ARCHIVE_SUPPORT_ZIP lsq_archive_support_zip_get_type()

#define LSQ_ARCHIVE_SUPPORT_ZIP(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT_ZIP,      \
			LSQArchiveSupportZip))

#define LSQ_IS_ARCHIVE_SUPPORT_ZIP(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT_ZIP))

#define LSQ_ARCHIVE_SUPPORT_ZIP_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_ARCHIVE_SUPPORT_ZIP,      \
			LSQArchiveSupportZipClass))

#define LSQ_IS_ARCHIVE_SUPPORT_ZIP_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_ARCHIVE_SUPPORT_ZIP))

typedef struct _LSQArchiveSupportZip LSQArchiveSupportZip;

struct _LSQArchiveSupportZip
{
	LSQArchiveSupport parent;
	gboolean  _extr_overwrite;
	gchar    *_extr_password;

	gboolean  _view_size;
	gboolean  _view_time;
	gboolean  _view_date;
	gboolean  _view_ratio;
	gboolean  _view_length;
	gboolean  _view_method;
	gboolean  _view_crc_32;
};

typedef struct _LSQArchiveSupportZipClass LSQArchiveSupportZipClass;

struct _LSQArchiveSupportZipClass
{
	LSQArchiveSupportClass parent;
}; 

GType                lsq_archive_support_zip_get_type(void);
LSQArchiveSupport *  lsq_archive_support_zip_new();

G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVE_SUPPORT_ZIP_H__ */
