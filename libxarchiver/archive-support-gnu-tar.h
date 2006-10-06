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
#ifndef __LIBXARCHIVER_ARCHIVE_SUPPORT_GNU_TAR_H__
#define __LIBXARCHIVER_ARCHIVE_SUPPORT_GNU_TAR_H__

G_BEGIN_DECLS

#define LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR lxa_archive_support_gnu_tar_get_type()

#define LXA_ARCHIVE_SUPPORT_GNU_TAR(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR,      \
			LXAArchiveSupportGnuTar))

#define LXA_IS_ARCHIVE_SUPPORT_GNU_TAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR))

#define LXA_ARCHIVE_SUPPORT_GNU_TAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR,      \
			LXAArchiveSupportGnuTarClass))

#define LXA_IS_ARCHIVE_SUPPORT_GNU_TAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR))

typedef struct _LXAArchiveSupportGnuTar LXAArchiveSupportGnuTar;

struct _LXAArchiveSupportGnuTar
{
	LXAArchiveSupport parent;
	gchar *app_name;
	gboolean _extr_overwrite;
	gboolean _extr_touch;
	guint    _extr_strip;

/* Optional properties */
	gboolean _view_size;
	gboolean _view_date;
	gboolean _view_time;
	gboolean _view_owner;
	gboolean _view_rights;
};

typedef struct _LXAArchiveSupportGnuTarClass LXAArchiveSupportGnuTarClass;

struct _LXAArchiveSupportGnuTarClass
{
	LXAArchiveSupportClass parent;
}; 

GType                lxa_archive_support_gnu_tar_get_type(void);
LXAArchiveSupport *  lxa_archive_support_gnu_tar_new();

gint                 lxa_archive_support_gnu_tar_add(LXAArchive *, GSList *);
gint                 lxa_archive_support_gnu_tar_extract(LXAArchive *, gchar *, GSList *);
gint                 lxa_archive_support_gnu_tar_remove(LXAArchive *, GSList *);
gint                 lxa_archive_support_gnu_tar_refresh(LXAArchive *);

G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVE_SUPPORT_GNU_TAR_H__ */