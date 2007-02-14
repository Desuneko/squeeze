#ifndef __LIBSQUEEZE_SUPPORT_H__
#define __LIBSQUEEZE_SUPPORT_H__ 

typedef struct _LSQArchiveSupport LSQArchiveSupport;

LSQArchiveSupport *  lsq_get_support_for_mimetype(const gchar *mime_type);
LSQArchiveSupport *  lsq_get_support_for_mime_from_slist(GSList *list, const gchar *mime);

gint                 lsq_archive_support_add(LSQArchiveSupport *, LSQArchive *, GSList *);
gint                 lsq_archive_support_extract(LSQArchiveSupport *, LSQArchive *, const gchar *, GSList *);
gint                 lsq_archive_support_remove(LSQArchiveSupport *, LSQArchive *, GSList *);
gint                 lsq_archive_support_refresh(LSQArchiveSupport *, LSQArchive *);
gint                 lsq_archive_support_view(LSQArchiveSupport *, LSQArchive *, GSList *);
guint64              lsq_archive_support_get_max_n_files(LSQArchiveSupport *);

#endif /* __LIBSQUEEZE_SUPPORT_H__ */
