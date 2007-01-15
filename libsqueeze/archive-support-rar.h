/*
 *  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
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
#ifndef __LIBSQUEEZE_ARCHIVE_SUPPORT_RAR_H__
#define __LIBSQUEEZE_ARCHIVE_SUPPORT_RAR_H__

G_BEGIN_DECLS


#define LSQ_TYPE_ARCHIVE_SUPPORT_RAR lsq_archive_support_rar_get_type()

#define LSQ_ARCHIVE_SUPPORT_RAR(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT_RAR,      \
			LSQArchiveSupportRar))

#define LSQ_IS_ARCHIVE_SUPPORT_RAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT_RAR))

#define LSQ_ARCHIVE_SUPPORT_RAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_ARCHIVE_SUPPORT_RAR,      \
			LSQArchiveSupportRarClass))

#define LSQ_IS_ARCHIVE_SUPPORT_RAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_ARCHIVE_SUPPORT_RAR))

typedef struct _LSQArchiveSupportRar LSQArchiveSupportRar;

struct _LSQArchiveSupportRar
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
	gboolean  _view_version;
	gboolean  _view_rights;
};

typedef struct _LSQArchiveSupportRarClass LSQArchiveSupportRarClass;

struct _LSQArchiveSupportRarClass
{
	LSQArchiveSupportClass parent;
}; 

GType                lsq_archive_support_rar_get_type(void);
LSQArchiveSupport *  lsq_archive_support_rar_new();

G_END_DECLS

#endif /* __LIBSQUEEZE_ARCHIVE_SUPPORT_RAR_H__ */
