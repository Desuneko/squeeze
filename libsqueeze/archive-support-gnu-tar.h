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
#ifndef __LIBSQUEEZE_ARCHIVE_SUPPORT_GNU_TAR_H__
#define __LIBSQUEEZE_ARCHIVE_SUPPORT_GNU_TAR_H__

G_BEGIN_DECLS

#define LSQ_TYPE_ARCHIVE_SUPPORT_GNU_TAR lsq_archive_support_gnu_tar_get_type()

#define LSQ_ARCHIVE_SUPPORT_GNU_TAR(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT_GNU_TAR,      \
			LSQArchiveSupportGnuTar))

#define LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT_GNU_TAR))

#define LSQ_ARCHIVE_SUPPORT_GNU_TAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_ARCHIVE_SUPPORT_GNU_TAR,      \
			LSQArchiveSupportGnuTarClass))

#define LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_ARCHIVE_SUPPORT_GNU_TAR))

typedef struct _LSQArchiveSupportGnuTar LSQArchiveSupportGnuTar;

struct _LSQArchiveSupportGnuTar
{
	LSQArchiveSupport parent;
	gchar *app_name;
	gboolean _extr_overwrite;
	gboolean _extr_touch;
	guint    _extr_strip;
	gboolean _extr_keep_old;
	gboolean _extr_keep_newer;

	gchar   *_add_mode;

/* Optional properties */
	gboolean _view_size;
	gboolean _view_date;
	gboolean _view_time;
	gboolean _view_owner;
	gboolean _view_rights;
};

typedef struct _LSQArchiveSupportGnuTarClass LSQArchiveSupportGnuTarClass;

struct _LSQArchiveSupportGnuTarClass
{
	LSQArchiveSupportClass parent;
}; 

GType                lsq_archive_support_gnu_tar_get_type(void) G_GNUC_INTERNAL;
LSQArchiveSupport *  lsq_archive_support_gnu_tar_new() G_GNUC_INTERNAL;

gint                 lsq_archive_support_gnu_tar_add(LSQArchive *, GSList *) G_GNUC_INTERNAL;
gint                 lsq_archive_support_gnu_tar_extract(LSQArchive *, const gchar *, GSList *) G_GNUC_INTERNAL;
gint                 lsq_archive_support_gnu_tar_remove(LSQArchive *, GSList *) G_GNUC_INTERNAL;
gint                 lsq_archive_support_gnu_tar_refresh(LSQArchive *) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* __LIBSQUEEZE_ARCHIVE_SUPPORT_GNU_TAR_H__ */
