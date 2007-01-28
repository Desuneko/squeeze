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
	LSQ_ARCHIVESTATUS_PREPARE_VIEW,
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
	LSQArchiveStatus    status;
	LSQArchiveStatus    old_status;
	gchar              *status_msg;
	gdouble             progress;
	GPid                child_pid;
	GIOChannel         *ioc_in;
	GIOChannel         *ioc_out;
	GIOChannel         *ioc_err;
	gpointer            support;
	GSList             *files;
	gboolean            has_passwd;
	struct {
		guint64 archive_size;
		guint64 content_size;
		guint64 n_files;
		guint64 n_directories;
	} props;
	gchar *temp_dir;
	GSList *monitor_list;
};

typedef struct _LSQArchiveClass LSQArchiveClass;

struct _LSQArchiveClass
{
	GObjectClass parent;
}; 

GType               lsq_archive_get_type(void);
LSQArchive         *lsq_archive_new(gchar *, const gchar *) G_GNUC_INTERNAL;

void                lsq_archive_set_status(LSQArchive *archive, LSQArchiveStatus status) G_GNUC_INTERNAL;
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

/* void            lsq_archive_iter_set_filename(LSQArchive *, LSQArchiveIter *, const gchar *); */
void                lsq_archive_iter_set_mime(LSQArchive *, LSQArchiveIter *, ThunarVfsMimeInfo *) G_GNUC_INTERNAL;
void                lsq_archive_iter_set_prop_str(LSQArchive *, LSQArchiveIter *, guint, const gchar *) G_GNUC_INTERNAL;
void                lsq_archive_iter_set_prop_uint(LSQArchive *, LSQArchiveIter *, guint, guint) G_GNUC_INTERNAL;
void                lsq_archive_iter_set_prop_uint64(LSQArchive *, LSQArchiveIter *, guint, guint64) G_GNUC_INTERNAL;
void                lsq_archive_iter_set_prop_value(LSQArchive *, LSQArchiveIter *, guint, const GValue *) G_GNUC_INTERNAL;
void                lsq_archive_iter_set_props(LSQArchive *, LSQArchiveIter *, ...) G_GNUC_INTERNAL;
void                lsq_archive_iter_set_propsv(LSQArchive *, LSQArchiveIter *, gconstpointer *) G_GNUC_INTERNAL;

gboolean            lsq_archive_iter_get_prop_value(const LSQArchive *archive, const LSQArchiveIter *iter, guint n, GValue *value);
void                lsq_archive_iter_get_icon_name(const LSQArchive *archive, const LSQArchiveIter *iter, GValue *value, GtkIconTheme *icon_theme);
const gchar        *lsq_archive_iter_get_filename(const LSQArchive *archive, const LSQArchiveIter *);
LSQArchiveIter     *lsq_archive_add_file(LSQArchive *archive, const gchar *path) G_GNUC_INTERNAL;
gboolean            lsq_archive_del_file(LSQArchive *archive, const gchar *path);
LSQArchiveIter     *lsq_archive_get_iter(const LSQArchive *archive, const gchar *path);
GType               lsq_archive_get_entry_property_type(LSQArchive *archive, guint n);
const gchar        *lsq_archive_get_entry_property_name(LSQArchive *archive, guint n);
void                lsq_archive_clear_entry_property_types(LSQArchive *archive) G_GNUC_INTERNAL;
void                lsq_archive_set_entry_property_type(LSQArchive *archive , guint n, GType type, const gchar *name) G_GNUC_INTERNAL;
void                lsq_archive_set_entry_property_typesv(LSQArchive *archive, GType *types, const gchar **names) G_GNUC_INTERNAL;
guint               lsq_archive_n_property(LSQArchive *archive);
guint64             lsq_archive_get_n_files(LSQArchive *archive);
guint64             lsq_archive_get_n_directories(LSQArchive *archive);

gboolean            lsq_archive_stop(LSQArchive *archive);

void                lsq_archive_add_children(LSQArchive *, GSList *) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* __LIBSQUEEZE_ARCHIVE_H__ */
