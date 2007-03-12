#ifndef __LIBSQUEEZE_SUPPORT_H__
#define __LIBSQUEEZE_SUPPORT_H__ 

#define LSQ_TYPE_ARCHIVE_SUPPORT lsq_archive_support_get_type()

#define LSQ_ARCHIVE_SUPPORT(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT,      \
			LSQArchiveSupport))

#define LSQ_IS_ARCHIVE_SUPPORT(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_ARCHIVE_SUPPORT))

#define LSQ_ARCHIVE_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_ARCHIVE_SUPPORT,      \
			LSQArchiveSupportClass))

#define LSQ_IS_ARCHIVE_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_ARCHIVE_SUPPORT))

typedef struct _LSQArchiveSupport LSQArchiveSupport;
typedef struct _LSQArchiveSupportClass LSQArchiveSupportClass;

GType                lsq_archive_support_get_type(void);

GSList *             lsq_get_registered_support_list();
const gchar *        lsq_archive_support_get_id(LSQArchiveSupport *support);

LSQArchiveSupport *  lsq_get_support_for_mimetype(const gchar *mime_type);
LSQArchiveSupport *  lsq_get_support_for_mime_from_slist(GSList *list, const gchar *mime);
GSList *             lsq_archive_support_list_properties(LSQArchiveSupport *support, gchar *);

gint                 lsq_archive_support_add(LSQArchiveSupport *, LSQArchive *, GSList *);
gint                 lsq_archive_support_extract(LSQArchiveSupport *, LSQArchive *, const gchar *, GSList *);
gint                 lsq_archive_support_remove(LSQArchiveSupport *, LSQArchive *, GSList *);
gint                 lsq_archive_support_refresh(LSQArchiveSupport *, LSQArchive *);
gint                 lsq_archive_support_view(LSQArchiveSupport *, LSQArchive *, GSList *);
guint64              lsq_archive_support_get_max_n_files(LSQArchiveSupport *);

#endif /* __LIBSQUEEZE_SUPPORT_H__ */
