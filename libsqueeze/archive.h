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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifndef __LIBSQUEEZE_ARCHIVE_H__
#define __LIBSQUEEZE_ARCHIVE_H__ 
G_BEGIN_DECLS

typedef enum
{
	LSQ_ARCHIVESTATUS_INIT = 0,
	LSQ_ARCHIVESTATUS_IDLE,
	LSQ_ARCHIVESTATUS_ADD,
	LSQ_ARCHIVESTATUS_EXTRACT,
	LSQ_ARCHIVESTATUS_REMOVE,
	LSQ_ARCHIVESTATUS_REFRESH,
	LSQ_ARCHIVESTATUS_ERROR,
	LSQ_ARCHIVESTATUS_CUSTOM,
	LSQ_ARCHIVESTATUS_USERBREAK
} LSQArchiveStatus;

enum
{
	LSQ_ARCHIVE_PROP_FILENAME = 0,
	LSQ_ARCHIVE_PROP_MIME_TYPE,
	LSQ_ARCHIVE_PROP_USER
};

typedef struct _LSQEntry LSQEntry;
typedef LSQEntry LSQArchiveIter;


#define LSQ_ARCHIVE(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			lsq_archive_get_type(),      \
			LSQArchive))

#define LSQ_IS_ARCHIVE(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			lsq_archive_get_type()))

#define LSQ_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			lsq_archive_get_type(),      \
			LSQArchiveClass))

#define LSQ_IS_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			lsq_archive_get_type()))


typedef struct _LSQArchive LSQArchive;


struct _LSQArchive
{
	GObject parent;
	gchar              *path;
	ThunarVfsPath      *path_info;
	ThunarVfsInfo      *file_info;
	ThunarVfsMimeInfo  *mime_info;
	guint               entry_n_property;
	GType              *entry_property_types;
	gchar             **entry_property_names;
	LSQEntry           *root_entry;
#ifdef G_THREADS_ENABLED
	GStaticRWLock       rw_lock;
#endif
	LSQArchiveStatus    status;
	LSQArchiveStatus    old_status;
	gchar              *status_msg;
	gdouble             progress;
	GPid                child_pid;
	GIOChannel         *ioc_in;
	GIOChannel         *ioc_out;
	GIOChannel         *ioc_err;
	gpointer            support;
	gchar              *tmp_file;
	gchar              *files;
	gboolean            has_passwd;
	struct {
		gint64 archive_size;
		gint64 content_size;
	} props;
};

typedef struct _LSQArchiveClass LSQArchiveClass;

struct _LSQArchiveClass
{
	GObjectClass parent;
}; 

GType               lsq_archive_get_type(void);
LSQArchive         *lsq_archive_new(gchar *, const gchar *);

void                lsq_archive_set_status(LSQArchive *archive, LSQArchiveStatus status);
LSQArchiveStatus    lsq_archive_get_status(LSQArchive *archive);
const gchar *       lsq_archive_get_status_msg(LSQArchive *archive);
LSQArchiveStatus    lsq_archive_get_old_status(LSQArchive *archive);
const gchar *       lsq_archive_get_filename(LSQArchive *archive);
const gchar *       lsq_archive_get_mimetype(LSQArchive *archive);

gboolean            lsq_archive_iter_is_directory(const LSQArchive *, const LSQArchiveIter *);
guint               lsq_archive_iter_n_children(const LSQArchive *, const LSQArchiveIter *);
LSQArchiveIter     *lsq_archive_iter_nth_child(LSQArchive *, LSQArchiveIter *, guint);
LSQArchiveIter     *lsq_archive_iter_add_child(LSQArchive *, LSQArchiveIter *, const gchar *);
LSQArchiveIter     *lsq_archive_iter_get_child(const LSQArchive *, const LSQArchiveIter *, const gchar *);
gboolean            lsq_archive_iter_del_child(LSQArchive *, LSQArchiveIter *, LSQArchiveIter *);

/* void            lsq_archive_iter_set_filename(LSQArchive *, LSQArchiveIter *, const gchar *); */
void                lsq_archive_iter_set_mime(LSQArchive *, LSQArchiveIter *, ThunarVfsMimeInfo *);
void                lsq_archive_iter_set_prop_str(LSQArchive *, LSQArchiveIter *, guint, const gchar *);
void                lsq_archive_iter_set_prop_uint(LSQArchive *, LSQArchiveIter *, guint, guint);
void                lsq_archive_iter_set_prop_uint64(LSQArchive *, LSQArchiveIter *, guint, guint64);
void                lsq_archive_iter_set_prop_value(LSQArchive *, LSQArchiveIter *, guint, const GValue *);
void                lsq_archive_iter_set_props(LSQArchive *, LSQArchiveIter *, ...);
void                lsq_archive_iter_set_propsv(LSQArchive *, LSQArchiveIter *, gconstpointer *);

gboolean            lsq_archive_iter_get_prop_value(const LSQArchive *, const LSQArchiveIter *, guint, GValue *);
void                lsq_archive_iter_get_icon_name(const LSQArchive *, const LSQArchiveIter *, GValue *, GtkIconTheme *icon_theme);

LSQArchiveIter     *lsq_archive_add_file(LSQArchive *, const gchar *);
LSQArchiveIter     *lsq_archive_get_iter(LSQArchive *, const gchar *);
GType               lsq_archive_get_entry_property_type(LSQArchive *, guint);
const gchar        *lsq_archive_get_entry_property_name(LSQArchive *, guint);
void                lsq_archive_clear_entry_property_types(LSQArchive *);
void                lsq_archive_set_entry_property_type(LSQArchive *, guint, GType, const gchar *);
void                lsq_archive_set_entry_property_typesv(LSQArchive *, GType *, const gchar **);
guint               lsq_archive_n_property(LSQArchive *);

G_END_DECLS

#endif /* __LIBSQUEEZE_ARCHIVE_H__ */
